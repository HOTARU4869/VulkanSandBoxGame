// render_pipeline.h

#pragma once
#include "render_context.h"
#include "texture_atlas.h"
#include "mesh_data.h"
#include "material.h"
#include "frustum.h"
#include <algorithm>
#include <map>
#include <vector>
#include "chunk_coord.h"

class RenderPipeline {
public:
    RenderPipeline(IRHIDevice* device, const std::string& textureDir)
        : m_context(device) {
        m_atlas.load(textureDir);

        MaterialDesc opaqueDesc;
        opaqueDesc.vertShaderPath = resolvePath("shaders/vert.spv");
        opaqueDesc.fragShaderPath = resolvePath("shaders/frag.spv");
        opaqueDesc.cullBackFace = true;
        opaqueDesc.blendEnable = false;
        opaqueDesc.depthWrite = true;
        m_opaqueMat = device->createMaterial(opaqueDesc);

        // Leaves — 両面・アルファテスト（ブレンドはフラグメントシェーダ側で discard）
        MaterialDesc leavesDesc;
        leavesDesc.vertShaderPath = resolvePath("shaders/vert.spv");
        leavesDesc.fragShaderPath = resolvePath("shaders/frag_leaves.spv");
        leavesDesc.cullBackFace = false;  // 両面
        leavesDesc.blendEnable = false;  // discard で抜くのでブレンド不要
        leavesDesc.depthWrite = true;   // 不透明扱いでデプス書き込みする
        m_leavesMat = device->createMaterial(leavesDesc);

        // Water surface — 水の上面、両面・ブレンド
        MaterialDesc waterSurfDesc;
        waterSurfDesc.vertShaderPath = resolvePath("shaders/vert_water.spv");
        waterSurfDesc.fragShaderPath = resolvePath("shaders/frag_water.spv");
        waterSurfDesc.cullBackFace = false; // 両面（水中から見上げる）
        waterSurfDesc.blendEnable = true;
        waterSurfDesc.depthWrite = false;
        m_waterSurfaceMat = device->createMaterial(waterSurfDesc);

        // Water side/bottom — 側面・底面、背面カリング・ブレンド
        MaterialDesc waterSideDesc;
        waterSideDesc.vertShaderPath = resolvePath("shaders/vert.spv");
        waterSideDesc.fragShaderPath = resolvePath("shaders/frag_water.spv");
        waterSideDesc.cullBackFace = true;
        waterSideDesc.blendEnable = true;
        waterSideDesc.depthWrite = false;
        m_waterSideMat = device->createMaterial(waterSideDesc);

    }

    ~RenderPipeline(){
        m_opaqueMeshes.clear();
        m_leavesMeshes.clear();
        m_waterSurfaceMeshes.clear();
        m_waterSideMeshes.clear();
    }
    // チャンクのメッシュをGPUに登録・更新する
    void submitChunk(ChunkCoord coord, const MeshData& md) {
        MeshData resolved = md;
        resolveUV(resolved.opaqueVertices);
        resolveUV(resolved.leavesVertices);
        resolveUV(resolved.waterSurfaceVertices);
        resolveUV(resolved.waterSideVertices);

        auto applyOffset = [&](std::vector<MeshVertex>& verts) {
            for (auto& v : verts) {
                v.position[0] += coord.x * CHUNK_SIZE_X;
                v.position[2] += coord.z * CHUNK_SIZE_Z;
            }
            };
        applyOffset(resolved.opaqueVertices);
        applyOffset(resolved.leavesVertices);
        applyOffset(resolved.waterSurfaceVertices);
        applyOffset(resolved.waterSideVertices);

        uploadMesh(m_opaqueMeshes[coord], resolved.opaqueVertices, resolved.opaqueIndices);
        uploadMesh(m_leavesMeshes[coord], resolved.leavesVertices, resolved.leavesIndices);
        uploadMesh(m_waterSurfaceMeshes[coord], resolved.waterSurfaceVertices, resolved.waterSurfaceIndices);
        uploadMesh(m_waterSideMeshes[coord], resolved.waterSideVertices, resolved.waterSideIndices);
    }

    // チャンクのGPUリソースを解放する
    void removeChunk(ChunkCoord coord) {
        m_opaqueMeshes.erase(coord);
        m_leavesMeshes.erase(coord);
        m_waterSurfaceMeshes.erase(coord);
        m_waterSideMeshes.erase(coord);
    }

