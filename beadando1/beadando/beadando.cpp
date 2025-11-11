#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "texture.h"
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_NONE
#include <vulkan/vulkan.h>

#include "glm_config.h"
#include <GLFW/glfw3.h>
#include <imgui.h>

#include "camera.h"
#include "context.h"
#include "crystal.h"
#include "imgui_integration.h"
#include "pedestal.h"
#include "star.h"
#include "swapchain.h"
#include "wrappers.h"

#include <iostream>

void KeyCallback(GLFWwindow* window, int key, int /*scancode*/, int /*action*/, int /*mods*/)
{
    Camera* camera = reinterpret_cast<Camera*>(glfwGetWindowUserPointer(window));

    switch (key) {
    case GLFW_KEY_ESCAPE: {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    }
    case GLFW_KEY_W:
        camera->Forward();
        break;
    case GLFW_KEY_S:
        camera->Back();
        break;
    case GLFW_KEY_A:
        camera->Left();
        break;
    case GLFW_KEY_D:
        camera->Right();
        break;
    }
}

void MouseCallback(GLFWwindow* window, double xposIn, double yposIn)
{
    static bool  firstMouse = true;
    static float lastX      = 0.0f;
    static float lastY      = 0.0f;

    ImGui_ImplGlfw_CursorPosCallback(window, xposIn, yposIn);

    Camera* camera = reinterpret_cast<Camera*>(glfwGetWindowUserPointer(window));

    if (!ImGui::GetIO().WantCaptureMouse && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        float xpos = static_cast<float>(xposIn);
        float ypos = static_cast<float>(yposIn);

        if (firstMouse) {
            lastX      = xpos;
            lastY      = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = ypos - lastY;

        lastX = xpos;
        lastY = ypos;

        camera->ProcessMouseMovement(xoffset, yoffset);
    } else {
        firstMouse = true;
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

    Context    context("beadando", true);
    VkInstance instance = context.CreateInstance({}, extensions);

    // Create the window to render onto
    uint32_t    windowWidth  = 1024;
    uint32_t    windowHeight = 800;
    GLFWwindow* window       = glfwCreateWindow(windowWidth, windowHeight, "beadando", NULL, NULL);

    Camera camera({windowWidth, windowHeight}, 50.0f, 0.1f, 100.0f);

    IMGUIIntegration imIntegration;
    imIntegration.Init(window);

    glfwSetWindowUserPointer(window, &camera);
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);


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

    camera.CreateVK(context.device());

    Texture depthTexture = Texture::Create2D(context.physicalDevice(), context.device(), VK_FORMAT_D32_SFLOAT,
                                             swapchain.surfaceExtent(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    Pedestal pedestal;
    pedestal.Create(context, swapchain.format(), sizeof(Camera::CameraPushConstant));

    Crystal crystal;
    crystal.Create(context, swapchain.format(), sizeof(Camera::CameraPushConstant));

    Star star;
    star.Create(context, swapchain.format(), sizeof(Camera::CameraPushConstant));

    glfwShowWindow(window);

    const VkViewport viewport = {
        .x        = 0,
        .y        = 0,
        .width    = (float)swapchain.surfaceExtent().width,
        .height   = (float)swapchain.surfaceExtent().height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    const VkRect2D scissor = {
        .offset = {0, 0},
        .extent = swapchain.surfaceExtent(),
    };

    int32_t color = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        camera.Update();
        {
            ImGuiIO& io = ImGui::GetIO();
            imIntegration.NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("Info");

            const glm::vec3& cameraPosition = camera.position();
            ImGui::Text("Camera position x: %.3f y: %.3f z: %.3f", cameraPosition.x, cameraPosition.y, cameraPosition.z);
            const glm::vec3& targetPosition = camera.lookAtPosition();
            ImGui::Text("Target position x: %.3f y: %.3f z: %.3f", targetPosition.x, targetPosition.y, targetPosition.z);
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
            const VkClearValue                 clearColor      = {{{color / 255.0f, 0.0f, 0.0f, 1.0f}}};
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
            const VkClearDepthStencilValue     depthClear      = {1.0f, 0u};
            const VkRenderingAttachmentInfoKHR depthAttachment = {
                .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
                .pNext              = nullptr,
                .imageView          = depthTexture.view(),
                .imageLayout        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                .resolveMode        = VK_RESOLVE_MODE_NONE,
                .resolveImageView   = VK_NULL_HANDLE,
                .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue         = {.depthStencil = depthClear},
            };
            const VkRenderingInfoKHR renderInfo = {
                .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
                .pNext                = nullptr,
                .flags                = 0,
                .renderArea           = {.offset = {0, 0}, .extent = {(uint32_t)windowWidth, (uint32_t)windowHeight}},
                .layerCount           = 1,
                .viewMask             = 0,
                .colorAttachmentCount = 1,
                .pColorAttachments    = &colorAttachment,
                .pDepthAttachment     = &depthAttachment,
                .pStencilAttachment   = nullptr};
            vkCmdBeginRendering(cmdBuffer, &renderInfo);

            vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
            vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

            camera.PushConstants(cmdBuffer);

            pedestal.Draw(cmdBuffer);
            crystal.Draw(cmdBuffer);
            star.Draw(cmdBuffer);

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

    depthTexture.Destroy(context.device());
    imIntegration.Destroy(context);

    vkDestroyFence(device, imageFence, nullptr);
    vkDestroySemaphore(device, presentSemaphore, nullptr);

    vkDestroyCommandPool(device, cmdPool, nullptr);

    camera.Destroy(device);
    pedestal.Destroy(context);
    crystal.Destroy(context);
    star.Destroy(context);
    swapchain.Destroy();
    context.Destroy();

    glfwDestroyWindow(window);

    glfwTerminate();

    return 0;
}
