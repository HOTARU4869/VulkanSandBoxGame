//---Physics.h---

#pragma once
#include "World.h"
#include <glm/glm.hpp>
#include <algorithm>

// AABB (Axis-Aligned Bounding Box) 衝突判定
struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    AABB() : min(0.0f), max(0.0f) {}
    AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

    bool intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
            (min.y <= other.max.y && max.y >= other.min.y) &&
            (min.z <= other.max.z && max.z >= other.min.z);
    }

    glm::vec3 getCenter() const {
        return (min + max) * 0.5f;
    }

    glm::vec3 getSize() const {
        return max - min;
    }
};

// レイキャスト結果
struct RaycastHit {
    bool hit;
    glm::vec3 position;
    glm::ivec3 blockPos;
    glm::ivec3 normal;
    float distance;


    RaycastHit()
        : hit(false),
        position(0.0f),
        blockPos(0),
        normal(0),
        distance(FLT_MAX) {
    }
};

class Physics {
public:
    Physics(World* world);

    // プレイヤーの物理演算
    void updatePlayer(glm::vec3& position, glm::vec3& velocity,
        bool onGround, float deltaTime);

    // 衝突判定
    bool checkCollision(const AABB& box, glm::vec3& position, glm::vec3& velocity);

    // レイキャスト（ブロック選択用）
    RaycastHit raycast(const glm::vec3& origin, const glm::vec3& direction,
        float maxDistance);

    // 設定
    void setGravity(float g) { gravity = g; }
    float getGravity() const { return gravity; }

private:
    World* world;
    float gravity;

    // プレイヤーの衝突ボックスサイズ
    static constexpr float PLAYER_WIDTH = 0.6f;
    static constexpr float PLAYER_HEIGHT = 1.8f;
    static constexpr float PLAYER_EYE_HEIGHT = 1.62f;

    // ブロックとの衝突チェック
    bool isBlockSolid(int x, int y, int z);

    // 軸ごとの衝突解決
    bool resolveCollisionAxis(const AABB& box, glm::vec3& position,
        glm::vec3& velocity, int axis);
};

// Physics実装
inline Physics::Physics(World* world)
    : world(world), gravity(32.0f) {
}

inline void Physics::updatePlayer(glm::vec3& position, glm::vec3& velocity,
    bool onGround, float deltaTime) {
    // 重力を適用
    if (!onGround) {
        velocity.y -= gravity * deltaTime;
    }

    // 速度制限
    const float maxFallSpeed = 78.4f;
    if (velocity.y < -maxFallSpeed) {
        velocity.y = -maxFallSpeed;
    }

    // 移動量を計算
    glm::vec3 deltaPos = velocity * deltaTime;

    // プレイヤーのAABBを作成（現在位置基準）
    AABB playerBox;
    playerBox.min = position - glm::vec3(PLAYER_WIDTH / 2, 0, PLAYER_WIDTH / 2);
    playerBox.max = position + glm::vec3(PLAYER_WIDTH / 2, PLAYER_HEIGHT, PLAYER_WIDTH / 2);

    // 衝突判定（deltaPosが補正される）
    checkCollision(playerBox, position, deltaPos);

    // 補正された移動量を位置に適用
    position += deltaPos;
}

inline bool Physics::isBlockSolid(int x, int y, int z) {
    Block block = world->getBlock(x, y, z);
    return block.isSolid();
}

inline bool Physics::checkCollision(const AABB& box, glm::vec3& position,
    glm::vec3& velocity) {


    // 各軸を個別に処理（スイープ法）
    glm::vec3 finalDelta = velocity;

    // Y軸（垂直）を最初に処理
    {
        glm::vec3 testDelta(0, velocity.y, 0);
        AABB testBox = box;
        testBox.min += testDelta;
        testBox.max += testDelta;

        int minX = static_cast<int>(std::floor(testBox.min.x));
        int minY = static_cast<int>(std::floor(testBox.min.y));
        int minZ = static_cast<int>(std::floor(testBox.min.z));
        int maxX = static_cast<int>(std::floor(testBox.max.x));
        int maxY = static_cast<int>(std::floor(testBox.max.y));
        int maxZ = static_cast<int>(std::floor(testBox.max.z));

        for (int x = minX; x <= maxX; x++) {
            for (int y = minY; y <= maxY; y++) {
                for (int z = minZ; z <= maxZ; z++) {
                    if (!isBlockSolid(x, y, z)) continue;

                    AABB blockBox(glm::vec3(x, y, z), glm::vec3(x + 1, y + 1, z + 1));

                    if (testBox.intersects(blockBox)) {
                        if (velocity.y > 0) {
                            finalDelta.y = blockBox.min.y - box.max.y - 0.001f;
                        }
                        else {
                            finalDelta.y = blockBox.max.y - box.min.y + 0.001f;
                        }
                        velocity.y = 0;
                        break;
                    }
                }
            }
        }
    }

    // X軸を処理（Y軸の移動を反映）
    {
        glm::vec3 testDelta(velocity.x, finalDelta.y, 0);
        AABB testBox = box;
        testBox.min += testDelta;
        testBox.max += testDelta;

        int minX = static_cast<int>(std::floor(testBox.min.x));
        int minY = static_cast<int>(std::floor(testBox.min.y));
        int minZ = static_cast<int>(std::floor(testBox.min.z));
        int maxX = static_cast<int>(std::floor(testBox.max.x));
        int maxY = static_cast<int>(std::floor(testBox.max.y));
        int maxZ = static_cast<int>(std::floor(testBox.max.z));

        for (int x = minX; x <= maxX; x++) {
            for (int y = minY; y <= maxY; y++) {
                for (int z = minZ; z <= maxZ; z++) {
                    if (!isBlockSolid(x, y, z)) continue;

                    AABB blockBox(glm::vec3(x, y, z), glm::vec3(x + 1, y + 1, z + 1));

                    if (testBox.intersects(blockBox)) {
                        if (velocity.x > 0) {
                            finalDelta.x = blockBox.min.x - box.max.x - 0.001f;
                        }
                        else {
                            finalDelta.x = blockBox.max.x - box.min.x + 0.001f;
                        }
                        velocity.x = 0;
                        break;
                    }
                }
            }
        }
    }

    // Z軸を処理（XとYの移動を反映）
    {
        glm::vec3 testDelta(finalDelta.x, finalDelta.y, velocity.z);
        AABB testBox = box;
        testBox.min += testDelta;
        testBox.max += testDelta;

        int minX = static_cast<int>(std::floor(testBox.min.x));
        int minY = static_cast<int>(std::floor(testBox.min.y));
        int minZ = static_cast<int>(std::floor(testBox.min.z));
        int maxX = static_cast<int>(std::floor(testBox.max.x));
        int maxY = static_cast<int>(std::floor(testBox.max.y));
        int maxZ = static_cast<int>(std::floor(testBox.max.z));

        for (int x = minX; x <= maxX; x++) {
            for (int y = minY; y <= maxY; y++) {
                for (int z = minZ; z <= maxZ; z++) {
                    if (!isBlockSolid(x, y, z)) continue;

                    AABB blockBox(glm::vec3(x, y, z), glm::vec3(x + 1, y + 1, z + 1));

                    if (testBox.intersects(blockBox)) {
                        if (velocity.z > 0) {
                            finalDelta.z = blockBox.min.z - box.max.z - 0.001f;
                        }
                        else {
                            finalDelta.z = blockBox.max.z - box.min.z + 0.001f;
                        }
                        velocity.z = 0;
                        break;
                    }
                }
            }
        }
    }

    velocity = finalDelta;
    return (finalDelta.x != 0 || finalDelta.y != 0 || finalDelta.z != 0);
}

