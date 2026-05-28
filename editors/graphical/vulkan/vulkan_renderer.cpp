#include "vulkan_renderer.h"

#include "vulkan_window.h"

VulkanRenderer::VulkanRenderer(VulkanWindow *w, std::shared_ptr<MeshData> meshData):
    m_window(w), m_meshData(meshData) {}

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
    createVertexIndexBuffers();
    createUniformBuffer();
    if (m_meshData->format == VertexFormat::Color) {
        createTexture();
    }

    // Create pipeline resources
    createPipelineStorage();
    createDescriptorSets();
    createPipelineLayouts();
    createSharedPipelineStructs();

    // Create pipeline info structures
    std::vector<VkGraphicsPipelineCreateInfo> pipelineInfos;
    pipelineInfos.push_back(createFlatPipelineInfo());
    pipelineInfos.push_back(createAxisPipelineInfo());

    // Create pipeline
    m_pipelines.resize(pipelineInfos.size());
    VkResult err = m_devFuncs->vkCreateGraphicsPipelines(
        m_device, m_pipelineCache, static_cast<uint32_t>(pipelineInfos.size()),
        pipelineInfos.data(), nullptr, m_pipelines.data());
    if (err != VK_SUCCESS) qFatal("Failed to create pipelines: %d", err);

    // Destroy shader modules
    if (m_flatShaderModules[0] != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyShaderModule(m_device, m_flatShaderModules[0], nullptr);
        m_flatShaderModules[0] = VK_NULL_HANDLE;
    }
    if (m_flatShaderModules[1] != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyShaderModule(m_device, m_flatShaderModules[1], nullptr);
        m_flatShaderModules[1] = VK_NULL_HANDLE;
    }
    if (m_axisShaderModules[0] != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyShaderModule(m_device, m_axisShaderModules[0], nullptr);
        m_axisShaderModules[0] = VK_NULL_HANDLE;
    }
    if (m_axisShaderModules[1] != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyShaderModule(m_device, m_axisShaderModules[1], nullptr);
        m_axisShaderModules[1] = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::startNextFrame() {

    // Check swapchain size
    const QSize size = m_window->swapChainImageSize();
    if (size.width() < 1 || size.height() < 1) {
        m_window->frameReady();
        return;
    }

    // Check if vertex data has changed
    /*
    if (m_window->isDirty()) {
        m_meshData = m_window->getMeshData();
        if (m_meshData) {

            // Calculate the byte size of the incoming data
            VkDeviceSize dataSize = m_meshData->data.size() * sizeof(float);
            if (dataSize > MAX_VERTEX_BUFFER_SIZE) {
                qWarning("Data exceeds allocated buffer size!");
                m_window->clearDirty();
                m_currentVertexCount = 0;
            }

            // Copy data to the vertex buffer
            memcpy(m_mappedVertexData, m_meshData->data.data(), dataSize);
        }

        // Clear dirty status
        m_window->clearDirty();
    }
    */

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
    clearValues[0].color = {{ 1.0f, 1.0f, 1.0f, 1.0f }};
    clearValues[1].depthStencil = { 1.0f, 0 };
    clearValues[2].color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};

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
    m_devFuncs->vkCmdBindIndexBuffer(cmdBuf, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = static_cast<float>(sz.width()),
        .height = static_cast<float>(sz.height()),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    m_devFuncs->vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

    // Create scissor
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = {static_cast<uint32_t>(sz.width()), static_cast<uint32_t>(sz.height())}
    };
    m_devFuncs->vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    // Draw the mesh
    switch(m_meshData->format) {
    case VertexFormat::Flat:
        m_devFuncs->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[0]);
        m_devFuncs->vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_flatPipelineLayout, 0, 1,
                                            &m_globalDescSet[m_window->currentFrame()], 0, nullptr);

        for (int i=0; i<m_meshData->patches.size(); i++) {

            // Set push constants
            m_devFuncs->vkCmdPushConstants(cmdBuf, m_flatPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                0, 4 * sizeof(float), &(flatColors[i]));

            // Execute draw
            const auto& patch = m_meshData->patches[i];
            m_devFuncs->vkCmdDrawIndexed(cmdBuf, patch.indexCount, 1, patch.firstIndex, 0, 0);
        }

        break;

    case VertexFormat::Wireframe:
        m_devFuncs->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[1]);
        m_devFuncs->vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_flatPipelineLayout, 0, 1,
                                            &m_globalDescSet[m_window->currentFrame()], 0, nullptr);
        break;

    case VertexFormat::Color:
        m_devFuncs->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[2]);
        // Bind Texture Descriptor Set (Set 1) specifically for this mesh
        m_devFuncs->vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                            m_colorPipelineLayout, 1, 1,
                                            &m_colorDescriptorSets[m_window->currentFrame()],
                                            0, nullptr);
        break;
    default:
        break;
    }

    // Draw the axes
    VkDeviceSize axisOffset = 0;
    m_devFuncs->vkCmdBindVertexBuffers(cmdBuf, 0, 1, &m_axisBuffer, &axisOffset);

    // Bind pipeline
    m_devFuncs->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[1]);

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
    if (m_meshData->format == VertexFormat::Color) {
        poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_concurrentFrameCount });
    }

    // Create the descriptor pool
    VkDescriptorPoolCreateInfo descPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = m_concurrentFrameCount * static_cast<uint32_t>(poolSizes.size()),
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };
    err = m_devFuncs->vkCreateDescriptorPool(m_device, &descPoolInfo, nullptr, &m_descPool);
    if (err != VK_SUCCESS) qFatal("Failed to create descriptor pool: %d", err);
}

