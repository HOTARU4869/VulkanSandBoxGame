// backend/vulkan/ivulkan_context.h  
#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <cstdint>
#include "../../rhi/rhi_texture.h"

class IVulkanContext {
public:
    virtual ~IVulkanContext() = default;

    virtual uint32_t beginFrame(const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& viewPos,
        int eyeInWater) = 0;

    virtual VkPipelineLayout getPipelineLayout() const = 0;
    virtual VkRenderPass     getRenderPass()     const = 0;

    virtual void endFrame(uint32_t imageIndex) = 0;

    virtual VkCommandBuffer getCommandBuffer(uint32_t imageIndex) = 0;

    virtual void createBuffer(VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& memory) = 0;

    virtual RHITextureRef createTexture(uint32_t width,
        uint32_t height,
        const void* pixels) = 0;

    virtual VkDevice getDevice() const = 0;
    
    virtual void waitIdle() = 0;
};