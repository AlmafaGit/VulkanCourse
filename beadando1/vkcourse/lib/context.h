#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan_core.h>

class Context {
public:
    Context(const std::string& appName, bool useValidation)
        : m_appName(appName), m_useValidation(useValidation)
    {
    }

    // Disable copy and move constructors
    Context(const Context& otherCtx) = delete;
    Context(Context&& otherCtx)      = delete;

    VkInstance       CreateInstance(const std::vector<const char*>& layers, const std::vector<const char*>& extensions);
    VkPhysicalDevice SelectPhysicalDevice(const VkSurfaceKHR surface);
    VkDevice         CreateDevice(const std::vector<const char*>& extensions);
    void             Destroy();

    VkInstance       instance() const { return m_instance; }
    VkPhysicalDevice physicalDevice() const { return m_phyDevice; }
    VkDevice         device() const { return m_device; }
    uint32_t         queueFamilyIdx() const { return m_queueFamilyIdx; }
    VkQueue          queue() const { return m_queue; }

protected:
    bool FindQueueFamily(const VkPhysicalDevice phyDevice, const VkSurfaceKHR surface, uint32_t* outQueueFamilyIdx);

    const std::string m_appName;
    const bool        m_useValidation;

    VkInstance       m_instance       = VK_NULL_HANDLE;
    VkPhysicalDevice m_phyDevice      = VK_NULL_HANDLE;
    VkDevice         m_device         = VK_NULL_HANDLE;
    uint32_t         m_queueFamilyIdx = -1;
    VkQueue          m_queue          = VK_NULL_HANDLE;
};