void VulkanRenderer::createDescriptorSets() {

    // Create descriptor set for the uniform buffer
    VkDescriptorSetLayoutBinding uniformBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr
    };
    VkDescriptorSetLayoutCreateInfo globalLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &uniformBinding
    };
    VkResult err = m_devFuncs->vkCreateDescriptorSetLayout(m_device, &globalLayoutInfo, nullptr, &m_globalDescSetLayout);
    if (err != VK_SUCCESS) qFatal("Failed to create global descriptor set layout");

    std::vector<VkDescriptorSetLayout> globalLayouts(m_concurrentFrameCount, m_globalDescSetLayout);
    VkDescriptorSetAllocateInfo globalAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_descPool,
        .descriptorSetCount = m_concurrentFrameCount,
        .pSetLayouts = globalLayouts.data()
    };
    err = m_devFuncs->vkAllocateDescriptorSets(m_device, &globalAllocInfo, m_globalDescSet.data());
    if (err != VK_SUCCESS) qFatal("Failed to allocate global descriptor sets");

    std::vector<VkWriteDescriptorSet> globalDescWrites(m_concurrentFrameCount);
    for (uint32_t i = 0; i < m_concurrentFrameCount; ++i) {
        globalDescWrites[i] = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_globalDescSet[i],
            .dstBinding = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &m_uniformBufferInfo[i]
        };
    }
    m_devFuncs->vkUpdateDescriptorSets(m_device, m_concurrentFrameCount, globalDescWrites.data(), 0, nullptr);

    // Descriptor set for the sampler/texture
    if (m_meshData->format == VertexFormat::Color) {
        VkDescriptorSetLayoutBinding samplerBinding = {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        };
        VkDescriptorSetLayoutCreateInfo colorLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &samplerBinding
        };
        err = m_devFuncs->vkCreateDescriptorSetLayout(m_device, &colorLayoutInfo, nullptr, &m_colorDescSetLayout);
        if (err != VK_SUCCESS) qFatal("Failed to create color descriptor set layout");

        std::vector<VkDescriptorSetLayout> colorLayouts(m_concurrentFrameCount, m_colorDescSetLayout);
        VkDescriptorSetAllocateInfo colorAllocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descPool,
            .descriptorSetCount = m_concurrentFrameCount,
            .pSetLayouts = colorLayouts.data()
        };
        err = m_devFuncs->vkAllocateDescriptorSets(m_device, &colorAllocInfo, m_colorDescriptorSets.data());
        if (err != VK_SUCCESS) qFatal("Failed to allocate color descriptor sets");

        std::vector<VkWriteDescriptorSet> colorDescWrites(m_concurrentFrameCount);
        for (uint32_t i = 0; i < m_concurrentFrameCount; ++i) {
            colorDescWrites[i] = {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_colorDescriptorSets[i],
                .dstBinding = 1,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &m_colorMapImageInfo
            };
        }
        m_devFuncs->vkUpdateDescriptorSets(m_device, m_concurrentFrameCount, colorDescWrites.data(), 0, nullptr);
    }
}

