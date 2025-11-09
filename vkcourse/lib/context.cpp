#include "context.h"

#include <cassert>


VkInstance Context::CreateInstance(const std::vector<const char*>& layers, const std::vector<const char*>& extensions)
{
    const std::vector<const char*> debugExtensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    const std::vector<const char*> debugLayers     = {"VK_LAYER_KHRONOS_validation"};

    std::vector<const char*> finalLayers = layers;
    if (m_useValidation) {
        finalLayers.insert(finalLayers.end(), debugLayers.begin(), debugLayers.end());
    }

    std::vector<const char*> finalExtensions = extensions;
    finalExtensions.insert(finalExtensions.end(), debugExtensions.begin(), debugExtensions.end());

    /*
    typedef struct VkApplicationInfo {
        VkStructureType    sType;
        const void*        pNext;
        const char*        pApplicationName;
        uint32_t           applicationVersion;
        const char*        pEngineName;
        uint32_t           engineVersion;
        uint32_t           apiVersion;
    } VkApplicationInfo;
    */
    const VkApplicationInfo appInfo = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = nullptr,
        .pApplicationName   = m_appName.c_str(),
        .applicationVersion = 1,
        .pEngineName        = "vkcourse",
        .engineVersion      = 1,
        .apiVersion         = VK_API_VERSION_1_3,
    };

    /*
    typedef struct VkInstanceCreateInfo {
        VkStructureType             sType;
        const void*                 pNext;
        VkInstanceCreateFlags       flags;
        const VkApplicationInfo*    pApplicationInfo;
        uint32_t                    enabledLayerCount;
        const char* const*          ppEnabledLayerNames;
        uint32_t                    enabledExtensionCount;
        const char* const*          ppEnabledExtensionNames;
    } VkInstanceCreateInfo;
    */
    const VkInstanceCreateInfo info = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0,
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = (uint32_t)finalLayers.size(),
        .ppEnabledLayerNames     = finalLayers.data(),
        .enabledExtensionCount   = (uint32_t)finalExtensions.size(),
        .ppEnabledExtensionNames = finalExtensions.data(),
    };
    VkResult result = vkCreateInstance(&info, nullptr, &m_instance);
    assert((result == VK_SUCCESS) && "VkInstance creation failed");

    return m_instance;
}

VkPhysicalDevice Context::SelectPhysicalDevice(const VkSurfaceKHR surface)
{
    // Query the number of physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        return VK_NULL_HANDLE;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    // Iterate over the devices and find first device and bail
    for (const VkPhysicalDevice& phyDevice : devices) {
        if (FindQueueFamily(phyDevice, surface, &m_queueFamilyIdx)) {
            m_phyDevice = phyDevice;
            return m_phyDevice;
        }
    }

    return VK_NULL_HANDLE;
}

VkDevice Context::CreateDevice(const std::vector<const char*>& extensions)
{
    const std::vector<const char *> swapchainExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    std::vector<const char *> finalExtensions = extensions;
    finalExtensions.insert(finalExtensions.end(), swapchainExtensions.begin(), swapchainExtensions.end());

    VkPhysicalDeviceSynchronization2Features syncFeatures = {
        .sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext              = nullptr,
        .synchronization2   = VK_TRUE,
    };

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRendering = {
        .sType              = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .pNext              = &syncFeatures,
        .dynamicRendering   = VK_TRUE,
    };

    const float queuePriority[1] = { 1.0f };

    const VkDeviceQueueCreateInfo queueInfo = {
        .sType              = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0,
        .queueFamilyIndex   = m_queueFamilyIdx,
        .queueCount         = 1,
        .pQueuePriorities   = queuePriority,
    };

    const VkDeviceCreateInfo createInfo = {
        .sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                      = &dynamicRendering,
        .flags                      = 0,
        .queueCreateInfoCount       = 1,
        .pQueueCreateInfos          = &queueInfo,
        .enabledLayerCount          = 0,        // deprecated
        .ppEnabledLayerNames        = nullptr,  // deprecated
        .enabledExtensionCount      = (uint32_t)finalExtensions.size(),
        .ppEnabledExtensionNames    = finalExtensions.data(),
        .pEnabledFeatures           = nullptr,
    };

    VkResult result = vkCreateDevice(m_phyDevice, &createInfo, nullptr, &m_device);
    assert((result == VK_SUCCESS) && "VkDevice creation failed");

    vkGetDeviceQueue(m_device, m_queueFamilyIdx, 0, &m_queue);

    CreateDescriptorPool(
        {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100},
        },
        100);

    return m_device;
}

DescriptorPool Context::CreateDescriptorPool(const std::unordered_map<VkDescriptorType, uint32_t>& countPerType,
                                             uint32_t                                              maxSets)
{
    VkResult result = m_descriptorPool.Create(m_device, countPerType, maxSets);
    assert((result == VK_SUCCESS) && "DescriptorPool creation failed");
    return m_descriptorPool;
}

VkCommandPool Context::CreateCommandPool()
{
    const VkCommandPoolCreateInfo createInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_queueFamilyIdx,
    };

    const VkResult result = vkCreateCommandPool(m_device, &createInfo, nullptr, &m_commandPool);
    assert((result == VK_SUCCESS) && "VkCreateCommandPool failed");

    return m_commandPool;
}

void Context::Destroy()
{
    m_descriptorPool.Destroy();
    vkDestroyDevice(m_device, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

bool Context::FindQueueFamily(const VkPhysicalDevice phyDevice, const VkSurfaceKHR surface, uint32_t* outQueueFamilyIdx)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t idx = 0; idx < queueFamilyCount; idx++) {
        if (queueFamilies[idx].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            // Check if the selected graphics queue family supports presentation.
            // At the moment the example expects that the graphics and presentation queue is the same.
            // This is not always the case.
            // TODO: add support for different graphics and presentation family indices.
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(phyDevice, idx, surface, &presentSupport);

            if (presentSupport) {
                *outQueueFamilyIdx = idx;
                return true;
            }
        }
    }

    return false;
}
