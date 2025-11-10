#pragma once

#include <vulkan/vulkan_core.h>

#include "glm_config.h"

#include "context.h"
#include "buffer.h"
#include "texture.h"

#include "shadow_map.h"

class LightningPass {
public:
    LightningPass(const VkFormat colorFormat,
                  const VkFormat depthFormat,
                  const uint32_t pushConstantStart,
                  VkExtent2D     extent);

    bool Create(Context& context, Texture& shadowMap);
    void Destroy(Context& context);
    void BeginPass(const VkCommandBuffer cmdBuffer);
    void EndPass(const VkCommandBuffer cmdBuffer);

    void BuildPipeline(const VkDevice device, const VkPipelineLayout pipelineLayout);

    void Destroy(const VkDevice device);

    VkPipeline SimplePipeline() const { return m_simplePipeline; }
    VkPipeline ShadowMapPipeline() const { return m_shadowMapPipeline; }

    Texture& colorOutput() { return m_colorOutput; }

    void updateLightInfo(Context& context, DirectionalLight& lightInfo);

private:
    void TransitionForRender(const VkCommandBuffer cmdBuffer);
    void TransitionForRead(const VkCommandBuffer cmdBuffer);

    VkFormat         m_colorFormat;
    VkFormat         m_depthFormat;
    uint32_t         m_pushConstStart;
    VkExtent2D       m_extent;
    VkPipelineLayout m_pipelineLayout    = VK_NULL_HANDLE;
    VkPipeline       m_simplePipeline    = VK_NULL_HANDLE;
    VkPipeline       m_shadowMapPipeline = VK_NULL_HANDLE;

    VkDescriptorSet m_lightSet;
    BufferInfo      m_lightBuffer;

    Texture m_colorOutput;
    Texture m_depthOutput;
};
