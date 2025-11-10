#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <vulkan/vulkan_core.h>

#include "descriptors.h"

class Context {
public:
    Context(const std::string& appName, bool useValidation)
        : m_appName(appName)
        , m_useValidation(useValidation)
    {
    }

    // Disable copy and move constructors
    Context(const Context& otherCtx) = delete;
    Context(Context&& otherCtx)      = delete;

    VkInstance       CreateInstance(const std::vector<const char*>& layers, const std::vector<const char*>& extensions);
    VkPhysicalDevice SelectPhysicalDevice(const VkSurfaceKHR surface);
    VkDevice         CreateDevice(const std::vector<const char*>& extensions);
    VkCommandPool    CreateCommandPool();
    DescriptorPool   CreateDescriptorPool(const std::unordered_map<VkDescriptorType, uint32_t>& countPerType, uint32_t maxSets);

    void             Destroy();

    VkInstance       instance() const { return m_instance; }
    VkPhysicalDevice physicalDevice() const { return m_phyDevice; }
    VkDevice         device() const { return m_device; }
    uint32_t         queueFamilyIdx() const { return m_queueFamilyIdx; }
    VkQueue          queue() const { return m_queue; }
    VkCommandPool    commandPool() const { return m_commandPool; }
    DescriptorPool&  descriptorPool() { return m_descriptorPool; }

protected:
    bool FindQueueFamily(const VkPhysicalDevice phyDevice, const VkSurfaceKHR surface, uint32_t* outQueueFamilyIdx);

    const std::string m_appName;
    const bool        m_useValidation;

    VkInstance       m_instance       = VK_NULL_HANDLE;
    VkPhysicalDevice m_phyDevice      = VK_NULL_HANDLE;
    VkDevice         m_device         = VK_NULL_HANDLE;
    uint32_t         m_queueFamilyIdx = -1;
    VkQueue          m_queue          = VK_NULL_HANDLE;

    VkCommandPool    m_commandPool    = VK_NULL_HANDLE;
    DescriptorPool   m_descriptorPool = {};
};
