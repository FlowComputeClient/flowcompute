#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

#include <QFile>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVulkanFunctions>
#include <QVulkanWindow>

#include "../../../geometry/mesh_data.h"

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
    VulkanRenderer(VulkanWindow *w, std::shared_ptr<MeshData> meshData);

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

    // Called by initResources
    void createVertexBuffer();
    void createIndexBuffer();
    void createAxisBuffer();
    void createUniformBuffer();
    void createTexture();
    void createPipelineStorage();
    void createDescriptorSets();
    void createPipelineLayouts();
    void createSharedPipelineStructs();
    void createPipelines();
    VkShaderModule createShader(const QString &name);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkGraphicsPipelineCreateInfo createFlatPipelineInfo();
    VkGraphicsPipelineCreateInfo createAxisPipelineInfo();

    // Vertex data
    std::shared_ptr<MeshData> m_meshData;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_axisBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_axisBuffer = VK_NULL_HANDLE;

    // Uniform data
    VkDeviceMemory m_uniformBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_uniformBuffer = VK_NULL_HANDLE;
    std::array<VkDescriptorBufferInfo, QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT> m_uniformBufferInfo;
    VkDeviceSize m_alignedUniformSize = 0;
    void* m_mappedUniformData = nullptr;

    // Descriptor pool objects
    VkDescriptorPool m_descPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_globalDescSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_colorDescSetLayout = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT> m_globalDescSet;
    std::array<VkDescriptorSet, QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT> m_colorDescriptorSets;

    // Texture data
    VkDescriptorImageInfo m_colorMapImageInfo;

    // Pipeline structs
    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineDynamicStateCreateInfo m_dynamicState;
    VkPipelineViewportStateCreateInfo m_viewportState;
    VkPipelineMultisampleStateCreateInfo m_multisampling;
    VkPipelineDepthStencilStateCreateInfo m_depthStencil;
    VkPipelineColorBlendStateCreateInfo m_colorBlending;

    // Structures for the flat pipeline
    VkPipelineRasterizationStateCreateInfo m_flatRasterizer;
    VkPipelineInputAssemblyStateCreateInfo m_flatAssemblyState;
    VkPipelineVertexInputStateCreateInfo m_flatVertexInputInfo;
    std::array<VkPipelineShaderStageCreateInfo, 2> m_flatShaderStages;
    std::array<VkShaderModule, 2> m_flatShaderModules;
    VkPipelineLayout m_flatPipelineLayout = VK_NULL_HANDLE;

    // Structures for the axis pipeline
    VkPipelineRasterizationStateCreateInfo m_axisRasterizer;
    VkPipelineInputAssemblyStateCreateInfo m_axisAssemblyState;
    VkPipelineVertexInputStateCreateInfo m_axisVertexInputInfo;
    std::array<VkPipelineShaderStageCreateInfo, 2> m_axisShaderStages;
    std::array<VkShaderModule, 2> m_axisShaderModules;
    VkPipelineLayout m_axisPipelineLayout = VK_NULL_HANDLE;

    // Pipeline objects
    VkPipelineLayout m_colorPipelineLayout = VK_NULL_HANDLE;
    std::vector<VkPipeline> m_pipelines;
    int m_current_format = 0;
};

#endif // VULKAN_RENDERER_H
