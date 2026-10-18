//backend/vulkan/vulkan_pipeline.h

#pragma once
#include "../../rhi/rhi_pipeline.h"
#include <vulkan/vulkan.h>

struct VulkanPipeline : public IRHIPipeline {
    VkDevice   device = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    ~VulkanPipeline() {
        if (device != VK_NULL_HANDLE && pipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(device, pipeline, nullptr);
    }
};