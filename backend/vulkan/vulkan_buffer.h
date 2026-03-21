// backend/vulkan/vulkan_buffer.h
#pragma once
#include "../../rhi/rhi_types.h"
#include <vulkan/vulkan.h>

struct VulkanBuffer : public IRHIBuffer {
    VkBuffer       buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mappedPtr = nullptr;
};