#pragma once

#include <vulkan/vulkan_core.h>

#include "glm_config.h"
#include "context.h"
#include "texture.h"

struct DirectionalLight {
    glm::vec4 position;
    glm::mat4 projection;
    glm::mat4 view;
};

class ShadowMap {
public:
    ShadowMap(const VkFormat depthFormat, const uint32_t pushConstantStart, VkExtent2D extent);

    bool Create(Context& context);
    void Destroy(Context& context);
    void BeginPass(const VkCommandBuffer cmdBuffer);
    void EndPass(const VkCommandBuffer cmdBuffer);

    bool BuildPipeline(const VkDevice device, const VkPipelineLayout pipelineLayout);

    VkExtent2D Extent() const { return m_extent; }
    uint32_t   Width() const { return m_extent.width; }
    uint32_t   Height() const { return m_extent.height; }

    VkPipeline Pipeline() const { return m_pipeline; }
    Texture&   Depth() { return m_shadowDepth; }

    void updateLightInfo(const VkCommandBuffer cmdBuffer, DirectionalLight& lightInfo);

private:
    void TransitionForRender(const VkCommandBuffer cmdBuffer);
    void TransitionForRead(const VkCommandBuffer cmdBuffer);

    VkFormat m_depthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;

    VkExtent2D       m_extent            = {0, 0};
    VkPipelineLayout m_pipelineLayout    = VK_NULL_HANDLE;
    VkPipeline       m_pipeline          = VK_NULL_HANDLE;
    uint32_t         m_pushConstantStart = 0;
    Texture          m_shadowDepth;
};
