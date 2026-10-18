// rhi/rhi_device.h
#pragma once
#include "rhi_types.h"
#include "rhi_texture.h"
#include "../renderer/material.h"  
#include <glm/glm.hpp>

// コマンドリスト — 描画命令を積むだけ、Vulkan を知らない
class IRHICommandList {
public:
    virtual ~IRHICommandList() = default;
    virtual void bindVertexBuffer(RHIBufferRef buf) = 0;
    virtual void bindIndexBuffer(RHIBufferRef buf) = 0;
    virtual void drawIndexed(uint32_t indexCount) = 0;
    virtual void bindPipeline(RHIPipelineRef pipeline) = 0;
};

// デバイス — リソース生成と描画実行
class IRHIDevice {
public:
	// リソースの生成と描画実行
    virtual ~IRHIDevice() = default;

	// バッファとテクスチャの生成・更新
    virtual RHIBufferRef  createBuffer(const RHIBufferDesc&) = 0;
    virtual void          updateBuffer(RHIBufferRef, const void* data, size_t) = 0;
    virtual RHITextureRef createTexture(const RHITextureDesc&, const void* pixels) = 0;

    // ★ マテリアル生成を一本化（descを受け取りpipelineを詰めて返す）
    virtual Material createMaterial(const MaterialDesc& desc) = 0;

	// フレーム開始・終了。beginFrame でコマンドリストを受け取る
    virtual RHICommandListRef beginFrame(const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& viewPos,
        int eyeInWater) = 0;
    virtual void              endFrame(RHICommandListRef) = 0;
    virtual RHIBackend        getBackend() const = 0;

	// GPU がアイドル状態になるまで待機（リソースの安全な破棄のためなどに使用）
    virtual void waitIdle() = 0;
};