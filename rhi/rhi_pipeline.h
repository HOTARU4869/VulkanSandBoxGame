// rhi/rhi_pipeline.h

#pragma once
#include <memory>

// 作成済みパイプラインの不透明ハンドル（バックエンド実装が継承する）
class IRHIPipeline {
public:
    virtual ~IRHIPipeline() = default;
};

using RHIPipelineRef = std::shared_ptr<IRHIPipeline>;