    void render(const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& viewPos,
        int eyeInWater) {

        m_frustum.update(proj * view);
        m_cameraPos = viewPos;
        m_context.beginFrame(view, proj, viewPos, eyeInWater);

        // 1. 不透明パス
        for (auto& [coord, mesh] : m_opaqueMeshes) {
            if (mesh.indexCount == 0) continue;
            if (!m_frustum.isVisible(makeAABB(coord))) continue;
            m_context.drawMesh(m_opaqueMat,
                mesh.vertexBuffer, mesh.indexBuffer, mesh.indexCount);
        }

        // 2. 葉っぱパス（デプス書き込みあり・discard で抜く）
        for (auto& [coord, mesh] : m_leavesMeshes) {
            if (mesh.indexCount == 0) continue;
            if (!m_frustum.isVisible(makeAABB(coord))) continue;
            m_context.drawMesh(m_leavesMat,
                mesh.vertexBuffer, mesh.indexBuffer, mesh.indexCount);
        }

        // 3. 水パス（Painter's Algorithm — 遠い順）
        //    水面と水側面を距離でソートしてまとめて描画
        std::vector<std::pair<float, ChunkCoord>> sorted;
        auto addToSort = [&](const std::map<ChunkCoord, ChunkMesh>& meshMap) {
            for (auto& [coord, mesh] : meshMap) {
                if (mesh.indexCount == 0) continue;
                if (!m_frustum.isVisible(makeAABB(coord))) continue;
                glm::vec3 center(
                    coord.x * CHUNK_SIZE_X + CHUNK_SIZE_X * 0.5f,
                    CHUNK_SIZE_Y * 0.5f,
                    coord.z * CHUNK_SIZE_Z + CHUNK_SIZE_Z * 0.5f);
                sorted.push_back({ glm::length(center - m_cameraPos), coord });
            }
            };
        addToSort(m_waterSurfaceMeshes);
        addToSort(m_waterSideMeshes);

        // 重複チャンクが入るので unique 化
        std::sort(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) { return a.first > b.first; });
        sorted.erase(std::unique(sorted.begin(), sorted.end(),
            [](const auto& a, const auto& b) { return a.second == b.second; }),
            sorted.end());

        for (auto& [dist, coord] : sorted) {
            // 水側面を先（背面カリングで遮蔽）→ 水面を後（両面）
            if (auto it = m_waterSideMeshes.find(coord);
                it != m_waterSideMeshes.end() && it->second.indexCount > 0) {
                m_context.drawMesh(m_waterSideMat,
                    it->second.vertexBuffer, it->second.indexBuffer, it->second.indexCount);
            }
            if (auto it = m_waterSurfaceMeshes.find(coord);
                it != m_waterSurfaceMeshes.end() && it->second.indexCount > 0) {
                m_context.drawMesh(m_waterSurfaceMat,
                    it->second.vertexBuffer, it->second.indexBuffer, it->second.indexCount);
            }
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
        const std::vector<MeshVertex >& verts,
        const std::vector<uint32_t>& indices) {

        if (indices.empty()) {
            mesh.indexCount = 0; // インデックスがない場合は描画しない
			return;
        }

		size_t vbSize = sizeof(MeshVertex) * verts.size();
        size_t ibSize = sizeof(uint32_t) * indices.size();

        if (!mesh.vertexBuffer || mesh.vertexBuffer->size < vbSize) {
            // 破棄前に GPU の完了を待つ
            m_context.getDevice()->waitIdle();
            RHIBufferDesc d;
            d.size = vbSize;
			d.isVertex = true;
			d.hostVisible = true;
			mesh.vertexBuffer = m_context.getDevice()->createBuffer(d);
        }
        if (!mesh.indexBuffer || mesh.indexBuffer->size < ibSize) {
            // 破棄前に GPU の完了を待つ
            m_context.getDevice()->waitIdle();
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
    
    // UV解決を専用メソッドに切り出す
    void resolveUV(std::vector<MeshVertex>& verts) {
        // 4頂点ひとまとまりで処理（addFaceToMesh が4頂点単位で積んでいるため）
        // BLOCK_TEX_COORDS の並び: {0,1},{1,1},{1,0},{0,0}
        static const float uvOffsets[4][2] = {
            {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}
        };

        int tileCount = m_atlas.getTileCount();  // アトラスの総タイル数

        for (size_t i = 0; i < verts.size(); i++) {
            uint32_t idx = verts[i].textureIndex;
            float u0 = static_cast<float>(idx) / tileCount;
            float u1 = static_cast<float>(idx + 1) / tileCount;

            int corner = static_cast<int>(i % 4);
            verts[i].texCoord[0] = uvOffsets[corner][0] > 0.5f ? u1 : u0;
            verts[i].texCoord[1] = uvOffsets[corner][1];
        }
    }

    RenderContext                   m_context;
    TextureAtlas m_atlas;
	Frustum m_frustum;
	glm::vec3 m_cameraPos;

	// マテリアルを保持するだけ — VulkanApp は何も知らない
    Material m_opaqueMat;
    Material m_leavesMat;
    Material m_waterSurfaceMat;
    Material m_waterSideMat;

	// チャンクごとのメッシュデータ
    std::map<ChunkCoord, ChunkMesh> m_opaqueMeshes;
    std::map<ChunkCoord, ChunkMesh> m_leavesMeshes;
    std::map<ChunkCoord, ChunkMesh> m_waterSurfaceMeshes;
    std::map<ChunkCoord, ChunkMesh> m_waterSideMeshes;
};