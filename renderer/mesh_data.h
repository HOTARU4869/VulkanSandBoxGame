// renderer/mesh_data.h
#pragma once
#include <vector>
#include <cstdint>

// レンダリング層が扱う頂点の定義
// Block.h / BlockVertex への依存をここで断ち切る
struct MeshVertex {
    float    position[3];
    float    texCoord[2];
    float    normal[3];
    uint32_t blockType;
    uint32_t textureIndex;
};

// 1チャンク分のメッシュデータ（CPU側）
// opaque と transparent を一緒に持ち運ぶ
struct MeshData {
    std::vector<MeshVertex> opaqueVertices;
    std::vector<uint32_t>   opaqueIndices;

    std::vector<MeshVertex> leavesVertices;   // 葉っぱ（両面・アルファテスト）
    std::vector<uint32_t>   leavesIndices;

    std::vector<MeshVertex> waterSurfaceVertices; // 水の上面のみ（両面・ブレンド）
    std::vector<uint32_t>   waterSurfaceIndices;

    std::vector<MeshVertex> waterSideVertices;    // 水の側面・底面（背面カリング・ブレンド）
    std::vector<uint32_t>   waterSideIndices;

    bool empty() const {
        return opaqueIndices.empty()
            && leavesIndices.empty()
            && waterSurfaceIndices.empty()
            && waterSideIndices.empty();
    }
};