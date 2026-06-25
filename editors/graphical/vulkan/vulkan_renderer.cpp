#include "vulkan_renderer.h"

#include "vulkan_window.h"

VulkanRenderer::VulkanRenderer(VulkanWindow *w, std::shared_ptr<RenderData> renderData):
    m_window(w), m_renderData(renderData) {}

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {

    VkPhysicalDevice physDev = m_window->physicalDevice();
    VkPhysicalDeviceMemoryProperties memProperties;
    m_window->vulkanInstance()->functions()->vkGetPhysicalDeviceMemoryProperties(physDev, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    qFatal("Failed to find suitable memory type!");
    return 0;
}

void VulkanRenderer::initResources() {

    // Access device
    m_device = m_window->device();
    m_devFuncs = m_window->vulkanInstance()->deviceFunctions(m_device);
    m_concurrentFrameCount = static_cast<uint32_t>(m_window->concurrentFrameCount());
    m_uboDirty.assign(m_concurrentFrameCount, true);

    // Create general resources
    createVertexBuffer();
    if (!m_renderData->indices.empty()) {
        createIndexBuffer();
    }
    createAxisBuffer();
    createUniformBuffer();
    if (m_renderData->format == RenderType::Color) {
        createTexture();
    }

    // Create pipeline resources
    createPipelineStorage();
    createDescriptorSets();
    createPipelines();
}

void VulkanRenderer::startNextFrame() {

    // Check swapchain size
    const QSize size = m_window->swapChainImageSize();
    if (size.width() < 1 || size.height() < 1) {
        m_window->frameReady();
        return;
    }

    // Check if mesh data has changed
    if (m_window->isDirty()) {

        // Wait for the GPU to finish rendering
        m_devFuncs->vkDeviceWaitIdle(m_window->device());

        // Access new mesh data
        m_renderData = m_window->getRenderData();

        // Recreate buffers when new data is present
        if (m_renderData && !m_renderData->data.empty()) {
            createVertexBuffer();
            if (!m_renderData->indices.empty()) {
                createIndexBuffer();
            }
        }

        // Clear dirty status
        m_window->clearDirty();
    }

    // Check if the uniform buffer has changed
    if (m_window->isUboDirty()) {
        std::fill(m_uboDirty.begin(), m_uboDirty.end(), true);
        m_window->clearUboDirty();
    }

    // Update uniform data as needed
    int currentFrame = m_window->currentFrame();
    if (m_uboDirty[currentFrame]) {

        TransformMatrices matrices = m_window->getMatrices();
        UniformBufferObject ubo;

        memcpy(ubo.model, matrices.model.constData(), 16 * sizeof(float));
        memcpy(ubo.view, matrices.view.constData(), 16 * sizeof(float));
        memcpy(ubo.proj, matrices.proj.constData(), 16 * sizeof(float));

        // Write to the memory block for the current frame
        uint8_t* destPtr = static_cast<uint8_t*>(m_mappedUniformData) +
                           (currentFrame * m_alignedUniformSize);
        memcpy(destPtr, &ubo, sizeof(ubo));

        // Mark this frame as clean
        m_uboDirty[currentFrame] = false;
    }

    // Set clear colors
    VkClearValue clearValues[3] = {};
    // clearValues[0].color = {{ 1.0f, 1.0f, 1.0f, 1.0f }};
    clearValues[0].color = {{ 0.1f, 0.1f, 0.12f, 1.0f }};
    clearValues[1].depthStencil = { 1.0f, 0 };
    clearValues[2].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};

    // Compute scaled maximum dimension
    float scaledDim = m_maxDim / 5.0f;

    // Begin the render pass
    const QSize sz = m_window->swapChainImageSize();
    VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
    VkRenderPassBeginInfo rpBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = m_window->defaultRenderPass(),
        .framebuffer = m_window->currentFramebuffer(),
        .renderArea = {
            {0, 0},
            {static_cast<uint32_t>(sz.width()), static_cast<uint32_t>(sz.height())}
        },
        .clearValueCount = m_window->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3u : 2u,
        .pClearValues = clearValues
    };
    m_devFuncs->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Create viewport
    VkDeviceSize vbOffset = 0;
    m_devFuncs->vkCmdBindVertexBuffers(cmdBuf, 0, 1, &m_vertexBuffer, &vbOffset);
    if (!m_renderData->indices.empty()) {
        m_devFuncs->vkCmdBindIndexBuffer(cmdBuf, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }
    VkViewport viewport = {
        .x = 0.0f, .y = 0.0f,
        .width = static_cast<float>(sz.width()),
        .height = static_cast<float>(sz.height()),
        .minDepth = 0.0f, .maxDepth = 1.0f
    };
    m_devFuncs->vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

    // Create scissor
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = {static_cast<uint32_t>(sz.width()), static_cast<uint32_t>(sz.height())}
    };
    m_devFuncs->vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    // Bind descriptor set and pipeline
    m_devFuncs->vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                                        &m_descriptorSets[m_window->currentFrame()], 0, nullptr);
    m_devFuncs->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[0]);

    // Render the data
    switch(m_renderData->format) {
    case RenderType::Surface:
        for (int i=0; i<m_renderData->patches.size(); i++) {

            // Set push constants
            m_devFuncs->vkCmdPushConstants(cmdBuf, m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                0, 4 * sizeof(float), &(patchColors[i]));

            // Execute draw
            const auto& patch = m_renderData->patches[i];
            m_devFuncs->vkCmdDrawIndexed(cmdBuf, patch.count, 1, patch.first, 0, 0);
        }
        break;
    case RenderType::Mesh:
        for (int i=0; i<m_renderData->patches.size(); i++) {

            // Set push constants
            m_devFuncs->vkCmdPushConstants(cmdBuf, m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                                           0, 4 * sizeof(float), &(patchColors[i]));

            // Execute draw
            const auto& patch = m_renderData->patches[i];
            m_devFuncs->vkCmdDraw(cmdBuf, patch.count, 1, patch.first, 0);
        }
        break;
    case RenderType::Color:
        m_devFuncs->vkCmdDrawIndexed(cmdBuf, m_renderData->indices.size(), 1, 0, 0, 0);
        break;

    default:
        break;
    }

    // Draw the axes
    VkDeviceSize axisOffset = 0;
    m_devFuncs->vkCmdBindVertexBuffers(cmdBuf, 0, 1, &m_axisBuffer, &axisOffset);

    // Bind pipeline
    m_devFuncs->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[1]);

    // Set push constants -
    m_devFuncs->vkCmdPushConstants(cmdBuf, m_pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                                   0, sizeof(float), &scaledDim);

    // Draw the axes
    m_devFuncs->vkCmdDraw(cmdBuf, 6, 1, 0, 0);
    m_devFuncs->vkCmdEndRenderPass(cmdBuf);
    m_window->frameReady();
}

