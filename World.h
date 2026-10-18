//---World.h---

#pragma once
#include "Chunk.h"
#include <map>
#include <cmath>
#include <random>
#include "chunk_coord.h"

class World {
public:
    World(int seed = 12345);
    ~World();

    // ブロックの取得・設定（ワールド座標）
    Block getBlock(int x, int y, int z);
    void setBlock(int x, int y, int z, BlockType type);

    // チャンクの取得・生成
    Chunk* getChunk(int chunkX, int chunkZ);
    Chunk* getOrCreateChunk(int chunkX, int chunkZ);

    // プレイヤー周辺のチャンクをロード
    void updateLoadedChunks(int playerChunkX, int playerChunkZ, int renderDistance);

    // 地形生成
    void generateTerrain(Chunk* chunk);

    // すべてのチャンクを取得
    const std::map<ChunkCoord, std::unique_ptr<Chunk>>& getChunks() const {
        return chunks;
    }

private:
    std::map<ChunkCoord, std::unique_ptr<Chunk>> chunks;
    int seed;
    std::mt19937 rng;

    // パーリンノイズ（簡易版）
    float noise2D(float x, float z);
    float interpolate(float a, float b, float t);
    float smoothNoise(int x, int z);
};

// World実装
inline World::World(int seed) : seed(seed), rng(seed) {
}

inline World::~World() {
}

inline Block World::getBlock(int x, int y, int z) {
    int chunkX, chunkZ, localX, localY, localZ;
    Chunk::worldToChunkCoords(x, y, z, chunkX, chunkZ, localX, localY, localZ);

    Chunk* chunk = getChunk(chunkX, chunkZ);
    if (chunk) {
        return chunk->getBlock(localX, localY, localZ);
    }
    return Block(BlockType::Air);
}

inline void World::setBlock(int x, int y, int z, BlockType type) {
    int chunkX, chunkZ, localX, localY, localZ;
    Chunk::worldToChunkCoords(x, y, z, chunkX, chunkZ, localX, localY, localZ);

    Chunk* chunk = getOrCreateChunk(chunkX, chunkZ);
    if (!chunk) return;
    chunk->setBlock(localX, localY, localZ, type);

    // チャンク境界なら隣チャンクも dirty にする
    if (localX == 0) {
        Chunk* neighbor = getChunk(chunkX - 1, chunkZ);
        if (neighbor) neighbor->setDirty(true);
    }
    if (localX == CHUNK_SIZE_X - 1) {
        Chunk* neighbor = getChunk(chunkX + 1, chunkZ);
        if (neighbor) neighbor->setDirty(true);
    }
    if (localZ == 0) {
        Chunk* neighbor = getChunk(chunkX, chunkZ - 1);
        if (neighbor) neighbor->setDirty(true);
    }
    if (localZ == CHUNK_SIZE_Z - 1) {
        Chunk* neighbor = getChunk(chunkX, chunkZ + 1);
        if (neighbor) neighbor->setDirty(true);
    }
}

