// Copyright 2026 FlowCompute LLC
//
// This file is part of FlowCompute.
//
// FlowCompute is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// FlowCompute is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with FlowCompute. If not, see <https://www.gnu.org/licenses/>.

#ifndef EDITORS_GRAPHICAL_VULKAN_VULKAN_RENDERER_H_
#define EDITORS_GRAPHICAL_VULKAN_VULKAN_RENDERER_H_

#include <QFile>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVulkanFunctions>
#include <QVulkanWindow>

#include <memory>
#include <vector>

#include "geometry/graphic_data.h"

class VulkanWindow;

struct Vertex {
    float position[3];
    float normal[3];
};

struct UniformBufferObject {
    float model[16];
    float view[16];
    float proj[16];
};

static const int UNIFORM_DATA_SIZE = 16 * sizeof(float);
static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign) {
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

class VulkanRenderer : public QVulkanWindowRenderer {
 public:
    VulkanRenderer(VulkanWindow *w, std::shared_ptr<RenderData> meshData);

    // Vulkan functions
    void initResources() override;
    void startNextFrame() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;

 protected:
    // Fundamental objects
    VulkanWindow *m_window;
    QVulkanDeviceFunctions *m_devFuncs;
    VkDevice m_device;

    // Keep track of how many vertices are currently active
    uint32_t m_currentVertexCount = 0;
    uint32_t m_concurrentFrameCount;
    std::vector<bool> m_uboDirty;

    // Maximum dimension
    float m_maxDim;

    // Called by initResources
    void createVertexBuffer();
    void createIndexBuffer();
    void createAxisBuffer();
    void createUniformBuffer();
    void createTexture();
    void createPipelineStorage();
    void createDescriptorSets();
    void createPipelines();
    VkShaderModule createShader(const QString &name);
    uint32_t findMemoryType(uint32_t typeFilter,
                            VkMemoryPropertyFlags properties);

    // Vertex/index buffers
    std::shared_ptr<RenderData> m_renderData;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_axisBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_axisBuffer = VK_NULL_HANDLE;

    // Uniform buffer data
    VkDeviceMemory m_uniformBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_uniformBuffer = VK_NULL_HANDLE;
    std::array<VkDescriptorBufferInfo,
               QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT> m_uniformBufferInfo;
    VkDeviceSize m_alignedUniformSize = 0;
    void* m_mappedUniformData = nullptr;

    // Descriptor pool objects
    VkDescriptorPool m_descPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    std::array<VkDescriptorSet,
               QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT> m_descriptorSets;

    // Texture data
    VkImage m_textureImage = VK_NULL_HANDLE;
    VkDeviceMemory m_textureImageMemory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkDescriptorImageInfo m_colorMapImageInfo;
    VkSampler m_sampler = VK_NULL_HANDLE;

    // Pipeline structs
    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    std::array<VkPipeline, 2> m_pipelines = { VK_NULL_HANDLE, VK_NULL_HANDLE };

 private:
    std::array<float, 3> m_clearColor;
};

#endif  // EDITORS_GRAPHICAL_VULKAN_VULKAN_RENDERER_H_
