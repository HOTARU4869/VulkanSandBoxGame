// frustum.h

#pragma once
#include <glm/glm.hpp>
#include <array>

struct Plane {
	glm::vec3 normal; // 法線ベクトル
	float distance;   // 原点からの距離
	// 点が平面のどちら側にあるかを判定
	float distanceTo(const glm::vec3& point) const {
		return glm::dot(normal, point) + distance;
	}
};

struct ChunkAABB {
	glm::vec3 min; // 最小点
	glm::vec3 max; // 最大点

	glm::vec3 center() const {return (min + max) * 0.5f;}
	glm::vec3 extents() const { return (max - min) * 0.5f; }
};

class Frustum {
private:
	std::array<Plane, 6> m_planes; // 6つの平面（左、右、上、下、近、遠）

public:
    void update(const glm::mat4& vp) {

        // 左
        m_planes[0].normal.x = vp[0][3] + vp[0][0];
        m_planes[0].normal.y = vp[1][3] + vp[1][0];
        m_planes[0].normal.z = vp[2][3] + vp[2][0];
        m_planes[0].distance = vp[3][3] + vp[3][0];

        // 右
        m_planes[1].normal.x = vp[0][3] - vp[0][0];
        m_planes[1].normal.y = vp[1][3] - vp[1][0];
        m_planes[1].normal.z = vp[2][3] - vp[2][0];
        m_planes[1].distance = vp[3][3] - vp[3][0];

        // 下
        m_planes[2].normal.x = vp[0][3] + vp[0][1];
        m_planes[2].normal.y = vp[1][3] + vp[1][1];
        m_planes[2].normal.z = vp[2][3] + vp[2][1];
        m_planes[2].distance = vp[3][3] + vp[3][1];

        // 上
        m_planes[3].normal.x = vp[0][3] - vp[0][1];
        m_planes[3].normal.y = vp[1][3] - vp[1][1];
        m_planes[3].normal.z = vp[2][3] - vp[2][1];
        m_planes[3].distance = vp[3][3] - vp[3][1];

        // 近
        m_planes[4].normal.x = vp[0][3] + vp[0][2];
        m_planes[4].normal.y = vp[1][3] + vp[1][2];
        m_planes[4].normal.z = vp[2][3] + vp[2][2];
        m_planes[4].distance = vp[3][3] + vp[3][2];

        // 遠
        m_planes[5].normal.x = vp[0][3] - vp[0][2];
        m_planes[5].normal.y = vp[1][3] - vp[1][2];
        m_planes[5].normal.z = vp[2][3] - vp[2][2];
        m_planes[5].distance = vp[3][3] - vp[3][2];

        for (auto& plane : m_planes) {
            float len = glm::length(plane.normal);
            plane.normal /= len;
            plane.distance /= len;
        }
    }

    bool isVisible(const ChunkAABB& box) const {
        for (const auto& plane : m_planes) {
            glm::vec3 positiveVertex = box.min;
            if (plane.normal.x >= 0) positiveVertex.x = box.max.x;
            if (plane.normal.y >= 0) positiveVertex.y = box.max.y;
            if (plane.normal.z >= 0) positiveVertex.z = box.max.z;
    
            if (plane.distanceTo(positiveVertex) < 0) {
                return false; // 完全に外側
            }
        }
        return true; // 少なくとも一部が見える
	}
};