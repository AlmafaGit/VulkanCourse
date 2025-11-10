#include "swapchain.h"

#include <cassert>

#include "debug.h"

namespace {

static const VkFormat g_preferredFormats[] = {
    VK_FORMAT_B8G8R8A8_SRGB,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_FORMAT_B8G8R8A8_UNORM,
    VK_FORMAT_R8G8B8A8_UNORM,
};

static const VkImageViewCreateInfo g_defaultImageViewCreateInfo = {
    .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext      = nullptr,
    .flags      = 0,
    .image      = VK_NULL_HANDLE, // will be updated later
    .viewType   = VK_IMAGE_VIEW_TYPE_2D,
    .format     = VK_FORMAT_UNDEFINED, // will be updated later
    .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                   .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                   .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                   .a = VK_COMPONENT_SWIZZLE_IDENTITY},
    .subresourceRange =
        {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
};

} // anonymous namespace

void Swapchain::AddRequiredExtensions(std::vector<const char*>& extensions)
{
    const std::vector<const char*> swapchainExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    extensions.insert(extensions.end(), swapchainExtensions.begin(), swapchainExtensions.end());
}

VkResult Swapchain::Create()
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_phyDevice, m_surface, &m_surfaceCapabilites);

    m_surfaceFormat = FindSurfaceFormat();
    if (m_surfaceFormat.format == VK_FORMAT_UNDEFINED) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    const VkResult swapchainResult = CreateVkSwapchain();
    if (swapchainResult != VK_SUCCESS) {
        return swapchainResult;
    }

    const std::vector<VkImage> images          = GetVkSwapchainImages();
    const VkResult             resourcesResult = CreateImageResources(images);
    if (resourcesResult != VK_SUCCESS) {
        return resourcesResult;
    }

    return VK_SUCCESS;
}

void Swapchain::Destroy()
{
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    for (const Swapchain::Image resource : m_swapchainImages) {
        vkDestroyImageView(m_device, resource.view, nullptr);
    }
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
}

VkSurfaceFormatKHR Swapchain::FindSurfaceFormat()
{
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_phyDevice, m_surface, &formatCount, NULL);

    if (formatCount < 1) {
        return {VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_phyDevice, m_surface, &formatCount, surfaceFormats.data());

    VkSurfaceFormatKHR selectedFormat = surfaceFormats[0];

    for (const VkSurfaceFormatKHR& entry : surfaceFormats) {
        for (size_t idx = 0; idx < sizeof(g_preferredFormats) / sizeof(g_preferredFormats[0]); idx++) {
            const VkFormat preferredFormat = g_preferredFormats[idx];

            if ((preferredFormat == entry.format) && (VK_COLOR_SPACE_SRGB_NONLINEAR_KHR == entry.colorSpace)) {
                selectedFormat = entry;
                break;
            }
        }
    }

    return selectedFormat;
}

VkResult Swapchain::CreateVkSwapchain()
{
    const VkImageUsageFlags usageFlags =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    const VkSwapchainCreateInfoKHR createInfo = {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext                 = 0,
        .flags                 = 0,
        .surface               = m_surface,
        .minImageCount         = m_surfaceCapabilites.minImageCount,
        .imageFormat           = m_surfaceFormat.format,
        .imageColorSpace       = m_surfaceFormat.colorSpace,
        .imageExtent           = m_surfaceExtent,
        .imageArrayLayers      = 1,
        .imageUsage            = usageFlags,
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .preTransform          = m_surfaceCapabilites.currentTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = m_presentMode,
        .clipped               = VK_TRUE,
        .oldSwapchain          = VK_NULL_HANDLE,
    };

    return vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain);
}

std::vector<VkImage> Swapchain::GetVkSwapchainImages()
{
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);

    assert(imageCount > 0);

    std::vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, images.data());

    return images;
}

VkResult Swapchain::CreateImageResources(const std::vector<VkImage>& images)
{
    m_swapchainImages.resize(images.size());

    for (size_t idx = 0; idx < images.size(); idx++) {
        VkImageViewCreateInfo createInfo = g_defaultImageViewCreateInfo;
        createInfo.image                 = images[idx];
        createInfo.format                = m_surfaceFormat.format;

        Swapchain::Image& currentResource = m_swapchainImages[idx];
        currentResource.image             = createInfo.image;
        currentResource.idx               = idx;

        SetResourceName(m_device, VK_OBJECT_TYPE_IMAGE, currentResource.image, "SwapchainImage_" + std::to_string(idx));

        const VkResult result = vkCreateImageView(m_device, &createInfo, nullptr, &currentResource.view);
        if (result != VK_SUCCESS) {
            return result;
        }

        SetResourceName(m_device, VK_OBJECT_TYPE_IMAGE_VIEW, currentResource.view,
                        "SwapchainImageView_" + std::to_string(idx));
    }

    return VK_SUCCESS;
}

const Swapchain::Image& Swapchain::AquireNextImage(const VkFence imageFence)
{
    vkAcquireNextImageKHR(m_device, m_swapchain, 1e9 * 2, VK_NULL_HANDLE, imageFence, &m_swapchainIdx);

    return m_swapchainImages[m_swapchainIdx];
}

void Swapchain::CmdTransitionToRender(const VkCommandBuffer   cmdBuffer,
                                      const Swapchain::Image& swapchainImage,
                                      uint32_t                queueFamilyIdx)
{
    const VkImageMemoryBarrier2 renderStartBarrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext               = nullptr,
        .srcStageMask        = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
        .srcAccessMask       = VK_ACCESS_2_NONE,
        .dstStageMask        = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
        .dstAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout           = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = queueFamilyIdx,
        .dstQueueFamilyIndex = queueFamilyIdx,
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

void Swapchain::CmdTransitionToPresent(const VkCommandBuffer   cmdBuffer,
                                       const Swapchain::Image& swapchainImage,
                                       uint32_t                queueFamilyIdx)
{
    const VkImageMemoryBarrier2 renderStartBarrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext               = nullptr,
        .srcStageMask        = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
        .srcAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR,
        .dstStageMask        = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
        .dstAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout           = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = queueFamilyIdx,
        .dstQueueFamilyIndex = queueFamilyIdx,
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

void Swapchain::QueuePresent(const VkQueue queue, const VkSemaphore presentSemaphore)
{
    VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext              = 0,
        .waitSemaphoreCount = ((presentSemaphore != VK_NULL_HANDLE) ? 1u : 0u),
        .pWaitSemaphores    = &presentSemaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &m_swapchain,
        .pImageIndices      = &m_swapchainIdx,
        .pResults           = nullptr,
    };

    vkQueuePresentKHR(queue, &presentInfo);
}
