//---Physics.h---

#pragma once
#include "World.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cfloat>

struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    AABB() : min(0.0f), max(0.0f) {}
    AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

    bool intersects(const AABB& other) const {
        return (min.x < other.max.x && max.x > other.min.x) &&
            (min.y < other.max.y && max.y > other.min.y) &&
            (min.z < other.max.z && max.z > other.min.z);
    }
};

struct RaycastHit {
    bool      hit = false;
    glm::vec3 position = glm::vec3(0.0f);
    glm::ivec3 blockPos = glm::ivec3(0);
    glm::ivec3 normal = glm::ivec3(0);
    float     distance = FLT_MAX;
};

class Physics {
public:
    explicit Physics(World* world) : world(world), gravity(28.0f) {}

    // メイン物理更新。position/velocity を直接書き換え、onGround を返す
    bool updatePlayer(glm::vec3& position, glm::vec3& velocity,
        bool onGround, float deltaTime);

    RaycastHit raycast(const glm::vec3& origin, const glm::vec3& direction,
        float maxDistance) const;

    void  setGravity(float g) { gravity = g; }
    float getGravity()  const { return gravity; }

    static constexpr float PLAYER_WIDTH = 0.6f;
    static constexpr float PLAYER_HEIGHT = 1.8f;
    static constexpr float PLAYER_EYE_HEIGHT = 1.62f;

private:
    World* world;
    float  gravity;

    static constexpr float MARGIN = 0.0005f; // 押し出しマージン
    static constexpr float STEP_SIZE = 0.25f;   // サブステップ最大移動量/step
    static constexpr int   MAX_SUBSTEP = 16;

    bool isBlockSolid(int x, int y, int z) const {
        return world->getBlock(x, y, z).isSolid();
    }

    // プレイヤー足元座標→AABB
    static AABB makeAABB(const glm::vec3& pos) {
        return AABB(
            pos - glm::vec3(PLAYER_WIDTH * 0.5f, 0.0f, PLAYER_WIDTH * 0.5f),
            pos + glm::vec3(PLAYER_WIDTH * 0.5f, PLAYER_HEIGHT, PLAYER_WIDTH * 0.5f)
        );
    }

    // 1軸の掃引衝突解決
    // box    : 移動前AABB
    // delta  : 移動量（単一軸）
    // axis   : 0=X, 1=Y, 2=Z
    // 戻り値 : 実際に移動できる量
    float sweepAxis(const AABB& box, float delta, int axis) const;
};

//  sweepAxis: 掃引ボリュームで正確に衝突距離を求める
inline float Physics::sweepAxis(const AABB& box, float delta, int axis) const {
    if (std::abs(delta) < 1e-7f) return 0.0f;

    const bool positive = (delta > 0.0f);

    // 移動前と移動後のAABBを合わせた「掃引AABB」で走査範囲を決める
    // これにより移動途中のブロックも確実に捕捉できる
    float sweepMin[3], sweepMax[3];
    for (int i = 0; i < 3; i++) {
        sweepMin[i] = box.min[i];
        sweepMax[i] = box.max[i];
    }
    if (positive) sweepMax[axis] += delta;
    else          sweepMin[axis] += delta;

    int bMinX = static_cast<int>(std::floor(sweepMin[0]));
    int bMinY = static_cast<int>(std::floor(sweepMin[1]));
    int bMinZ = static_cast<int>(std::floor(sweepMin[2]));
    int bMaxX = static_cast<int>(std::floor(sweepMax[0] - 1e-6f));
    int bMaxY = static_cast<int>(std::floor(sweepMax[1] - 1e-6f));
    int bMaxZ = static_cast<int>(std::floor(sweepMax[2] - 1e-6f));

    float allowedDelta = delta; // 衝突がなければそのまま移動

    for (int bx = bMinX; bx <= bMaxX; bx++) {
        for (int by = bMinY; by <= bMaxY; by++) {
            for (int bz = bMinZ; bz <= bMaxZ; bz++) {
                if (!isBlockSolid(bx, by, bz)) continue;

                AABB block(glm::vec3(bx, by, bz),
                    glm::vec3(bx + 1, by + 1, bz + 1));

                // 衝突軸以外でオーバーラップしているか確認
                // （掃引AABB全体を走査しているため、横のブロックを誤検出しないようにする）
                bool overlap = true;
                for (int i = 0; i < 3; i++) {
                    if (i == axis) continue;
                    if (box.max[i] <= block.min[i] + 1e-6f ||
                        box.min[i] >= block.max[i] - 1e-6f) {
                        overlap = false;
                        break;
                    }
                }
                if (!overlap) continue;

                // 移動軸の侵入距離を計算
                if (positive) {
                    // box の先頭面（max[axis]）が block.min[axis] に当たる
                    float gap = block.min[axis] - box.max[axis];
                    // gap が delta より小さければ衝突する
                    if (gap < allowedDelta) {
                        allowedDelta = std::max(0.0f, gap - MARGIN);
                    }
                }
                else {
                    // box の末尾面（min[axis]）が block.max[axis] に当たる
                    float gap = block.max[axis] - box.min[axis];
                    // gap が delta（負値）より大きければ衝突する
                    if (gap > allowedDelta) {
                        allowedDelta = std::min(0.0f, gap + MARGIN);
                    }
                }
            }
        }
    }

    return allowedDelta;
}