inline Chunk* World::getChunk(int chunkX, int chunkZ) {
    ChunkCoord coord{ chunkX, chunkZ };
    auto it = chunks.find(coord);
    if (it != chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

inline Chunk* World::getOrCreateChunk(int chunkX, int chunkZ) {
    Chunk* chunk = getChunk(chunkX, chunkZ);
    if (chunk) return chunk;

    // 新しいチャンクを作成
    ChunkCoord coord{ chunkX, chunkZ };
    chunks[coord] = std::make_unique<Chunk>(chunkX, chunkZ);
    chunk = chunks[coord].get();

    // 地形生成
    generateTerrain(chunk);

    // 隣接チャンクを設定
    Chunk* neighbor;
    if ((neighbor = getChunk(chunkX + 1, chunkZ)) != nullptr) {
        chunk->setNeighbor(0, neighbor);
        neighbor->setNeighbor(1, chunk);
    }
    if ((neighbor = getChunk(chunkX - 1, chunkZ)) != nullptr) {
        chunk->setNeighbor(1, neighbor);
        neighbor->setNeighbor(0, chunk);
    }
    if ((neighbor = getChunk(chunkX, chunkZ + 1)) != nullptr) {
        chunk->setNeighbor(2, neighbor);
        neighbor->setNeighbor(3, chunk);
    }
    if ((neighbor = getChunk(chunkX, chunkZ - 1)) != nullptr) {
        chunk->setNeighbor(3, neighbor);
        neighbor->setNeighbor(2, chunk);
    }

    return chunk;
}

inline void World::updateLoadedChunks(int playerChunkX, int playerChunkZ, int renderDistance) {
    // プレイヤー周辺のチャンクをロード
    for (int cx = playerChunkX - renderDistance; cx <= playerChunkX + renderDistance; cx++) {
        for (int cz = playerChunkZ - renderDistance; cz <= playerChunkZ + renderDistance; cz++) {
            getOrCreateChunk(cx, cz);
        }
    }

    // 遠いチャンクをアンロード（オプション）
    std::vector<ChunkCoord> toRemove;
    for (auto& pair : chunks) {
        int dx = pair.first.x - playerChunkX;
        int dz = pair.first.z - playerChunkZ;
        if (abs(dx) > renderDistance + 2 || abs(dz) > renderDistance + 2) {
            toRemove.push_back(pair.first);
        }
    }

    for (auto& coord : toRemove) {
        chunks.erase(coord);
    }
}

inline float World::interpolate(float a, float b, float t) {
    return a + t * (b - a);
}

inline float World::smoothNoise(int x, int z) {
    // 簡易的な疑似乱数ハッシュ
    int n = x + z * 57 + seed;
    n = (n << 13) ^ n;
    return 1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f;
}

inline float World::noise2D(float x, float z) {
    int intX = static_cast<int>(std::floor(x));
    int intZ = static_cast<int>(std::floor(z));
    float fracX = x - intX;
    float fracZ = z - intZ;

    // 4つの角の値
    float v1 = smoothNoise(intX, intZ);
    float v2 = smoothNoise(intX + 1, intZ);
    float v3 = smoothNoise(intX, intZ + 1);
    float v4 = smoothNoise(intX + 1, intZ + 1);

    // 補間
    float i1 = interpolate(v1, v2, fracX);
    float i2 = interpolate(v3, v4, fracX);
    return interpolate(i1, i2, fracZ);
}

constexpr int SEA_LEVEL = 24; // 海面の高さ

inline void World::generateTerrain(Chunk* chunk) {
    int baseX = chunk->getChunkX() * CHUNK_SIZE_X;
    int baseZ = chunk->getChunkZ() * CHUNK_SIZE_Z;

    for (int x = 0; x < CHUNK_SIZE_X; x++) {
        for (int z = 0; z < CHUNK_SIZE_Z; z++) {
            int worldX = baseX + x;
            int worldZ = baseZ + z;

            // マルチオクターブノイズで高さを決定
            float height = 0;
            float amplitude = 40.0f;
            float frequency = 0.01f;

            for (int octave = 0; octave < 4; octave++) {
                height += noise2D(worldX * frequency, worldZ * frequency) * amplitude;
                amplitude *= 0.5f;
                frequency *= 2.0f;
            }

            int terrainHeight = static_cast<int>(height) + 64;
            if (terrainHeight < 0) terrainHeight = 0;
            if (terrainHeight >= CHUNK_SIZE_Y) terrainHeight = CHUNK_SIZE_Y - 1;

            // ブロックを配置
            for (int y = 0; y <= terrainHeight; y++) {
                BlockType type;
                if (y == terrainHeight) {
                    type = BlockType::Grass;
                }
                else if (y >= terrainHeight - 3) {
                    type = BlockType::Dirt;
                }
                else {
                    type = BlockType::Stone;
                }
                chunk->setBlock(x, y, z, type);
            }

            // 地形が海面以下なら水で埋める
            if (terrainHeight < SEA_LEVEL) {
                for (int y = terrainHeight + 1; y <= SEA_LEVEL; y++) {
                    chunk->setBlock(x, y, z, BlockType::Water);
                }
            }

            // 木を生成（ランダム）
            if (terrainHeight > SEA_LEVEL + 5 && rng() % 100 < 2) {
                int treeHeight = 4 + (rng() % 3);
                for (int y = 1; y <= treeHeight; y++) {
                    chunk->setBlock(x, terrainHeight + y, z, BlockType::Wood);
                }
                // 葉
                for (int ly = -2; ly <= 2; ly++) {
                    for (int lx = -2; lx <= 2; lx++) {
                        for (int lz = -2; lz <= 2; lz++) {
                            if (abs(lx) + abs(lz) <= 2) {
                                int leafY = terrainHeight + treeHeight + ly;
                                if (leafY < CHUNK_SIZE_Y) {
                                    chunk->setBlock(x + lx, leafY, z + lz, BlockType::Leaves);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}