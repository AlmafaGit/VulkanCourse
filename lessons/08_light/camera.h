#pragma once

#include <vulkan/vulkan_core.h>

#include "glm_config.h"

class Camera {
public:
    struct CameraPushConstant {
        glm::vec4 position;
        glm::mat4 projection;
        glm::mat4 view;
    };

    Camera(VkExtent2D viewport, float fov = 45.0f, float nearPlane = 0.1f, float farPlane = 100.0f)
        : m_aspectRatio(viewport.width / (float)viewport.height)
        , m_projection(glm::perspective(glm::radians(fov), m_aspectRatio, nearPlane, farPlane))
        , m_yaw(90.0f)
        , m_pitch(-10.0f)
        , m_position(glm::vec3(0.0f, 1.0f, -3.0f))
        , m_front(glm::vec3(0.0f, 1.0f, 1.0f))
        , m_worldUp(glm::vec3(0.0f, -1.0f, 0.0f))
        , m_view(glm::mat4(1.0f))
    {
    }

    void Forward() { m_position += CAMERA_SPEED * m_front; }
    void Back() { m_position -= CAMERA_SPEED * m_front; }
    void Left() { m_position -= glm::normalize(glm::cross(m_front, m_worldUp)) * CAMERA_SPEED; }
    void Right() { m_position += glm::normalize(glm::cross(m_front, m_worldUp)) * CAMERA_SPEED; }

    void ProcessMouseMovement(float offsetX, float offsetY)
    {
        m_yaw -= offsetX * SENSITIVITY;
        m_pitch -= offsetY * SENSITIVITY;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        m_pitch = std::min(m_pitch, 89.0f);
        m_pitch = std::max(-89.0f, m_pitch);

        Update();
    }

    void Update()
    {
        const float yawRadians   = glm::radians(m_yaw);
        const float pitchRadians = glm::radians(m_pitch);

        m_target = m_position + m_front;

        // calculate the new Front vector
        const glm::vec3 front(cos(yawRadians) * cos(pitchRadians), sin(pitchRadians),
                              sin(yawRadians) * cos(pitchRadians));
        m_front = glm::normalize(front);

        // also re-calculate the Right and Up vector
        const glm::vec3 right = glm::normalize(glm::cross(m_front, m_worldUp));
        // normalize the vectors, because their length gets closer to 0 the more you
        // look up or down which results in slower movement.
        const glm::vec3 up = glm::normalize(glm::cross(right, m_front));

        m_view = glm::lookAt(m_position, m_target, up);
    }

    const glm::vec3& position() const { return m_position; }
    const glm::vec3& lookAtPosition() const { return m_target; }
    const glm::mat4& projection() const { return m_projection; };
    const glm::mat4& view() const { return m_view; };

    void CreateVK(const VkDevice device)
    {

    }

    void Destroy(VkDevice device) {
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
    }

private:
    const float SENSITIVITY  = 0.1f;
    const float CAMERA_SPEED = 2.5f * 0.05f;

    float     m_aspectRatio;
    glm::mat4 m_projection;

    float     m_yaw;
    float     m_pitch;
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_worldUp;

    glm::vec3 m_target;
    glm::mat4 m_view;

    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
};
