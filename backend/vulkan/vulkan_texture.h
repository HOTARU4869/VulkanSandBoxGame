// backend/vulkan/vulkan_texture.h
#pragma once
#include "../../rhi/rhi_texture.h"
#include <vulkan/vulkan.h>

struct VulkanTexture : public IRHITexture {
    VkDevice       device = VK_NULL_HANDLE;
    VkImage        image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView    imageView = VK_NULL_HANDLE;
    VkSampler      sampler = VK_NULL_HANDLE;

    ~VulkanTexture() {
        if (device == VK_NULL_HANDLE) return;
        if (sampler != VK_NULL_HANDLE) vkDestroySampler(device, sampler, nullptr);
        if (imageView != VK_NULL_HANDLE) vkDestroyImageView(device, imageView, nullptr);
        if (image != VK_NULL_HANDLE) vkDestroyImage(device, image, nullptr);
        if (memory != VK_NULL_HANDLE) vkFreeMemory(device, memory, nullptr);
    }
};