void VulkanRenderer::createPipelineLayouts() {
    VkResult err;

    // Configure push constants
    VkPushConstantRange range = {
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = 4 * sizeof(float)
    };

    // Flat Layout
    if (m_meshData->format == VertexFormat::Flat || m_meshData->format == VertexFormat::Color) {
        VkPipelineLayoutCreateInfo flatPipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &m_globalDescSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &range
        };
        err = m_devFuncs->vkCreatePipelineLayout(m_device, &flatPipelineLayoutInfo, nullptr, &m_flatPipelineLayout);
        if (err != VK_SUCCESS) qFatal("Failed to create flat pipeline layout: %d", err);
    }

    // Color Layout
    if (m_meshData->format == VertexFormat::Color) {
        std::array<VkDescriptorSetLayout, 2> colorSetLayouts = { m_globalDescSetLayout, m_colorDescSetLayout };
        VkPipelineLayoutCreateInfo colorPipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = static_cast<uint32_t>(colorSetLayouts.size()),
            .pSetLayouts = colorSetLayouts.data(),
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &range
        };
        err = m_devFuncs->vkCreatePipelineLayout(m_device, &colorPipelineLayoutInfo, nullptr, &m_colorPipelineLayout);
        if (err != VK_SUCCESS) qFatal("Failed to create color pipeline layout: %d", err);
    }
}

void VulkanRenderer::createSharedPipelineStructs() {

    static const VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    m_dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates
    };

    m_viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    m_multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = m_window->sampleCountFlagBits()
    };

    m_depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
    };

    static const VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    m_colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };
}

// Create pipeline info for flat rendering
VkGraphicsPipelineCreateInfo VulkanRenderer::createFlatPipelineInfo() {

    // Set vertex binding and attributes
    static const VkVertexInputBindingDescription flatVertexBindingDesc = {
        .binding = 0,
        .stride = 3 * sizeof(float),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    static const VkVertexInputAttributeDescription flatVertexAttrDesc = {
        .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0
    };

    m_flatVertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &flatVertexBindingDesc,
        .vertexAttributeDescriptionCount = 1,
        .pVertexAttributeDescriptions = &flatVertexAttrDesc
    };

    m_flatShaderModules[0] = createShader(QStringLiteral(":/shaders/flat/vert.spv"));
    m_flatShaderModules[1] = createShader(QStringLiteral(":/shaders/flat/frag.spv"));

    m_flatShaderStages[0] = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = m_flatShaderModules[0],
        .pName = "main"
    };
    m_flatShaderStages[1] = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = m_flatShaderModules[1],
        .pName = "main"
    };

    m_flatAssemblyState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    // Rasterization: Filled polygons
    m_flatRasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.0f
    };

    return {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = m_flatShaderStages.data(),
        .pVertexInputState = &m_flatVertexInputInfo,
        .pInputAssemblyState = &m_flatAssemblyState,
        .pViewportState = &m_viewportState,
        .pRasterizationState = &m_flatRasterizer,
        .pMultisampleState = &m_multisampling,
        .pDepthStencilState = &m_depthStencil,
        .pColorBlendState = &m_colorBlending,
        .pDynamicState = &m_dynamicState,
        .layout = m_flatPipelineLayout,
        .renderPass = m_window->defaultRenderPass()
    };
}

