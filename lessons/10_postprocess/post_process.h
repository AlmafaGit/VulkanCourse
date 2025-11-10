#pragma once

#include <vulkan/vulkan_core.h>

#include "context.h"
#include "descriptors.h"
#include "texture.h"

class PostProcessPass {
public:
    struct PostProcessOptions {
        uint32_t mode = 0;
    } options;

    PostProcessPass(VkFormat colorFormat, VkExtent2D extent);

    bool Create(Context& context);
    void Destroy(Context& context);

    void BeginPass(const VkCommandBuffer cmdBuffer, VkImageView colorOutputView);
    void Draw(const VkCommandBuffer cmdBuffer);
    void EndPass(const VkCommandBuffer cmdBuffer);

    void BindInputImage(const VkDevice device, const Texture& texture);

    VkPipeline          Pipeline() const { return m_pipeline; }
    VkPipelineLayout    PipelineLayout() const { return m_pipelineLayout; }

private:
    VkFormat            m_colorFormat       = {};
    VkExtent2D          m_extent            = {};

    VkDescriptorSet     m_descSet           = VK_NULL_HANDLE;
    VkPipelineLayout    m_pipelineLayout    = VK_NULL_HANDLE;
    VkPipeline          m_pipeline          = VK_NULL_HANDLE;
};
