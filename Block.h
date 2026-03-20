//---Block.h---

#pragma once
#include <cstdint>

// ブロックタイプの定義
enum class BlockType : uint8_t {
    Air = 0,
    Grass,
    Dirt,
    Stone,
    Wood,
    Leaves,
    Sand,
    Water,
    COUNT
};

// ブロッククラス
class Block {
public:
    Block() : type(BlockType::Air) {}
    explicit Block(BlockType t) : type(t) {}

    BlockType getType() const { return type; }
    void setType(BlockType t) { type = t; }

    bool isOpaque() const {
        return type != BlockType::Air && type != BlockType::Water;
    }

    bool isSolid() const {
        return type != BlockType::Air && type != BlockType::Water;
    }

    bool isTransparent() const {
        return type == BlockType::Air || type == BlockType::Water;
    }

private:
    BlockType type;
};

// ブロック面の方向
enum class BlockFace {
    Front = 0,  // +Z
    Back,       // -Z
    Left,       // -X
    Right,      // +X
    Top,        // +Y
    Bottom      // -Y
};

// ブロックの頂点データ（メッシュ生成用）
struct BlockVertex {
    float position[3];
    float texCoord[2];
    float normal[3];
    uint32_t blockType;
};

// 各面の頂点オフセット（立方体）
static const float BLOCK_VERTICES[6][4][3] = {
    // Front face (+Z)
    {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}},
    // Back face (-Z)
    {{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}},
    // Left face (-X)
    {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}},
    // Right face (+X)
    {{1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}},
    // Top face (+Y)
    {{0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}},
    // Bottom face (-Y)
    {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}
};

// 各面の法線
static const float BLOCK_NORMALS[6][3] = {
    {0, 0, 1},   // Front
    {0, 0, -1},  // Back
    {-1, 0, 0},  // Left
    {1, 0, 0},   // Right
    {0, 1, 0},   // Top
    {0, -1, 0}   // Bottom
};

// テクスチャ座標
static const float BLOCK_TEX_COORDS[4][2] = {
    {0, 1}, {1, 1}, {1, 0}, {0, 0}
};