// Create pipeline info for flat rendering
VkGraphicsPipelineCreateInfo VulkanRenderer::createAxisPipelineInfo() {

    // Set axis binding and attributes
    static const VkVertexInputBindingDescription axisVertexBindingDesc = {
        .binding = 0,
        .stride = 6 * sizeof(float),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    static const VkVertexInputAttributeDescription axisVertexAttrDescs[2] = {
        { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0 },
        { .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 3 * sizeof(float) }
    };

    m_axisVertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &axisVertexBindingDesc,
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions = axisVertexAttrDescs
    };

    m_axisShaderModules[0] = createShader(QStringLiteral(":/shaders/axis/vert.spv"));
    m_axisShaderModules[1] = createShader(QStringLiteral(":/shaders/axis/frag.spv"));

    m_axisShaderStages[0] = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = m_axisShaderModules[0],
        .pName = "main"
    };
    m_axisShaderStages[1] = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = m_axisShaderModules[1],
        .pName = "main"
    };

    m_axisAssemblyState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST
    };

    // Rasterization
    m_axisRasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 2.0f
    };

    return {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = m_axisShaderStages.data(),
        .pVertexInputState = &m_axisVertexInputInfo,
        .pInputAssemblyState = &m_axisAssemblyState,
        .pViewportState = &m_viewportState,
        .pRasterizationState = &m_axisRasterizer,
        .pMultisampleState = &m_multisampling,
        .pDepthStencilState = &m_depthStencil,
        .pColorBlendState = &m_colorBlending,
        .pDynamicState = &m_dynamicState,
        .layout = m_flatPipelineLayout,
        .renderPass = m_window->defaultRenderPass()
    };
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
    VkResult err = m_devFuncs->vkCreateShaderModule(m_window->device(), &shaderInfo, nullptr, &shaderModule);
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
    if (m_flatPipelineLayout != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyPipelineLayout(m_device, m_flatPipelineLayout, nullptr);
        m_flatPipelineLayout = VK_NULL_HANDLE;
    }
    if (m_colorPipelineLayout != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyPipelineLayout(m_device, m_colorPipelineLayout, nullptr);
        m_colorPipelineLayout = VK_NULL_HANDLE;
    }
    if (m_pipelineCache != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyPipelineCache(m_device, m_pipelineCache, nullptr);
        m_pipelineCache = VK_NULL_HANDLE;
    }

    // Destroy descriptor objects
    if (m_globalDescSetLayout != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyDescriptorSetLayout(m_device, m_globalDescSetLayout, nullptr);
        m_globalDescSetLayout = VK_NULL_HANDLE;
    }
    if (m_colorDescSetLayout != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyDescriptorSetLayout(m_device, m_colorDescSetLayout, nullptr);
        m_colorDescSetLayout = VK_NULL_HANDLE;
    }
    if (m_descPool != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyDescriptorPool(m_device, m_descPool, nullptr);
        m_descPool = VK_NULL_HANDLE;
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

    // Unmap/destroy uniform buffer
    if (m_uniformBufferMemory != VK_NULL_HANDLE) {
        m_devFuncs->vkUnmapMemory(m_device, m_uniformBufferMemory);
        m_devFuncs->vkFreeMemory(m_device, m_uniformBufferMemory, nullptr);
        m_uniformBufferMemory = VK_NULL_HANDLE;
    }
    if (m_uniformBuffer != VK_NULL_HANDLE) {
        m_devFuncs->vkDestroyBuffer(m_device, m_uniformBuffer, nullptr);
        m_uniformBuffer = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::initSwapChainResources() {
    // Get new dimensions
    QSize sz = m_window->swapChainImageSize();

    // 1. Protect against division by zero during tab switching/layout recalculation
    float aspect = 1.0f;
    if (sz.height() > 0 && sz.width() > 0) {
        aspect = static_cast<float>(sz.width()) / static_cast<float>(sz.height());
    }

    // Build the projection matrix
    QMatrix4x4 projMatrix = m_window->clipCorrectionMatrix();
    projMatrix.perspective(45.0f, aspect, 0.001f, 1000.0f);

    // Update the window
    m_window->setProjMatrix(projMatrix);
}

void VulkanRenderer::releaseSwapChainResources() {}
