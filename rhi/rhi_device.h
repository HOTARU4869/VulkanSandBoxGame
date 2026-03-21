// rhi/rhi_device.h
#pragma once
#include "rhi_types.h"
#include "rhi_texture.h"

// コマンドリスト — 描画命令を積むだけ、Vulkan を知らない
class IRHICommandList {
public:
    virtual ~IRHICommandList() = default;
    virtual void bindVertexBuffer(RHIBufferRef buf) = 0;
    virtual void bindIndexBuffer(RHIBufferRef buf) = 0;
    virtual void drawIndexed(uint32_t indexCount) = 0;
};

// デバイス — リソース生成と描画実行
class IRHIDevice {
public:
    virtual ~IRHIDevice() = default;

    virtual RHIBufferRef      createBuffer(const RHIBufferDesc&) = 0;
    virtual void              updateBuffer(RHIBufferRef, const void* data, size_t) = 0;
    virtual RHITextureRef     createTexture(const RHITextureDesc&, const void* pixels) = 0;

    // フレームの開始・終了
    virtual RHICommandListRef beginFrame() = 0;
    virtual void              endFrame(RHICommandListRef) = 0;

    virtual RHIBackend getBackend() const = 0;
};