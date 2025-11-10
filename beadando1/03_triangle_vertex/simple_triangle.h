#pragma once

#include <vulkan/vulkan_core.h>

#include "buffer.h"
#include "glm_config.h"

class Context;

class SimpleTriangle {
public:
    struct PushConstant {
        glm::mat4 mvp;
    };

    SimpleTriangle();

    VkResult Create(const Context& context, const VkFormat colorFormat);
    void     Destroy(const VkDevice device);
    void     Draw(const VkCommandBuffer cmdBuffer);

private:
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline       m_pipeline       = VK_NULL_HANDLE;
    PushConstant     m_constantData   = {};
    BufferInfo       m_buffer         = {};
};
