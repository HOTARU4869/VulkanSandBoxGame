// vulkan_command_list.h

#pragma once
#include "../../rhi/rhi_device.h"
#include "vulkan_buffer.h"
#include "vulkan_pipeline.h"
#include <vulkan/vulkan.h>

class VulkanCommandList : public IRHICommandList {
public:
    explicit VulkanCommandList(VkCommandBuffer cmd)
        : m_cmd(cmd) {
    }

    void bindVertexBuffer(RHIBufferRef buf) override {
        auto vkBuf = std::static_pointer_cast<VulkanBuffer>(buf);
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_cmd, 0, 1, &vkBuf->buffer, offsets);
    }

    void bindIndexBuffer(RHIBufferRef buf) override {
        auto vkBuf = std::static_pointer_cast<VulkanBuffer>(buf);
        vkCmdBindIndexBuffer(m_cmd, vkBuf->buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    void drawIndexed(uint32_t indexCount) override {
        vkCmdDrawIndexed(m_cmd, indexCount, 1, 0, 0, 0);
    }

    // パイプラインオブジェクトを受け取ってバインド
    void bindPipeline(RHIPipelineRef pipeline) override {
        auto vkPipeline = std::static_pointer_cast<VulkanPipeline>(pipeline);
        vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            vkPipeline->pipeline);
    }


    VkCommandBuffer getRaw() const { return m_cmd; }

private:
    VkCommandBuffer m_cmd;
};