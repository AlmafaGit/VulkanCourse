#include "imgui_integration.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

static VkDescriptorPool CreateSimpleDescriptorPool(const VkDevice device) {

    const VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
    };

    VkDescriptorPoolCreateInfo createInfo = {
        .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext          = nullptr,
        .flags          = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets        = 10,
        .poolSizeCount  = std::size(poolSizes),
        .pPoolSizes     = poolSizes,
    };

    VkDescriptorPool descPool = VK_NULL_HANDLE;
    VkResult result = vkCreateDescriptorPool(device, &createInfo, nullptr, &descPool);
    assert((result == VK_SUCCESS) && "");
    return descPool;
}

bool IMGUIIntegration::Init(GLFWwindow* window)
{
    if (!IMGUI_CHECKVERSION()) {
        return false;
    }
    ImGui::CreateContext();

    return ImGui_ImplGlfw_InitForVulkan(window, true);
}

bool IMGUIIntegration::CreateContext(const Context& context, const Swapchain& swapchain)
{
    m_descriptorPool = CreateSimpleDescriptorPool(context.device());

    const VkFormat swapchainFormat = swapchain.format();

    ImGui_ImplVulkan_InitInfo imguiInfo = {
        .Instance            = context.instance(),
        .PhysicalDevice      = context.physicalDevice(),
        .Device              = context.device(),
        .QueueFamily         = context.queueFamilyIdx(),
        .Queue               = context.queue(),
        .DescriptorPool      = m_descriptorPool,
        .RenderPass          = VK_NULL_HANDLE,
        .MinImageCount       = 2,
        .ImageCount          = 2,
        .MSAASamples         = VK_SAMPLE_COUNT_1_BIT,
        .PipelineCache       = VK_NULL_HANDLE,
        .Subpass             = 0,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo =
            {
                .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
                .pNext                   = nullptr,
                .viewMask                = 0,
                .colorAttachmentCount    = 1,
                .pColorAttachmentFormats = &swapchainFormat,
                .depthAttachmentFormat   = VK_FORMAT_D32_SFLOAT,
                .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
            },
        .Allocator         = nullptr,
        .CheckVkResultFn   = nullptr,
        .MinAllocationSize = 1024 * 1024,
    };
    ImGui_ImplVulkan_Init(&imguiInfo);

    ImGui_ImplVulkan_CreateFontsTexture();

    return true;
}

void IMGUIIntegration::NewFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
}

void IMGUIIntegration::Draw(const VkCommandBuffer cmdBuffer)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
}

void IMGUIIntegration::Destroy(const Context& context)
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    vkDestroyDescriptorPool(context.device(), m_descriptorPool, nullptr);
}
