#include "vulkan_renderer.h"

#include "vulkan_window.h"

// Create vertex and index buffers
void VulkanRenderer::createVertexIndexBuffers() {

    // Check mesh data
    if (!m_meshData || m_meshData->data.empty() || m_meshData->indices.empty()) {
        qWarning("Mesh data is empty. Skipping buffer creation.");
        return;
    }

    // Create vertex buffer
    VkDeviceSize vertexBufferSize = m_meshData->data.size() * sizeof(m_meshData->data[0]);
    VkBufferCreateInfo vertexBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = vertexBufferSize,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    VkResult err = m_devFuncs->vkCreateBuffer(m_device, &vertexBufferInfo, nullptr, &m_vertexBuffer);
    if (err != VK_SUCCESS) qFatal("Failed to create vertex buffer: %d", err);

    VkMemoryRequirements vertexMemReqs;
    m_devFuncs->vkGetBufferMemoryRequirements(m_device, m_vertexBuffer, &vertexMemReqs);

    VkMemoryAllocateInfo vertexAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = vertexMemReqs.size,
        .memoryTypeIndex = findMemoryType(vertexMemReqs.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    err = m_devFuncs->vkAllocateMemory(m_device, &vertexAllocInfo, nullptr, &m_vertexBufferMemory);
    if (err != VK_SUCCESS) qFatal("Failed to allocate vertex memory: %d", err);
    m_devFuncs->vkBindBufferMemory(m_device, m_vertexBuffer, m_vertexBufferMemory, 0);

    // Copy vertex data
    void* vertexData;
    err = m_devFuncs->vkMapMemory(m_device, m_vertexBufferMemory, 0, vertexBufferSize, 0, &vertexData);
    if (err != VK_SUCCESS) qFatal("Failed to map vertex memory: %d", err);
    memcpy(vertexData, m_meshData->data.data(), (size_t)vertexBufferSize);
    m_devFuncs->vkUnmapMemory(m_device, m_vertexBufferMemory);

    // Create index buffer
    VkDeviceSize indexBufferSize = m_meshData->indices.size() * sizeof(m_meshData->indices[0]);
    VkBufferCreateInfo indexBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = indexBufferSize,
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    err = m_devFuncs->vkCreateBuffer(m_device, &indexBufferInfo, nullptr, &m_indexBuffer);
    if (err != VK_SUCCESS) qFatal("Failed to create index buffer: %d", err);

    VkMemoryRequirements indexMemReqs;
    m_devFuncs->vkGetBufferMemoryRequirements(m_device, m_indexBuffer, &indexMemReqs);

    VkMemoryAllocateInfo indexAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = indexMemReqs.size,
        .memoryTypeIndex = findMemoryType(indexMemReqs.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    err = m_devFuncs->vkAllocateMemory(m_device, &indexAllocInfo, nullptr, &m_indexBufferMemory);
    if (err != VK_SUCCESS) qFatal("Failed to allocate index memory: %d", err);
    m_devFuncs->vkBindBufferMemory(m_device, m_indexBuffer, m_indexBufferMemory, 0);

    // Copy index data
    void* indexData;
    err = m_devFuncs->vkMapMemory(m_device, m_indexBufferMemory, 0, indexBufferSize, 0, &indexData);
    if (err != VK_SUCCESS) qFatal("Failed to map index memory: %d", err);
    memcpy(indexData, m_meshData->indices.data(), (size_t)indexBufferSize);
    m_devFuncs->vkUnmapMemory(m_device, m_indexBufferMemory);

    // Create axis buffer
    std::array<float, 36> axisData = {
        0.0, 0.0, 0.0, 1.0, 0.35, 0.35, 1.0, 0.0, 0.0, 1.0, 0.35, 0.35,
        0.0, 0.0, 0.0, 0.35, 1.0, 0.35, 0.0, 1.0, 0.0, 0.35, 1.0, 0.35,
        0.0, 0.0, 0.0, 0.4, 0.65, 1.0, 0.0, 0.0, 1.0, 0.4, 0.65, 1.0 };
    VkDeviceSize axisBufferSize = axisData.size() * sizeof(axisData[0]);
    VkBufferCreateInfo axisBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = axisBufferSize,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };
    err = m_devFuncs->vkCreateBuffer(m_device, &axisBufferInfo, nullptr, &m_axisBuffer);
    if (err != VK_SUCCESS) qFatal("Failed to create vertex buffer: %d", err);

    VkMemoryRequirements axisMemReqs;
    m_devFuncs->vkGetBufferMemoryRequirements(m_device, m_axisBuffer, &axisMemReqs);

    VkMemoryAllocateInfo axisAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = axisMemReqs.size,
        .memoryTypeIndex = findMemoryType(axisMemReqs.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    err = m_devFuncs->vkAllocateMemory(m_device, &axisAllocInfo, nullptr, &m_axisBufferMemory);
    if (err != VK_SUCCESS) qFatal("Failed to allocate vertex memory: %d", err);
    m_devFuncs->vkBindBufferMemory(m_device, m_axisBuffer, m_axisBufferMemory, 0);

    // Copy axis data
    void* axisCopyData;
    err = m_devFuncs->vkMapMemory(m_device, m_axisBufferMemory, 0, axisBufferSize, 0, &axisCopyData);
    if (err != VK_SUCCESS) qFatal("Failed to map axis memory: %d", err);
    memcpy(axisCopyData, axisData.data(), (size_t)axisBufferSize);
    m_devFuncs->vkUnmapMemory(m_device, m_axisBufferMemory);
}

// Create uniform buffer
void VulkanRenderer::createUniformBuffer() {

    // Get the device's minimum uniform buffer alignment
    VkPhysicalDeviceProperties physDevProps;
    m_window->vulkanInstance()->functions()->vkGetPhysicalDeviceProperties(m_window->physicalDevice(), &physDevProps);
    VkDeviceSize minAlignment = physDevProps.limits.minUniformBufferOffsetAlignment;

    // Set buffer size
    VkDeviceSize uboSize = sizeof(UniformBufferObject);

    // Bitwise trick to round up to the nearest multiple of minAlignment
    m_alignedUniformSize = (uboSize + minAlignment - 1) & ~(minAlignment - 1);

    // Total buffer size for all concurrent frames
    VkDeviceSize totalBufferSize = m_alignedUniformSize * m_concurrentFrameCount;

    // Create the logical buffer
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = totalBufferSize,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VkResult err = m_devFuncs->vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_uniformBuffer);
    if (err != VK_SUCCESS) qFatal("Failed to create uniform buffer: %d", err);

    // Query memory requirements
    VkMemoryRequirements memReqs;
    m_devFuncs->vkGetBufferMemoryRequirements(m_device, m_uniformBuffer, &memReqs);

    // Allocate physical memory
    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memReqs.size,
        .memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };

    err = m_devFuncs->vkAllocateMemory(m_device, &allocInfo, nullptr, &m_uniformBufferMemory);
    if (err != VK_SUCCESS) qFatal("Failed to allocate uniform memory: %d", err);

    // Bind memory to buffer
    m_devFuncs->vkBindBufferMemory(m_device, m_uniformBuffer, m_uniformBufferMemory, 0);

    // Persistently map the memory
    err = m_devFuncs->vkMapMemory(m_device, m_uniformBufferMemory, 0, VK_WHOLE_SIZE, 0, &m_mappedUniformData);
    if (err != VK_SUCCESS) qFatal("Failed to map uniform memory: %d", err);

    // Populate the descriptor buffer infos for createPipelineLayout()
    for (uint32_t i = 0; i < m_concurrentFrameCount; ++i) {
        m_uniformBufferInfo[i] = {
            .buffer = m_uniformBuffer,
            .offset = i * m_alignedUniformSize,
            .range = uboSize
        };
    }
}

