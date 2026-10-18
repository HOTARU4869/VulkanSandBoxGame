// backend/vulkan/vulkan_buffer.h
#pragma once
#include "../../rhi/rhi_types.h"
#include <vulkan/vulkan.h>

struct VulkanBuffer : public IRHIBuffer {
    VkDevice       device = VK_NULL_HANDLE;
    VkBuffer       buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mappedPtr = nullptr;

    ~VulkanBuffer() {
        if (device == VK_NULL_HANDLE) return;
        if (mappedPtr) {
            vkUnmapMemory(device, memory);
        }
        if (buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, buffer, nullptr);
        }
        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, memory, nullptr);
        }
    }
};