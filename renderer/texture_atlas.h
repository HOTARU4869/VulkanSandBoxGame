// texture_atlas.h - テクスチャアトラス管理クラス

#pragma once
#include "../third_party/stb_image.h"
#include "../Block.h"
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

// 1テクスチャのUV範囲
struct UVRect {
    float u0, v0;  // 左上
    float u1, v1;  // 右下
};

// ブロックの面の種類
enum class BlockFaceType {
    Top, Bottom, Side,
    GrassTop, GrassSide,
    WoodTop, WoodSide
};

class TextureAtlas {
public:
    static constexpr int TILE_SIZE = 16;  // 1テクスチャは16x16

    // テクスチャディレクトリを指定して初期化
    void load(const std::string& textureDir) {
        // 読み込むファイルの一覧（内部インデックス順）
        std::vector<std::string> files = {
            textureDir + "/grass_top.png",
            textureDir + "/grass_side.png",
            textureDir + "/dirt.png",
            textureDir + "/stone.png",
            textureDir + "/wood_top.png",
            textureDir + "/wood_side.png",
            textureDir + "/leaves.png",
            textureDir + "/sand.png",
            textureDir + "/water.png",
        };

        m_tileCount = static_cast<int>(files.size());

        // アトラスサイズ = 横にタイルを並べる
        m_atlasWidth = TILE_SIZE * m_tileCount;
        m_atlasHeight = TILE_SIZE;
        m_atlasData.resize(m_atlasWidth * m_atlasHeight * 4, 0);

        for (int i = 0; i < m_tileCount; i++) {
            int w, h, ch;
            stbi_set_flip_vertically_on_load(false);
            unsigned char* img = stbi_load(files[i].c_str(), &w, &h, &ch, 4);
            if (!img) {
                printf("Failed to load texture: %s\n", files[i].c_str());  // ← 追加
                fillMagenta(i);
                continue;
            }
            printf("Loaded texture: %s (%dx%d)\n", files[i].c_str(), w, h);  // ← 追加

            // アトラスにコピー
            for (int y = 0; y < TILE_SIZE; y++) {
                for (int x = 0; x < TILE_SIZE; x++) {
                    int srcIdx = (y * w + x) * 4;
                    int dstIdx = (y * m_atlasWidth + (i * TILE_SIZE + x)) * 4;
                    m_atlasData[dstIdx + 0] = img[srcIdx + 0];
                    m_atlasData[dstIdx + 1] = img[srcIdx + 1];
                    m_atlasData[dstIdx + 2] = img[srcIdx + 2];
                    m_atlasData[dstIdx + 3] = img[srcIdx + 3];
                }
            }
            stbi_image_free(img);
        }
    }

    // ブロックタイプと面からUVRectを返す
    UVRect getUV(BlockType type, BlockFace face) const {
        int index = getTextureIndex(type, face);
        float u0 = static_cast<float>(index * TILE_SIZE) / m_atlasWidth;
        float u1 = static_cast<float>((index + 1) * TILE_SIZE) / m_atlasWidth;
        return { u0, 0.0f, u1, 1.0f };
    }

    // アトラス画像データへのアクセス
    const std::vector<unsigned char>& getData()      const { return m_atlasData; }
    int                               getWidth()     const { return m_atlasWidth; }
    int                               getHeight()    const { return m_atlasHeight; }

private:
    int getTextureIndex(BlockType type, BlockFace face) const {
        switch (type) {
        case BlockType::Grass:
            return (face == BlockFace::Top) ? 0
                : (face == BlockFace::Bottom) ? 2  // 底面はDirt
                : 1;                               // 側面はGrassSide
        case BlockType::Dirt:   return 2;
        case BlockType::Stone:  return 3;
        case BlockType::Wood:
            return (face == BlockFace::Top || face == BlockFace::Bottom) ? 4 : 5;
        case BlockType::Leaves: return 6;
        case BlockType::Sand:   return 7;
        case BlockType::Water:  return 8;
        default:                return 3;  // fallback: Stone
        }
    }

    void fillMagenta(int index) {
        for (int y = 0; y < TILE_SIZE; y++) {
            for (int x = 0; x < TILE_SIZE; x++) {
                int dstIdx = (y * m_atlasWidth + (index * TILE_SIZE + x)) * 4;
                m_atlasData[dstIdx + 0] = 255;
                m_atlasData[dstIdx + 1] = 0;
                m_atlasData[dstIdx + 2] = 255;
                m_atlasData[dstIdx + 3] = 255;
            }
        }
    }

    int                        m_tileCount = 0;
    int                        m_atlasWidth = 0;
    int                        m_atlasHeight = 0;
    std::vector<unsigned char> m_atlasData;
};