void VulkanRenderer::createPipelineStorage() {

    // Create pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO
    };
    VkResult err = m_devFuncs->vkCreatePipelineCache(m_device, &pipelineCacheInfo, nullptr, &m_pipelineCache);
    if (err != VK_SUCCESS) qFatal("Failed to create pipeline cache: %d", err);

    // Build the pool sizes
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_concurrentFrameCount });
    if (m_renderData->format == RenderType::Color) {
        poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_concurrentFrameCount });
    }

    // Create the descriptor pool
    VkDescriptorPoolCreateInfo descPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = m_concurrentFrameCount,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };
    err = m_devFuncs->vkCreateDescriptorPool(m_device, &descPoolInfo, nullptr, &m_descPool);
    if (err != VK_SUCCESS) qFatal("Failed to create descriptor pool: %d", err);
}

void VulkanRenderer::createDescriptorSets() {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    // Bind the UBO
    bindings.push_back({
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr
    });

    // Bind the color sampler if necessary
    if (m_renderData->format == RenderType::Color) {
        bindings.push_back({
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        });
    }

    // Create the descriptor set layout
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };
    VkResult err = m_devFuncs->vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout);
    if (err != VK_SUCCESS) qFatal("Failed to create descriptor set layout");

    // Allocate the sets
    std::vector<VkDescriptorSetLayout> layouts(m_concurrentFrameCount, m_descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_descPool,
        .descriptorSetCount = m_concurrentFrameCount,
        .pSetLayouts = layouts.data()
    };

    err = m_devFuncs->vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data());
    if (err != VK_SUCCESS) qFatal("Failed to allocate descriptor sets");

    // Update the sets
    for (uint32_t i = 0; i < m_concurrentFrameCount; ++i) {
        std::vector<VkWriteDescriptorSet> descriptorWrites;

        // Write UBO
        descriptorWrites.push_back({
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descriptorSets[i],
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &m_uniformBufferInfo[i]
        });

        // Write sampler if necessary
        if (m_renderData->format == RenderType::Color) {
            descriptorWrites.push_back({
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_descriptorSets[i],
                .dstBinding = 1,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &m_colorMapImageInfo
            });
        }
        m_devFuncs->vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()),
                                           descriptorWrites.data(), 0, nullptr);
    }
}

