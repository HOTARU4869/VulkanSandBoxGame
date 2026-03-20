//---Chunk.h---

#pragma once
#include "Block.h"
#include <vector>
#include <array>
#include <memory>

// チャンクのサイズ定義
constexpr int CHUNK_SIZE_X = 16;
constexpr int CHUNK_SIZE_Y = 256;
constexpr int CHUNK_SIZE_Z = 16;

class Chunk {
public:
    Chunk(int chunkX, int chunkZ);
    ~Chunk() = default;

    // ブロックの取得・設定
    Block getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockType type);

    // チャンク座標
    int getChunkX() const { return chunkX; }
    int getChunkZ() const { return chunkZ; }

    // メッシュ生成
    void generateMesh(std::vector<BlockVertex>& vertices,
        std::vector<uint32_t>& indices);

    // チャンクが変更されたかどうか
    bool isDirty() const { return dirty; }
    void setDirty(bool d) { dirty = d; }

    // 隣接チャンクの設定（面カリング用）
    void setNeighbor(int direction, Chunk* neighbor);

    // ワールド座標からチャンク内座標への変換
    static void worldToChunkCoords(int worldX, int worldY, int worldZ,
        int& chunkX, int& chunkZ,
        int& localX, int& localY, int& localZ);

private:
    int chunkX, chunkZ;
    bool dirty;

    // ブロックデータ（3D配列）
    std::array<std::array<std::array<Block, CHUNK_SIZE_Z>, CHUNK_SIZE_Y>, CHUNK_SIZE_X> blocks;

    // 隣接チャンク（面カリング用）
    Chunk* neighbors[4]; // 0:+X, 1:-X, 2:+Z, 3:-Z

    // 面が描画されるべきかチェック
    bool shouldDrawFace(int x, int y, int z, BlockFace face) const;

    // 面をメッシュに追加
    void addFaceToMesh(int x, int y, int z, BlockFace face,
        std::vector<BlockVertex>& vertices,
        std::vector<uint32_t>& indices);
};

// Chunk実装
inline Chunk::Chunk(int chunkX, int chunkZ)
    : chunkX(chunkX), chunkZ(chunkZ), dirty(true) {
    for (int i = 0; i < 4; i++) {
        neighbors[i] = nullptr;
    }

    // ブロックをAirで初期化
    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int y = 0; y < CHUNK_SIZE_Y; y++) {
            for (int z = 0; z < CHUNK_SIZE_Z; z++) {
                blocks[x][y][z] = Block(BlockType::Air);
            }
        }
    }
}

inline Block Chunk::getBlock(int x, int y, int z) const {
    if (x < 0 || x >= CHUNK_SIZE_X ||
        y < 0 || y >= CHUNK_SIZE_Y ||
        z < 0 || z >= CHUNK_SIZE_Z) {
        return Block(BlockType::Air);
    }
    return blocks[x][y][z];
}

inline void Chunk::setBlock(int x, int y, int z, BlockType type) {
    if (x < 0 || x >= CHUNK_SIZE_X ||
        y < 0 || y >= CHUNK_SIZE_Y ||
        z < 0 || z >= CHUNK_SIZE_Z) {
        return;
    }
    blocks[x][y][z].setType(type);
    dirty = true;
}

inline void Chunk::setNeighbor(int direction, Chunk* neighbor) {
    if (direction >= 0 && direction < 4) {
        neighbors[direction] = neighbor;
    }
}

inline void Chunk::worldToChunkCoords(int worldX, int worldY, int worldZ,
    int& chunkX, int& chunkZ,
    int& localX, int& localY, int& localZ) {
    chunkX = worldX / CHUNK_SIZE_X;
    chunkZ = worldZ / CHUNK_SIZE_Z;
    if (worldX < 0) chunkX--;
    if (worldZ < 0) chunkZ--;

    localX = worldX - chunkX * CHUNK_SIZE_X;
    localY = worldY;
    localZ = worldZ - chunkZ * CHUNK_SIZE_Z;
}

inline bool Chunk::shouldDrawFace(int x, int y, int z, BlockFace face) const {
    Block currentBlock = getBlock(x, y, z);
    if (!currentBlock.isOpaque()) return false;

    int nx = x, ny = y, nz = z;

    switch (face) {
    case BlockFace::Front:  nz++; break;
    case BlockFace::Back:   nz--; break;
    case BlockFace::Left:   nx--; break;
    case BlockFace::Right:  nx++; break;
    case BlockFace::Top:    ny++; break;
    case BlockFace::Bottom: ny--; break;
    }

    // チャンク内の場合
    if (nx >= 0 && nx < CHUNK_SIZE_X &&
        ny >= 0 && ny < CHUNK_SIZE_Y &&
        nz >= 0 && nz < CHUNK_SIZE_Z) {
        Block neighborBlock = getBlock(nx, ny, nz);

        // ★ 追加：同じタイプの葉っぱ同士なら面を描画しない
        if (currentBlock.getType() == BlockType::Leaves &&
            neighborBlock.getType() == BlockType::Leaves) {
            return false;
        }
        return neighborBlock.isTransparent();
    }

    // チャンク境界の場合、隣接チャンクをチェック
    // 簡略化のため、境界は常に描画
    return true;
}

inline void Chunk::addFaceToMesh(int x, int y, int z, BlockFace face,
    std::vector<BlockVertex>& vertices,
    std::vector<uint32_t>& indices) {
    int faceIndex = static_cast<int>(face);
    uint32_t baseIndex = static_cast<uint32_t>(vertices.size());

    Block block = getBlock(x, y, z);

    // 4つの頂点を追加
    for (int i = 0; i < 4; i++) {
        BlockVertex vertex;
        vertex.position[0] = x + BLOCK_VERTICES[faceIndex][i][0];
        vertex.position[1] = y + BLOCK_VERTICES[faceIndex][i][1];
        vertex.position[2] = z + BLOCK_VERTICES[faceIndex][i][2];

        vertex.texCoord[0] = BLOCK_TEX_COORDS[i][0];
        vertex.texCoord[1] = BLOCK_TEX_COORDS[i][1];

        vertex.normal[0] = BLOCK_NORMALS[faceIndex][0];
        vertex.normal[1] = BLOCK_NORMALS[faceIndex][1];
        vertex.normal[2] = BLOCK_NORMALS[faceIndex][2];

        vertex.blockType = static_cast<uint32_t>(block.getType());

        vertices.push_back(vertex);
    }

    // 2つの三角形のインデックス
    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);

    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);
}

inline void Chunk::generateMesh(std::vector<BlockVertex>& vertices,
    std::vector<uint32_t>& indices) {
    vertices.clear();
    indices.clear();

    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int y = 0; y < CHUNK_SIZE_Y; y++) {
            for (int z = 0; z < CHUNK_SIZE_Z; z++) {
                Block block = getBlock(x, y, z);
                if (block.getType() == BlockType::Air) continue;

                // 各面をチェックして、必要なら描画
                for (int f = 0; f < 6; f++) {
                    BlockFace face = static_cast<BlockFace>(f);
                    if (shouldDrawFace(x, y, z, face)) {
                        addFaceToMesh(x, y, z, face, vertices, indices);
                    }
                }
            }
        }
    }

    dirty = false;
}