// material.h

#pragma once
#include "../rhi/rhi_pipeline.h"
#include "../rhi/rhi_types.h"  
#include <string>

// マテリアルのレシピ — 描画方法を宣言的に記述
// Vulkan を知らない
struct MaterialDesc {
    std::string vertShaderPath;
    std::string fragShaderPath;

    bool cullBackFace = true;   // false = 両面描画（葉っぱ・水）
    bool blendEnable = false;  // true  = アルファブレンド
    bool depthWrite = true;   // false = 透明物はデプス書き込みしない
    bool depthTest = true;
};

// 作成済みマテリアルの実体
// RenderPipeline が保持し、drawMesh() に渡す
struct Material {
    MaterialDesc   desc;      // 元のレシピ（デバッグ・再生成用）
    RHIPipelineRef pipeline;  // バックエンドが生成したパイプラインハンドル

    bool isValid() const { return pipeline != nullptr; }
};