// render_pipeline.h

#pragma once
#include "render_context.h"
#include "texture_atlas.h"
#include "frustum.h"
#include "../World.h"
#include "../Player.h"
#include <algorithm>

class RenderPipeline {
public:
    RenderPipeline(IRHIDevice* device, World* world, Player* player,
        const std::string& textureDir)
        : m_context(device), m_world(world), m_player(player) {
        m_atlas.load(textureDir);  // ← テクスチャ読み込み
    }

    void updateChunks() {
        // ★ ワールドにないチャンクのメッシュを削除
        std::vector<ChunkCoord> toRemove;
        for (auto& [coord, mesh] : m_opaqueMeshes) {
            if (m_world->getChunk(coord.x, coord.z) == nullptr) {
                toRemove.push_back(coord);
            }
        }
        for (auto& coord : toRemove) {
            m_opaqueMeshes.erase(coord);
			m_transparentMeshes.erase(coord);
        }

        constexpr int MAX_UPDATES_PER_FRAME = 4;
        int updateCount = 0;

        for (auto& [coord, chunk] : m_world->getChunks()) {
            if (!chunk->isDirty()) continue;
            if (updateCount >= MAX_UPDATES_PER_FRAME) break;

			// チャンクのメッシュを再生成
            std::vector<BlockVertex> opaqueVerts, transVerts;
            std::vector<uint32_t>   opaqueIndices, transIndices;
            chunk->generateMesh(opaqueVerts, opaqueIndices, 
                transVerts , transIndices, m_atlas);

            // チャンク座標オフセット
            for (auto& v : opaqueVerts) {
                v.position[0] += coord.x * CHUNK_SIZE_X;
                v.position[2] += coord.z * CHUNK_SIZE_Z;
            }
            for (auto& v : transVerts) {
                v.position[0] += coord.x * CHUNK_SIZE_X;
                v.position[2] += coord.z * CHUNK_SIZE_Z;
            }

            uploadMesh(m_opaqueMeshes[coord], opaqueVerts, opaqueIndices);
            uploadMesh(m_transparentMeshes[coord], transVerts, transIndices);

            updateCount++;
        }
    }

    void render(const glm::mat4& view, const glm::mat4& proj) {
		m_fructum.update(proj * view);
		m_cameraPos = glm::vec3(glm::inverse(view)[3]); // カメラ位置を抽出

        m_context.beginFrame();

		// まず不透明メッシュを描画（前面の透明メッシュが正しく描画されるように）
        for (auto& [coord, mesh] : m_opaqueMeshes) {
            if (mesh.indexCount == 0) continue;
            if (!m_fructum.isVisible(makeAABB(coord))) continue;
			m_context.drawMesh(mesh.vertexBuffer, mesh.indexBuffer, mesh.indexCount);
        }

		// 透明メッシュはカメラからの距離が遠い順に描画（Painter's Algorithm）
        std::vector<std::pair<float, ChunkCoord>> sorted;
        for (auto& [coord, mesh] : m_transparentMeshes) {
            if (mesh.indexCount == 0) continue;
            if (!m_fructum.isVisible(makeAABB(coord))) continue;
            glm::vec3 center(
                coord.x * CHUNK_SIZE_X + CHUNK_SIZE_X * 0.5f,
                CHUNK_SIZE_Y * 0.5f,
				coord.z * CHUNK_SIZE_Z + CHUNK_SIZE_Z * 0.5f);
			float dist = glm::length(center - m_cameraPos);
            sorted.push_back({ dist, coord });
        }

		// 描画距離が遠い順にソート
        std::sort(sorted.begin(), sorted.end(),
			[](const auto& a, const auto& b) { return a.first > b.first; });

        for (auto& [dist, coord] : sorted) {
            auto& mesh = m_transparentMeshes[coord];
            m_context.drawMesh(mesh.vertexBuffer, mesh.indexBuffer, mesh.indexCount);
		}

        m_context.endFrame();
    }

    // アトラス画像データを外から取得できるようにする（VulkanApp がテクスチャ作成に使う）
    const TextureAtlas& getAtlas() const { return m_atlas; }

private:
    struct ChunkMesh {
        RHIBufferRef vertexBuffer;
        RHIBufferRef indexBuffer;
        uint32_t     indexCount = 0;
    };

    ChunkAABB makeAABB(const ChunkCoord& coord) const {
        ChunkAABB aabb;
        aabb.min = glm::vec3(
            coord.x * CHUNK_SIZE_X,
            0,
            coord.z * CHUNK_SIZE_Z);
        aabb.max = glm::vec3(
            coord.x * CHUNK_SIZE_X + CHUNK_SIZE_X,
            CHUNK_SIZE_Y,
            coord.z * CHUNK_SIZE_Z + CHUNK_SIZE_Z);
        return aabb;
    }

    void uploadMesh(ChunkMesh& mesh,
        const std::vector<BlockVertex>& verts,
        const std::vector<uint32_t>& indices) {

        if (indices.empty()) {
            mesh.indexCount = 0; // インデックスがない場合は描画しない
			return;
        }

		size_t vbSize = sizeof(BlockVertex) * verts.size();
        size_t ibSize = sizeof(uint32_t) * indices.size();

        if (!mesh.vertexBuffer || mesh.vertexBuffer->size < vbSize) {
            RHIBufferDesc d;
            d.size = vbSize;
			d.isVertex = true;
			d.hostVisible = true;
			mesh.vertexBuffer = m_context.getDevice()->createBuffer(d);
        }
        if (!mesh.indexBuffer || mesh.indexBuffer->size < ibSize) {
            RHIBufferDesc d;
			d.size = ibSize;
			d.isIndex = true;
			d.hostVisible = true;
			mesh.indexBuffer = m_context.getDevice()->createBuffer(d);
        }

		m_context.getDevice()->updateBuffer(mesh.vertexBuffer, verts.data(), vbSize);
		m_context.getDevice()->updateBuffer(mesh.indexBuffer, indices.data(), ibSize);
		mesh.indexCount = static_cast<uint32_t>(indices.size());
    }

    RenderContext                   m_context;
    World* m_world = nullptr;
    Player* m_player = nullptr;
    TextureAtlas m_atlas;
	Frustum m_fructum;
	glm::vec3 m_cameraPos;
	std::map<ChunkCoord, ChunkMesh> m_opaqueMeshes;
	std::map<ChunkCoord, ChunkMesh> m_transparentMeshes;
};