void VulkanRenderer::createPipelines() {

    // Configure push constants
    VkPushConstantRange range = {
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = 4 * sizeof(float)
    };

    // Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo;
    if (m_renderData->format != RenderType::Color) {
        pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_descriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &range
        };
    } else {
        pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_descriptorSetLayout
        };
    }
    VkResult err = m_devFuncs->vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (err != VK_SUCCESS) qFatal("Failed to create flat pipeline layout: %d", err);

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = m_window->sampleCountFlagBits()
    };

    VkPipelineDepthStencilStateCreateInfo depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    VkVertexInputBindingDescription vertexBindingDesc = {};
    std::vector<VkVertexInputAttributeDescription> vertexAttrDescs;
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    VkShaderModule shaderModules[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    if (m_renderData->format == RenderType::Surface) {
        vertexBindingDesc = {
            .binding = 0,
            .stride = 3 * sizeof(float),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };
        vertexAttrDescs.push_back({
            .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0
        });
        vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertexBindingDesc,
            .vertexAttributeDescriptionCount = 1,
            .pVertexAttributeDescriptions = vertexAttrDescs.data()
        };
        shaderModules[0] = createShader(QStringLiteral(":/shaders/model/vert.spv"));
        shaderModules[1] = createShader(QStringLiteral(":/shaders/model/frag.spv"));
    }
    else if (m_renderData->format == RenderType::Mesh) {
        vertexBindingDesc = {
            .binding = 0,
            .stride = 6 * sizeof(float),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };
        vertexAttrDescs.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 });
        vertexAttrDescs.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, 3 * sizeof(float) });
        vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertexBindingDesc,
            .vertexAttributeDescriptionCount = 2,
            .pVertexAttributeDescriptions = vertexAttrDescs.data()
        };
        shaderModules[0] = createShader(QStringLiteral(":/shaders/mesh/vert.spv"));
        shaderModules[1] = createShader(QStringLiteral(":/shaders/mesh/frag.spv"));
    }
    else if (m_renderData->format == RenderType::Color) {
        vertexBindingDesc = {
            .binding = 0,
            .stride = 4 * sizeof(float),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        };
        vertexAttrDescs.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 });
        vertexAttrDescs.push_back({ 1, 0, VK_FORMAT_R32_SFLOAT, 3 * sizeof(float) });
        vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertexBindingDesc,
            .vertexAttributeDescriptionCount = 2,
            .pVertexAttributeDescriptions = vertexAttrDescs.data()
        };
        shaderModules[0] = createShader(QStringLiteral(":/shaders/color/vert.spv"));
        shaderModules[1] = createShader(QStringLiteral(":/shaders/color/frag.spv"));
    }

    VkPipelineShaderStageCreateInfo shaderStages[2] = {{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = shaderModules[0],
        .pName = "main"
    },
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = shaderModules[1],
        .pName = "main"
    }};

    VkPipelineInputAssemblyStateCreateInfo assemblyState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    // Rasterization
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f
    };

    VkGraphicsPipelineCreateInfo pipelineCreateInfos[2];
    pipelineCreateInfos[0] = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &assemblyState,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = m_pipelineLayout,
        .renderPass = m_window->defaultRenderPass()
    };

    // Disable depth writes for transparent axes
    VkPipelineDepthStencilStateCreateInfo axisDepthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
    };

    VkPipelineColorBlendAttachmentState axisColorBlendAttachment = {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo axisColorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &axisColorBlendAttachment
    };

    // Set axis binding and attributes
    VkVertexInputBindingDescription axisVertexBindingDesc = {
        .binding = 0,
        .stride = 3 * sizeof(float),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkVertexInputAttributeDescription axisVertexAttrDesc = {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = 0
    };

    VkPipelineVertexInputStateCreateInfo axisVertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &axisVertexBindingDesc,
        .vertexAttributeDescriptionCount = 1,
        .pVertexAttributeDescriptions = &axisVertexAttrDesc
    };

    VkShaderModule axisShaderModules[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    axisShaderModules[0] = createShader(QStringLiteral(":/shaders/axis/vert.spv"));
    axisShaderModules[1] = createShader(QStringLiteral(":/shaders/axis/frag.spv"));

    VkPipelineShaderStageCreateInfo axisShaderStages[2] = {{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = axisShaderModules[0],
        .pName = "main"
    },
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = axisShaderModules[1],
        .pName = "main"
    }};

    VkPipelineInputAssemblyStateCreateInfo axisAssemblyState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    // Rasterization
    VkPipelineRasterizationStateCreateInfo axisRasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f
    };

    pipelineCreateInfos[1] = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = axisShaderStages,
        .pVertexInputState = &axisVertexInputInfo,
        .pInputAssemblyState = &axisAssemblyState,
        .pViewportState = &viewportState,
        .pRasterizationState = &axisRasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &axisDepthStencil,
        .pColorBlendState = &axisColorBlending,
        .pDynamicState = &dynamicState,
        .layout = m_pipelineLayout,
        .renderPass = m_window->defaultRenderPass()
    };

    // Create pipeline
    err = m_devFuncs->vkCreateGraphicsPipelines(
        m_device, m_pipelineCache, 2, pipelineCreateInfos, nullptr, m_pipelines.data());
    if (err != VK_SUCCESS) qFatal("Failed to create pipelines: %d", err);

    // Destroy shader modules
    if (shaderModules[0] != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyShaderModule(m_device, shaderModules[0], nullptr);
        shaderModules[0] = VK_NULL_HANDLE;
    }
    if (shaderModules[1] != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyShaderModule(m_device, shaderModules[1], nullptr);
        shaderModules[1] = VK_NULL_HANDLE;
    }
    if (axisShaderModules[0] != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyShaderModule(m_device, axisShaderModules[0], nullptr);
        axisShaderModules[0] = VK_NULL_HANDLE;
    }
    if (axisShaderModules[1] != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyShaderModule(m_device, axisShaderModules[1], nullptr);
        axisShaderModules[1] = VK_NULL_HANDLE;
    }
}

