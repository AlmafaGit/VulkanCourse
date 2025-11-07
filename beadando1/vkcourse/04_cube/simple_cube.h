#pragma once

#include <vulkan/vulkan_core.h>

#include "buffer.h"
#include "glm/fwd.hpp"
#include "glm_config.h"

class Context;

class SimpleCube {
public:
    struct PushConstant {
        glm::mat4 mvp;
    };

    SimpleCube();

    VkResult Create(const Context& context, const VkFormat colorFormat);
    void     Destroy(const VkDevice device);
    void     Draw(const VkCommandBuffer cmdBuffer);

    void projection(const glm::mat4& projection) { m_projection = projection; }
    void view(const glm::mat4& view) { m_view = view; }
    void position(const glm::mat4& position) { m_position = position; }
    void rotation(const glm::mat4& rotation) { m_rotation = rotation; }

private:
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline       m_pipeline       = VK_NULL_HANDLE;
    PushConstant     m_constantData   = {};
    BufferInfo       m_buffer         = {};
    uint32_t         m_vertexCount    = 0;
    glm::mat4        m_projection     = glm::mat4(1.0f);
    glm::mat4        m_view           = glm::mat4(1.0f);
    glm::mat4        m_position       = glm::mat4(1.0f);
    glm::mat4        m_rotation       = glm::mat4(1.0f);
};