// Initialize texture
void VulkanRenderer::createTexture() {

    // Create color map
    QImage colormap(":/images/viridis_map.png");
    if (colormap.isNull()) qFatal("Failed to load colormap image");
    colormap = colormap.convertToFormat(QImage::Format_RGBA8888);
    VkDeviceSize imageSize = colormap.sizeInBytes();

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = imageSize,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    };
    m_devFuncs->vkCreateBuffer(m_device, &bufferInfo, nullptr, &stagingBuffer);

    // Allocate memory
    VkMemoryRequirements memRequirements;
    m_devFuncs->vkGetBufferMemoryRequirements(m_device, stagingBuffer, &memRequirements);
    VkMemoryAllocateInfo memAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    };
    m_devFuncs->vkAllocateMemory(m_device, &memAllocInfo, nullptr, &stagingBufferMemory);
    m_devFuncs->vkBindBufferMemory(m_device, stagingBuffer, stagingBufferMemory, 0);

    // Copy data to staging buffer
    void* data;
    m_devFuncs->vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, colormap.constBits(), static_cast<size_t>(imageSize));
    m_devFuncs->vkUnmapMemory(m_device, stagingBufferMemory);

    // Create Texture Image
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageCreateInfo imageInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_1D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = { static_cast<uint32_t>(colormap.width()), 1, 1 },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };
    m_devFuncs->vkCreateImage(m_device, &imageInfo, nullptr, &textureImage);

    // Allocate GPU memory for the Image
    m_devFuncs->vkGetImageMemoryRequirements(m_device, textureImage, &memRequirements);
    memAllocInfo.allocationSize = memRequirements.size;
    memAllocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_devFuncs->vkAllocateMemory(m_device, &memAllocInfo, nullptr, &textureImageMemory);
    m_devFuncs->vkBindImageMemory(m_device, textureImage, textureImageMemory, 0);

    // Create command pool
    VkCommandPool commandPool;
    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = m_window->graphicsQueueFamilyIndex()
    };
    m_devFuncs->vkCreateCommandPool(m_device, &poolInfo, nullptr, &commandPool);

    // Allocate info for command buffer
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    m_devFuncs->vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    // Start recording commands
    m_devFuncs->vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Transition image layout for Copying
    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = textureImage,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1 }
    };

    m_devFuncs->vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Configure copy operation
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = { static_cast<uint32_t>(colormap.width()), 1, 1 };

    // Copy data from staging buffer to image
    m_devFuncs->vkCmdCopyBufferToImage(commandBuffer,
        stagingBuffer, textureImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &region);

    // 3. Transition Image Layout for Shader Reading
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    m_devFuncs->vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    // End command buffer
    m_devFuncs->vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };

    // Submit command to queue
    VkQueue graphicsQueue = m_window->graphicsQueue();
    m_devFuncs->vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    m_devFuncs->vkQueueWaitIdle(graphicsQueue);

    // Free resources
    m_devFuncs->vkFreeCommandBuffers(m_device, commandPool, 1, &commandBuffer);
    m_devFuncs->vkDestroyCommandPool(m_device, commandPool, nullptr); // Added: Don't forget to destroy the pool!
    m_devFuncs->vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    m_devFuncs->vkFreeMemory(m_device, stagingBufferMemory, nullptr);

    // Create the image view
    VkImageView imageView;
    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = textureImage,
        .viewType = VK_IMAGE_VIEW_TYPE_1D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0, .levelCount = 1,
            .baseArrayLayer = 0, .layerCount = 1
        }
    };
    m_devFuncs->vkCreateImageView(m_device, &viewInfo, nullptr, &imageView);

    // Create the sampler
    VkSampler sampler;
    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .anisotropyEnable = VK_FALSE,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE
    };
    m_devFuncs->vkCreateSampler(m_device, &samplerInfo, NULL, &sampler);

    // Create the descriptor image
    m_colorMapImageInfo = {
        .sampler = sampler,
        .imageView = imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
}