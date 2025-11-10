#include <cstdio>
#include <stdexcept>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_NONE
#include <vulkan/vulkan.h>

#include "glm_config.h"
#include <GLFW/glfw3.h>
#include <imgui.h>

#include "camera.h"
#include "context.h"
#include "grid.h"
#include "imgui_integration.h"
#include "lightning_pass.h"
#include "shadow_map.h"
#include "simple_cube.h"
#include "swapchain.h"
#include "texture.h"
#include "wrappers.h"

void KeyCallback(GLFWwindow* window, int key, int /*scancode*/, int /*action*/, int /*mods*/)
{
    Camera* camera = reinterpret_cast<Camera*>(glfwGetWindowUserPointer(window));

    if (!ImGui::GetIO().WantCaptureKeyboard) {
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

    Context    context("03_triangle_vertex", true);
    VkInstance instance = context.CreateInstance({}, extensions);

    // Create the window to render onto
    uint32_t    windowWidth  = 1024;
    uint32_t    windowHeight = 800;
    GLFWwindow* window       = glfwCreateWindow(windowWidth, windowHeight, "02_triangle GLFW", NULL, NULL);

    Camera camera({windowWidth, windowHeight}, 45.0f, 0.1f, 100.0f);

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

    VkCommandPool cmdPool = context.CreateCommandPool();

    std::vector<VkCommandBuffer> cmdBuffers = AllocateCommandBuffers(device, cmdPool, swapchain.images().size());

    VkFence     imageFence       = CreateFence(device);
    VkSemaphore presentSemaphore = CreateSemaphore(device);

    imIntegration.CreateContext(context, swapchain);

    VkFormat depthFormat  = VK_FORMAT_D32_SFLOAT;
    Texture  depthTexture = Texture::Create2D(context.physicalDevice(), context.device(), depthFormat,
                                              swapchain.surfaceExtent(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    struct LightInfo {
        glm::vec4 position;
    };

    LightInfo lightData = {{0.0f, 1.0f, 0.0f, 0.0f}};

    VkPipelineLayout          commonLayout            = VK_NULL_HANDLE;
    const VkPushConstantRange commonPushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_ALL,
        .offset     = 0,
        .size       = sizeof(Camera::CameraPushConstant) + sizeof(LightInfo),
    };
    {
        const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .setLayoutCount         = 0,
            .pSetLayouts            = nullptr,
            .pushConstantRangeCount = 1u,
            .pPushConstantRanges    = &commonPushConstantRange,
        };

        VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &commonLayout);
        assert(result == VK_SUCCESS);
    }

    SimpleCube cube;
    cube.Create(context, swapchain.format(), commonPushConstantRange.size);

    Grid grid;
    grid.Create(context, swapchain.format(), commonPushConstantRange.size, 10.0f, 10.0f, 10);
    grid.position(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f)));
    grid.rotation(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));

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

    ShadowMap shadowMap(depthFormat, commonPushConstantRange.size, swapchain.surfaceExtent());
    shadowMap.Create(context);

    DirectionalLight directionalLight = {
        glm::vec4(0.0f, -1.0f, -1.5f, 1.0f),
        glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 10.0f),
        glm::mat4(1.0f),
    };

    LightningPass lightningPass(swapchain.format(), depthFormat, commonPushConstantRange.size,
                                swapchain.surfaceExtent());
    lightningPass.Create(context, shadowMap.Depth());

    int32_t color = 0;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        camera.Update();
        {
            ImGuiIO& io = ImGui::GetIO();
            imIntegration.NewFrame();
            ImGui::NewFrame();

            ImGui::ShowDemoWindow();
            ImGui::Begin("Info");

            static int  rotationDegree[3] = {0, 0, 0};
            static bool autoInc           = false;

            ImGui::Checkbox("Use auto increment", &autoInc);

            if (autoInc) {
                rotationDegree[0] = (rotationDegree[0] + 1) % 360;
                rotationDegree[1] = (rotationDegree[1] + 1) % 360;
                rotationDegree[2] = (rotationDegree[2] + 1) % 360;
            }
            ImGui::SliderInt3("Rotation", rotationDegree, 0, 360);
            const glm::vec3& cameraPosition = camera.position();
            ImGui::Text("Camera position x: %.3f y: %.3f z: %.3f", cameraPosition.x, cameraPosition.y,
                        cameraPosition.z);
            const glm::vec3& targetPosition = camera.lookAtPosition();
            ImGui::Text("Target position x: %.3f y: %.3f z: %.3f", targetPosition.x, targetPosition.y,
                        targetPosition.z);
            ImGui::InputFloat3("Light Positon", (float*)&directionalLight.position);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
            ImGui::Render();

            // Hacked in rotation
            glm::mat4 cubeRotation =
                glm::rotate(glm::mat4(1.0f), glm::radians((float)rotationDegree[0]), glm::vec3(1.0f, 0.0f, 0.0f)) *
                glm::rotate(glm::mat4(1.0f), glm::radians((float)rotationDegree[1]), glm::vec3(0.0f, 1.0f, 0.0f)) *
                glm::rotate(glm::mat4(1.0f), glm::radians((float)rotationDegree[2]), glm::vec3(0.0f, 0.0f, 1.0f));

            cube.rotation(cubeRotation);

            float cameraSpeed = static_cast<float>(2.5 * 0.05);

            if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
                lightData.position.x -= cameraSpeed / 2;
            } else if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                lightData.position.x += cameraSpeed / 2;
            } else if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
                lightData.position.z += cameraSpeed / 2;
            } else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
                lightData.position.z -= cameraSpeed / 2;
            }

            directionalLight.position = lightData.position;
            directionalLight.view = glm::lookAt(
                // camera.position(),
                glm::vec3(directionalLight.position),
                glm::vec3(0.0f), // Look at the center of the scene
                glm::vec3(0.0f, -1.0f, 0.0f));
            // directionalLight.view = camera.view();
            // directionalLight.projection = camera.projection();
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

            // Shadowmap rendering
            shadowMap.BeginPass(cmdBuffer);

            shadowMap.updateLightInfo(cmdBuffer, directionalLight);

            cube.Draw(cmdBuffer, false);
            grid.Draw(cmdBuffer, false);

            shadowMap.EndPass(cmdBuffer);

            // Color rendering
            lightningPass.updateLightInfo(context, directionalLight);
            lightningPass.BeginPass(cmdBuffer);

            Camera::CameraPushConstant cameraData = {
                .position   = glm::vec4(camera.position(), 0.0f),
                .projection = camera.projection(),
                .view       = camera.view(),
            };
            vkCmdPushConstants(cmdBuffer, commonLayout, VK_SHADER_STAGE_ALL, 0, sizeof(cameraData), &cameraData);
            vkCmdPushConstants(cmdBuffer, commonLayout, VK_SHADER_STAGE_ALL, sizeof(cameraData), sizeof(lightData),
                               &lightData);

            cube.Draw(cmdBuffer, false);
            grid.Draw(cmdBuffer, false);

            // Render things
            imIntegration.Draw(cmdBuffer);
            lightningPass.EndPass(cmdBuffer);

            // BLIT
            // swapchain.CmdTransitionToRender(cmdBuffer, swapchainImage, queueFamilyIdx);
            {
                const VkImageMemoryBarrier2 renderStartBarrier = {
                    .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext               = nullptr,
                    .srcStageMask        = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                    .srcAccessMask       = VK_ACCESS_2_NONE,
                    .dstStageMask        = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                    .dstAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                    .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
                    .newLayout           = VK_IMAGE_LAYOUT_GENERAL,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image               = swapchainImage.image,
                    .subresourceRange =
                        {
                            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel   = 0,
                            .levelCount     = 1,
                            .baseArrayLayer = 0,
                            .layerCount     = 1,
                        },
                };
                const VkDependencyInfo startDependency = {
                    .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    .pNext                    = 0,
                    .dependencyFlags          = 0,
                    .memoryBarrierCount       = 0,
                    .pMemoryBarriers          = nullptr,
                    .bufferMemoryBarrierCount = 0,
                    .pBufferMemoryBarriers    = nullptr,
                    .imageMemoryBarrierCount  = 1,
                    .pImageMemoryBarriers     = &renderStartBarrier,
                };
                vkCmdPipelineBarrier2(cmdBuffer, &startDependency);
            }
            const VkImageBlit region = {
                .srcSubresource =
                    {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel       = 0,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    },
                .srcOffsets = {{0, 0, 0},
                               {(int32_t)swapchain.surfaceExtent().width, (int32_t)swapchain.surfaceExtent().height,
                                1}},
                .dstSubresource =
                    {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .mipLevel       = 0,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    },
                .dstOffsets = {{0, 0, 0},
                               {(int32_t)swapchain.surfaceExtent().width, (int32_t)swapchain.surfaceExtent().height,
                                1}},
            };
            vkCmdBlitImage(cmdBuffer, lightningPass.colorOutput().image(), VK_IMAGE_LAYOUT_GENERAL,
                           swapchainImage.image, VK_IMAGE_LAYOUT_GENERAL, 1, &region, VK_FILTER_LINEAR);

            {
                const VkImageMemoryBarrier2 renderStartBarrier = {
                    .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                    .pNext               = nullptr,
                    .srcStageMask        = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
                    .srcAccessMask       = VK_ACCESS_2_NONE,
                    .dstStageMask        = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                    .dstAccessMask       = VK_ACCESS_2_NONE,
                    .oldLayout           = VK_IMAGE_LAYOUT_GENERAL,
                    .newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .image               = swapchainImage.image,
                    .subresourceRange =
                        {
                            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                            .baseMipLevel   = 0,
                            .levelCount     = 1,
                            .baseArrayLayer = 0,
                            .layerCount     = 1,
                        },
                };
                const VkDependencyInfo startDependency = {
                    .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    .pNext                    = 0,
                    .dependencyFlags          = 0,
                    .memoryBarrierCount       = 0,
                    .pMemoryBarriers          = nullptr,
                    .bufferMemoryBarrierCount = 0,
                    .pBufferMemoryBarriers    = nullptr,
                    .imageMemoryBarrierCount  = 1,
                    .pImageMemoryBarriers     = &renderStartBarrier,
                };
                vkCmdPipelineBarrier2(cmdBuffer, &startDependency);
            }

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

    shadowMap.Destroy(context);
    lightningPass.Destroy(device);

    vkDestroyPipelineLayout(device, commonLayout, nullptr);

    depthTexture.Destroy(context.device());
    imIntegration.Destroy(context);

    vkDestroyFence(device, imageFence, nullptr);
    vkDestroySemaphore(device, presentSemaphore, nullptr);

    vkDestroyCommandPool(device, cmdPool, nullptr);

    camera.Destroy(device);
    grid.Destroy(context);
    cube.Destroy(device);
    swapchain.Destroy();
    context.Destroy();

    glfwDestroyWindow(window);

    glfwTerminate();
    return 0;
}
