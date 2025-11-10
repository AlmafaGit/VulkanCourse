#pragma once

#include <vulkan/vulkan_core.h>

#include "context.h"
#include "swapchain.h"

#include <backends/imgui_impl_glfw.h>

struct GLFWwindow;

class IMGUIIntegration {
public:
    IMGUIIntegration() {}

    bool Init(GLFWwindow* window);
    bool CreateContext(const Context& context, const Swapchain& swapchain);
    void NewFrame();
    void Draw(const VkCommandBuffer cmdBuffer);

    void Destroy(const Context& context);

protected:
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
};