inline RaycastHit Physics::raycast(const glm::vec3& origin, const glm::vec3& direction,
    float maxDistance) {
    RaycastHit result;
    result.hit = false;

    // DDA（Digital Differential Analyzer）アルゴリズム
    glm::vec3 rayPos = origin;
    glm::vec3 rayDir = glm::normalize(direction);

    // 現在のブロック座標
    glm::ivec3 blockPos(
        static_cast<int>(std::floor(rayPos.x)),
        static_cast<int>(std::floor(rayPos.y)),
        static_cast<int>(std::floor(rayPos.z))
    );

    // レイの進む方向（-1, 0, 1）
    glm::ivec3 step(
        rayDir.x > 0 ? 1 : (rayDir.x < 0 ? -1 : 0),
        rayDir.y > 0 ? 1 : (rayDir.y < 0 ? -1 : 0),
        rayDir.z > 0 ? 1 : (rayDir.z < 0 ? -1 : 0)
    );

    // 次のブロック境界までの距離
    glm::vec3 tDelta(
        step.x != 0 ? std::abs(1.0f / rayDir.x) : FLT_MAX,
        step.y != 0 ? std::abs(1.0f / rayDir.y) : FLT_MAX,
        step.z != 0 ? std::abs(1.0f / rayDir.z) : FLT_MAX
    );

    // 現在位置から次のブロック境界までの距離
    glm::vec3 tMax;
    tMax.x = step.x > 0 ? (blockPos.x + 1 - rayPos.x) / rayDir.x :
        step.x < 0 ? (blockPos.x - rayPos.x) / rayDir.x : FLT_MAX;
    tMax.y = step.y > 0 ? (blockPos.y + 1 - rayPos.y) / rayDir.y :
        step.y < 0 ? (blockPos.y - rayPos.y) / rayDir.y : FLT_MAX;
    tMax.z = step.z > 0 ? (blockPos.z + 1 - rayPos.z) / rayDir.z :
        step.z < 0 ? (blockPos.z - rayPos.z) / rayDir.z : FLT_MAX;

    float distance = 0.0f;
    glm::ivec3 normal(0);

    // レイキャスト
    while (distance < maxDistance) {
        // 現在のブロックをチェック
        Block block = world->getBlock(blockPos.x, blockPos.y, blockPos.z);
        if (block.isSolid()) {
            result.hit = true;
            result.blockPos = blockPos;
            result.normal = normal;
            result.distance = distance;
            result.position = rayPos + rayDir * distance;
            return result;
        }

        // 次のブロックへ移動
        if (tMax.x < tMax.y) {
            if (tMax.x < tMax.z) {
                blockPos.x += step.x;
                distance = tMax.x;
                tMax.x += tDelta.x;
                normal = glm::ivec3(-step.x, 0, 0);
            }
            else {
                blockPos.z += step.z;
                distance = tMax.z;
                tMax.z += tDelta.z;
                normal = glm::ivec3(0, 0, -step.z);
            }
        }
        else {
            if (tMax.y < tMax.z) {
                blockPos.y += step.y;
                distance = tMax.y;
                tMax.y += tDelta.y;
                normal = glm::ivec3(0, -step.y, 0);
            }
            else {
                blockPos.z += step.z;
                distance = tMax.z;
                tMax.z += tDelta.z;
                normal = glm::ivec3(0, 0, -step.z);
            }
        }
    }

    return result;
}