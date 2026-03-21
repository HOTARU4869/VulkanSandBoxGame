// backend/vulkan/vulkan_texture.h
#pragma once
#include "../../rhi/rhi_texture.h"
#include <vulkan/vulkan.h>

struct VulkanTexture : public IRHITexture {
    VkImage        image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView    imageView = VK_NULL_HANDLE;
    VkSampler      sampler = VK_NULL_HANDLE;
};