//  updatePlayer
inline bool Physics::updatePlayer(glm::vec3& position, glm::vec3& velocity,
    bool onGround, float deltaTime) {
    // 重力
    if (!onGround) {
        velocity.y -= gravity * deltaTime;
        if (velocity.y < -60.0f) velocity.y = -60.0f;
    }

    glm::vec3 delta = velocity * deltaTime;

    // サブステップ数を決定（最大移動量が STEP_SIZE を超えないように）
    float maxAbs = std::max({ std::abs(delta.x),
                              std::abs(delta.y),
                              std::abs(delta.z) });
    int substeps = (maxAbs < 1e-7f) ? 1
        : std::min(MAX_SUBSTEP,
            static_cast<int>(std::ceil(maxAbs / STEP_SIZE)));
    glm::vec3 subDelta = delta / static_cast<float>(substeps);

    bool newOnGround = false;

    for (int s = 0; s < substeps; s++) {
        AABB box = makeAABB(position);

        // ---- Y軸（優先：重力・着地の確定） ----
        float ry = sweepAxis(box, subDelta.y, 1);
        if (std::abs(ry) < std::abs(subDelta.y) - 1e-6f) {
            // 何かに当たった
            if (subDelta.y < 0.0f) newOnGround = true; // 下方向→着地
            velocity.y = 0.0f;
            subDelta.y = ry; // 今後のサブステップ用に更新
        }
        position.y += ry;

        // ---- X軸 ----
        box = makeAABB(position); // Y移動後のAABBで再評価
        float rx = sweepAxis(box, subDelta.x, 0);
        if (std::abs(rx) < std::abs(subDelta.x) - 1e-6f) {
            velocity.x = 0.0f;
            subDelta.x = rx;
        }
        position.x += rx;

        // ---- Z軸 ----
        box = makeAABB(position); // X移動後のAABBで再評価
        float rz = sweepAxis(box, subDelta.z, 2);
        if (std::abs(rz) < std::abs(subDelta.z) - 1e-6f) {
            velocity.z = 0.0f;
            subDelta.z = rz;
        }
        position.z += rz;
    }

    return newOnGround;
}

//  raycast
inline RaycastHit Physics::raycast(const glm::vec3& origin,
    const glm::vec3& direction,
    float maxDistance) const {
    RaycastHit result;

    glm::vec3  rayDir = glm::normalize(direction);
    glm::ivec3 blockPos(static_cast<int>(std::floor(origin.x)),
        static_cast<int>(std::floor(origin.y)),
        static_cast<int>(std::floor(origin.z)));

    glm::ivec3 step(rayDir.x > 0 ? 1 : (rayDir.x < 0 ? -1 : 0),
        rayDir.y > 0 ? 1 : (rayDir.y < 0 ? -1 : 0),
        rayDir.z > 0 ? 1 : (rayDir.z < 0 ? -1 : 0));

    glm::vec3 tDelta(step.x != 0 ? std::abs(1.0f / rayDir.x) : FLT_MAX,
        step.y != 0 ? std::abs(1.0f / rayDir.y) : FLT_MAX,
        step.z != 0 ? std::abs(1.0f / rayDir.z) : FLT_MAX);

    glm::vec3 tMax;
    tMax.x = step.x > 0 ? (blockPos.x + 1 - origin.x) / rayDir.x :
        step.x < 0 ? (blockPos.x - origin.x) / rayDir.x : FLT_MAX;
    tMax.y = step.y > 0 ? (blockPos.y + 1 - origin.y) / rayDir.y :
        step.y < 0 ? (blockPos.y - origin.y) / rayDir.y : FLT_MAX;
    tMax.z = step.z > 0 ? (blockPos.z + 1 - origin.z) / rayDir.z :
        step.z < 0 ? (blockPos.z - origin.z) / rayDir.z : FLT_MAX;

    float      distance = 0.0f;
    glm::ivec3 normal(0);

    while (distance < maxDistance) {
        if (world->getBlock(blockPos.x, blockPos.y, blockPos.z).isSolid()) {
            result.hit = true;
            result.blockPos = blockPos;
            result.normal = normal;
            result.distance = distance;
            result.position = origin + rayDir * distance;
            return result;
        }

        if (tMax.x < tMax.y) {
            if (tMax.x < tMax.z) {
                blockPos.x += step.x; distance = tMax.x;
                tMax.x += tDelta.x;   normal = glm::ivec3(-step.x, 0, 0);
            }
            else {
                blockPos.z += step.z; distance = tMax.z;
                tMax.z += tDelta.z;   normal = glm::ivec3(0, 0, -step.z);
            }
        }
        else {
            if (tMax.y < tMax.z) {
                blockPos.y += step.y; distance = tMax.y;
                tMax.y += tDelta.y;   normal = glm::ivec3(0, -step.y, 0);
            }
            else {
                blockPos.z += step.z; distance = tMax.z;
                tMax.z += tDelta.z;   normal = glm::ivec3(0, 0, -step.z);
            }
        }
    }
    return result;
}