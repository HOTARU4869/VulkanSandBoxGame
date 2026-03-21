// vulkan_device.h

#pragma once
#include "../../rhi/rhi_device.h"
#include "../../VulkanApp.h"
#include "vulkan_buffer.h"     
#include "vulkan_command_list.h"

class VulkanDevice : public IRHIDevice {
public:
    explicit VulkanDevice(VulkanApp* app) : m_app(app) {}

    RHIBufferRef createBuffer(const RHIBufferDesc& desc) override {
        VkBufferUsageFlags usage = 0;
        if (desc.isVertex)  usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        if (desc.isIndex)   usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        if (desc.isUniform) usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        VkMemoryPropertyFlags memProps = desc.hostVisible
            ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        auto buf = std::make_shared<VulkanBuffer>();
        buf->size = desc.size;
        m_app->createBuffer(desc.size, usage, memProps, buf->buffer, buf->memory);

        if (desc.hostVisible) {
            vkMapMemory(m_app->device, buf->memory, 0, desc.size, 0, &buf->mappedPtr);
        }

        return buf;
    }

    void updateBuffer(RHIBufferRef rhiBuf, const void* data, size_t size) override {
        auto buf = std::static_pointer_cast<VulkanBuffer>(rhiBuf);

        // ★ 既に map 済みなので memcpy するだけ
        if (buf->mappedPtr) {
            memcpy(buf->mappedPtr, data, size);
            return;
        }

        // map されていない場合（DEVICE_LOCAL）はステージングが必要
        // 現状は hostVisible のみ使うので基本ここには来ない
        void* mapped = nullptr;
        vkMapMemory(m_app->device, buf->memory, 0, size, 0, &mapped);
        memcpy(mapped, data, size);
        vkUnmapMemory(m_app->device, buf->memory);
    }

    // フレーム開始 — VulkanApp からコマンドバッファを受け取る
    RHICommandListRef beginFrame() override {
        m_currentImageIndex = m_app->beginFrame();
        VkCommandBuffer cmd = m_app->getCommandBuffer(m_currentImageIndex);
        return std::make_shared<VulkanCommandList>(cmd);
    }

    // フレーム終了 — VulkanApp に submit と present を委譲
    void endFrame(RHICommandListRef) override {
        m_app->endFrame(m_currentImageIndex);
    }

	// バックエンドの種類を返す
    RHIBackend getBackend() const override { return RHIBackend::Vulkan; }

	// テクスチャの作成も VulkanApp に委譲
    RHITextureRef createTexture(const RHITextureDesc& desc, const void* pixels) override {
        return m_app->createTexture(desc.width, desc.height, pixels);
    }
private:
    VulkanApp* m_app;
    uint32_t   m_currentImageIndex = 0;
};