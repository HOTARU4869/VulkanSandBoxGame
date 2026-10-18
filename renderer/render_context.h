// render_context.h

#pragma once
#include "../rhi/rhi_device.h"

// RenderPipeline が描画命令を積むコンテキスト
// Vulkan を一切知らない
class RenderContext {
public:
    explicit RenderContext(IRHIDevice* device) : m_device(device) {}

    // フレーム開始 — コマンドリストを取得
    void beginFrame(const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& viewPos,
        int eyeInWater) {
        m_cmdList = m_device->beginFrame(view, proj, viewPos, eyeInWater);
    }

    // フレーム終了 — GPU に送信
    void endFrame() {
        m_device->endFrame(m_cmdList);
        m_cmdList = nullptr;
    }

    // Material を受け取り、内包するパイプラインをバインドしてから描画
    void drawMesh(const Material& mat,
        RHIBufferRef     vertexBuffer,
        RHIBufferRef     indexBuffer,
        uint32_t         indexCount) {
        if (!m_cmdList) return;
        m_cmdList->bindPipeline(mat.pipeline);
        m_cmdList->bindVertexBuffer(vertexBuffer);
        m_cmdList->bindIndexBuffer(indexBuffer);
        m_cmdList->drawIndexed(indexCount);
    }

    IRHIDevice* getDevice() const { return m_device; }

private:
    IRHIDevice* m_device = nullptr;
    RHICommandListRef m_cmdList = nullptr;
};