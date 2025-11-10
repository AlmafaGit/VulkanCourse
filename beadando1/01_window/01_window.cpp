#include <cstdio>
#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_NONE
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include <imgui.h>

#include "context.h"
#include "swapchain.h"
#include "imgui_integration.h"
#include "wrappers.h"

void KeyCallback(GLFWwindow* window, int key, int /*scancode*/, int /*action*/, int /*mods*/)
{
    switch (key) {
    case GLFW_KEY_ESCAPE: {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    }
    }
}

int main(int /*argc*/, char** /*argv*/)
{
    if (glfwVulkanSupported()) {
        printf("Failed to look up minimal Vulkan loader/ICD\n!");
        return -1;
    }

    if (!glfwInit()) {
        printf("Failed to init GLFW!\n");
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    uint32_t     count          = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);

    printf("Minimal set of requred extension by GLFW:\n");
    for (uint32_t idx = 0; idx < count; idx++) {
        printf("-> %s\n", glfwExtensions[idx]);
    }

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + count);

    Context    context("01_window", true);
    VkInstance instance = context.CreateInstance({}, extensions);

    // Create the window to render onto
    uint32_t    windowWidth  = 1024;
    uint32_t    windowHeight = 800;
    GLFWwindow* window       = glfwCreateWindow(windowWidth, windowHeight, "01_window GLFW", NULL, NULL);

    glfwSetWindowUserPointer(window, nullptr);
    glfwSetKeyCallback(window, KeyCallback);

    IMGUIIntegration imIntegration;
    imIntegration.Init(window);

    // We have the window, the instance, create a surface from the window to draw onto.
    // Create a Vulkan Surface using GLFW.
    // By using GLFW the current windowing system's surface is created (xcb, win32, etc..)
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
        // TODO: not the best, but will fail the application surely
        throw std::runtime_error("Failed to create window surface!");
    }

    VkPhysicalDevice phyDevice      = context.SelectPhysicalDevice(surface);
    VkDevice         device         = context.CreateDevice({});
    uint32_t         queueFamilyIdx = context.queueFamilyIdx();
    VkQueue          queue          = context.queue();

    Swapchain swapchain(instance, phyDevice, device, surface, {windowWidth, windowHeight});
    VkResult  swapchainCreated = swapchain.Create();
    assert(swapchainCreated == VK_SUCCESS);

    VkCommandPool cmdPool = VK_NULL_HANDLE;
    CreateCommandPool(device, queueFamilyIdx, &cmdPool); // TODO: check result

    std::vector<VkCommandBuffer> cmdBuffers = AllocateCommandBuffers(device, cmdPool, swapchain.images().size());

    VkFence     imageFence       = CreateFence(device);
    VkSemaphore presentSemaphore = CreateSemaphore(device);

    imIntegration.CreateContext(context, swapchain);

    glfwShowWindow(window);

    int32_t color = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        {
            ImGuiIO& io = ImGui::GetIO();
            imIntegration.NewFrame();
            ImGui::NewFrame();

            ImGui::ShowDemoWindow();
            ImGui::Begin("Info");

            static bool colorAutoInc = true;
            ImGui::Checkbox("Use auto increment", &colorAutoInc);

            if (colorAutoInc) {
                color = (color + 1) % 255;
            }

            ImGui::SliderInt("Red value", &color, 0, 255);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
            ImGui::Render();
        }

        // Get new image to render to
        vkResetFences(device, 1, &imageFence);
        const Swapchain::Image& swapchainImage = swapchain.AquireNextImage(imageFence);
        vkWaitForFences(device, 1, &imageFence, VK_TRUE, UINT64_MAX);

        // Get command buffer based on swapchain image index
        VkCommandBuffer cmdBuffer = cmdBuffers[swapchainImage.idx];
        {
            // Begin command buffer record
            const VkCommandBufferBeginInfo beginInfo = {
                .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext            = nullptr,
                .flags            = 0,
                .pInheritanceInfo = nullptr,
            };
            vkBeginCommandBuffer(cmdBuffer, &beginInfo);

            swapchain.CmdTransitionToRender(cmdBuffer, swapchainImage, queueFamilyIdx);

            // Begin render commands
            const VkClearValue clearColor = {{{color / 255.0f, 0.0f, 0.0f, 1.0f}}};
            const VkRenderingAttachmentInfoKHR colorAttachment = {
                .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                .pNext              = nullptr,
                .imageView          = swapchainImage.view,
                .imageLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .resolveMode        = VK_RESOLVE_MODE_NONE,
                .resolveImageView   = VK_NULL_HANDLE,
                .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue         = clearColor,
            };
            const VkRenderingInfoKHR renderInfo = {
                .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
                .pNext                = nullptr,
                .flags                = 0,
                .renderArea           = {
                    .offset = {0, 0},
                    .extent = {(uint32_t)windowWidth, (uint32_t)windowHeight}
                },
                .layerCount           = 1,
                .viewMask             = 0,
                .colorAttachmentCount = 1,
                .pColorAttachments    = &colorAttachment,
                .pDepthAttachment     = nullptr,
                .pStencilAttachment   = nullptr
            };
            vkCmdBeginRendering(cmdBuffer, &renderInfo);

            // Render things
            imIntegration.Draw(cmdBuffer);

            // End render commands
            vkCmdEndRendering(cmdBuffer);

            // Finish up recording
            swapchain.CmdTransitionToPresent(cmdBuffer, swapchainImage, queueFamilyIdx);
            vkEndCommandBuffer(cmdBuffer);
        }

        // Execute recorded commands
        const VkSubmitInfo submitInfo = {
            .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext                = nullptr,
            .waitSemaphoreCount   = 0,
            .pWaitSemaphores      = nullptr,
            .pWaitDstStageMask    = nullptr,
            .commandBufferCount   = 1,
            .pCommandBuffers      = &cmdBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores    = &presentSemaphore,
        };
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

        // Present current image
        swapchain.QueuePresent(queue, presentSemaphore);

        vkDeviceWaitIdle(device);
    }

    imIntegration.Destroy(context);

    vkDestroyFence(device, imageFence, nullptr);
    vkDestroySemaphore(device, presentSemaphore, nullptr);

    vkDestroyCommandPool(device, cmdPool, nullptr);

    swapchain.Destroy();
    context.Destroy();

    glfwDestroyWindow(window);

    glfwTerminate();
    return 0;
}
