// chunk_coord.h

#pragma once

// チャンクのサイズ定義
constexpr int CHUNK_SIZE_X = 16;
constexpr int CHUNK_SIZE_Y = 256;
constexpr int CHUNK_SIZE_Z = 16;

// チャンク座標をキーとするための構造体
struct ChunkCoord {
    int x, z;

    bool operator<(const ChunkCoord& other) const {
        if (x != other.x) return x < other.x;
        return z < other.z;
    }

    bool operator==(const ChunkCoord& other) const {
        return x == other.x && z == other.z;
    }
};
