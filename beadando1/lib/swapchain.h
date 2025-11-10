#pragma once

#include <cstdint>
#include <vector>

#include <vulkan/vulkan_core.h>

class Swapchain {
public:
    static void AddRequiredExtensions(std::vector<const char*>& extensions);

    struct Image {
        uint32_t    idx   = -1;
        VkImage     image = VK_NULL_HANDLE;
        VkImageView view  = VK_NULL_HANDLE;
    };
    Swapchain(const VkInstance&       instance,
              const VkPhysicalDevice& phyDevice,
              const VkDevice&         device,
              const VkSurfaceKHR&     surface,
              const VkExtent2D&       surfaceExtent)
        : m_instance(instance)
        , m_phyDevice(phyDevice)
        , m_device(device)
        , m_surface(surface)
        , m_surfaceExtent(surfaceExtent)
        , m_presentMode(VK_PRESENT_MODE_FIFO_KHR)
        , m_swapchainIdx(0)
    {
    }

    VkResult Create();
    void     Destroy();

    const Swapchain::Image& AquireNextImage(const VkFence imageFence);
    void                    CmdTransitionToRender(const VkCommandBuffer   cmdBuffer,
                                                  const Swapchain::Image& swapchainImage,
                                                  uint32_t                queueFamilyIdx);
    void                    CmdTransitionToPresent(const VkCommandBuffer   cmdBuffer,
                                                   const Swapchain::Image& swapchainImage,
                                                   uint32_t                queueFamilyIdx);
    void                    QueuePresent(const VkQueue queue, const VkSemaphore presentSemaphore);

    VkFormat                  format() const { return m_surfaceFormat.format; }
    const std::vector<Image>& images() const { return m_swapchainImages; }
    const VkExtent2D&         surfaceExtent() const { return m_surfaceExtent; }

protected:
    VkSurfaceFormatKHR   FindSurfaceFormat();
    VkResult             CreateVkSwapchain();
    std::vector<VkImage> GetVkSwapchainImages();
    VkResult             CreateImageResources(const std::vector<VkImage>& images);

    const VkInstance&       m_instance;
    const VkPhysicalDevice& m_phyDevice;
    const VkDevice&         m_device;
    const VkSurfaceKHR&     m_surface;
    const VkExtent2D        m_surfaceExtent;
    const VkPresentModeKHR  m_presentMode;

    uint32_t                 m_swapchainIdx;
    VkSurfaceCapabilitiesKHR m_surfaceCapabilites;
    VkSurfaceFormatKHR       m_surfaceFormat;
    VkSwapchainKHR           m_swapchain;

    std::vector<Swapchain::Image> m_swapchainImages;
};
