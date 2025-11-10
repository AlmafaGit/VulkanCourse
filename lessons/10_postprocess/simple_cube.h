#pragma once

#include <vulkan/vulkan_core.h>

#include "glm_config.h"
#include "buffer.h"


class Context;

class SimpleCube {
public:
    struct ModelPushConstant {
        glm::mat4 model;
    };

    SimpleCube();

    VkResult Create(const Context& context, const VkFormat colorFormat, const uint32_t pushConstantStart);
    void     Destroy(const VkDevice device);
    void     Draw(const VkCommandBuffer cmdBuffer, bool bindPipeline = true);

    void position(const glm::mat4& position) { m_position = position; }
    void rotation(const glm::mat4& rotation) { m_rotation = rotation; }

private:
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline       m_pipeline       = VK_NULL_HANDLE;
    uint32_t         m_constantOffset = 0;
    BufferInfo       m_buffer         = {};
    uint32_t         m_vertexCount    = 0;
    glm::mat4        m_position       = glm::mat4(1.0f);
    glm::mat4        m_rotation       = glm::mat4(1.0f);
};
