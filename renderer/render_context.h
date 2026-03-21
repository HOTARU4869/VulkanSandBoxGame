// render_context.h

#pragma once
#include "../rhi/rhi_device.h"

// RenderPipeline が描画命令を積むコンテキスト
// Vulkan を一切知らない
class RenderContext {
public:
    explicit RenderContext(IRHIDevice* device) : m_device(device) {}

    // フレーム開始 — コマンドリストを取得
    void beginFrame() {
        m_cmdList = m_device->beginFrame();
    }

    // フレーム終了 — GPU に送信
    void endFrame() {
        m_device->endFrame(m_cmdList);
        m_cmdList = nullptr;
    }

    // 描画命令の発行
    void drawMesh(RHIBufferRef vertexBuffer,
        RHIBufferRef indexBuffer,
        uint32_t     indexCount) {
        if (!m_cmdList) return;
        m_cmdList->bindVertexBuffer(vertexBuffer);
        m_cmdList->bindIndexBuffer(indexBuffer);
        m_cmdList->drawIndexed(indexCount);
    }

    IRHIDevice* getDevice() const { return m_device; }

private:
    IRHIDevice* m_device = nullptr;
    RHICommandListRef m_cmdList = nullptr;
};