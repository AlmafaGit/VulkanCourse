#pragma once

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

void PrintPhyDeviceInfo(const VkInstance /*instance*/, const VkPhysicalDevice phyDevice);

VkResult CreateCommandPool(const VkDevice device, const uint32_t queueFamilyIdx, VkCommandPool* outCmdPool);
std::vector<VkCommandBuffer> AllocateCommandBuffers(const VkDevice device, const VkCommandPool cmdPool, const uint32_t count);

VkFence CreateFence(const VkDevice device);

VkSemaphore CreateSemaphore(const VkDevice device);

VkShaderModule CreateShaderModule(const VkDevice device, const uint32_t *SPIRVBinary, uint32_t SPIRVBinarySize);