VkShaderModule VulkanRenderer::createShader(const QString &name) {

    QFile file(name);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Failed to read shader %s", qPrintable(name));
        return VK_NULL_HANDLE;
    }
    QByteArray blob = file.readAll();
    file.close();

    VkShaderModuleCreateInfo shaderInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = (size_t)blob.size(),
        .pCode = reinterpret_cast<const uint32_t *>(blob.constData())
    };
    VkShaderModule shaderModule;
    VkResult err = m_devFuncs->vkCreateShaderModule(m_window->device(), &shaderInfo,
                                                    nullptr, &shaderModule);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create shader module: %d", err);
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

void VulkanRenderer::releaseResources() {

    m_devFuncs->vkDeviceWaitIdle(m_device);

    // Destroy pipelines
    for (int i=0; i<m_pipelines.size(); i++) {
        if (m_pipelines[i] != VK_NULL_HANDLE) {
            m_devFuncs->vkDestroyPipeline(m_device, m_pipelines[i], nullptr);
            m_pipelines[i] = VK_NULL_HANDLE;
        }
    }

    // Destroy pipeline layout and Cache
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    if (m_pipelineCache != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyPipelineCache(m_device, m_pipelineCache, nullptr);
        m_pipelineCache = VK_NULL_HANDLE;
    }

    // Destroy descriptor objects
    if (m_descPool != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyDescriptorPool(m_device, m_descPool, nullptr);
        m_descPool = VK_NULL_HANDLE;
    }
    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }

    // Destroy axis buffer
    if (m_axisBuffer != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyBuffer(m_device, m_axisBuffer, nullptr);
        m_axisBuffer = VK_NULL_HANDLE;
    }
    if (m_axisBufferMemory != VK_NULL_HANDLE) {
        m_devFuncs->vkFreeMemory(m_device, m_axisBufferMemory, nullptr);
        m_axisBufferMemory = VK_NULL_HANDLE;
    }

    // Destroy index buffer
    if (m_indexBuffer != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
        m_indexBuffer = VK_NULL_HANDLE;
    }
    if (m_indexBufferMemory != VK_NULL_HANDLE) {
        m_devFuncs->vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
        m_indexBufferMemory = VK_NULL_HANDLE;
    }

    // Destroy vertex buffer
    if (m_vertexBuffer != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
    }
    if (m_vertexBufferMemory != VK_NULL_HANDLE) {
        m_devFuncs->vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
        m_vertexBufferMemory = VK_NULL_HANDLE;
    }

    // Destroy sampler
    if (m_sampler != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroySampler(m_device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }

    // Destroy image view
    if (m_imageView != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyImageView(m_device, m_imageView, nullptr);
        m_imageView = VK_NULL_HANDLE;
    }

    // Destroy the image
    if (m_textureImage != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyImage(m_device, m_textureImage, nullptr);
        m_textureImage = VK_NULL_HANDLE;
    }

    // Free image memory
    if (m_textureImageMemory != VK_NULL_HANDLE) {
        m_devFuncs->vkFreeMemory(m_device, m_textureImageMemory, nullptr);
        m_textureImageMemory = VK_NULL_HANDLE;
    }

    // Unmap/destroy uniform buffer
    if (m_uniformBuffer != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyBuffer(m_device, m_uniformBuffer, nullptr);
        m_uniformBuffer = VK_NULL_HANDLE;
    }
    if (m_uniformBufferMemory != VK_NULL_HANDLE) {
        m_devFuncs->vkUnmapMemory(m_device, m_uniformBufferMemory);
        m_devFuncs->vkFreeMemory(m_device, m_uniformBufferMemory, nullptr);
        m_uniformBufferMemory = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::initSwapChainResources() {

    // Get new dimensions
    QSize sz = m_window->swapChainImageSize();

    // Protect against division by zero
    float aspect = 1.0f;
    if (sz.height() > 0 && sz.width() > 0) {
        aspect = static_cast<float>(sz.width()) / static_cast<float>(sz.height());
    }

    // Calculate dynamic zNear and zFar based on the bounding box
    float zNear = 0.001f;
    float zFar = 1000.0f;
    float dx = m_renderData->boundingBoxMax[0] - m_renderData->boundingBoxMin[0];
    float dy = m_renderData->boundingBoxMax[1] - m_renderData->boundingBoxMin[1];
    float dz = m_renderData->boundingBoxMax[2] - m_renderData->boundingBoxMin[2];

    // Calculate the diagonal length of the bounding box
    float boundsDiagonal = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (boundsDiagonal > 1e-6f) {
        zNear = boundsDiagonal * 0.001f;
        zFar  = boundsDiagonal * 100.0f;
    }

    // Build the projection matrix
    QMatrix4x4 projMatrix = m_window->clipCorrectionMatrix();
    projMatrix.perspective(45.0f, aspect, zNear, zFar);

    // Update the window
    m_window->setProjMatrix(projMatrix);
}

void VulkanRenderer::releaseSwapChainResources() {}
