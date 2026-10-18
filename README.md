# 🎮 VulkanSandBoxGame / C++

> Vulkan を用いてゼロから実装したボクセルゲームエンジンです。  
> C++ / Vulkan の技術習得を目的として個人開発しました。

![C++](https://img.shields.io/badge/C%2B%2B-17-blue?logo=cplusplus)
![Vulkan](https://img.shields.io/badge/Vulkan-1.0-red?logo=vulkan)
![Platform](https://img.shields.io/badge/Platform-Windows%20%2F%20Linux-lightgrey)

---

## 📸 スクリーンショット

<!-- スクリーンショットを撮ったら下の行のコメントを外して画像を追加してください -->
<!-- ![gameplay](docs/screenshot.png) -->

---

## 🔍 概要

Minecraft ライクなボクセルワールドを **Vulkan + C++17** でスクラッチ実装したプロジェクトです。  
グラフィックス API の低レベルな仕組み（スワップチェーン・レンダーパス・デスクリプタセット等）を理解することを主な目標としています。

---

## ✨ 実装機能

### グラフィックス
- Vulkan による描画パイプライン（スワップチェーン・レンダーパス・フレームバッファ）
- テクスチャアトラスによるブロックテクスチャ管理
- 不透明・葉っぱ（アルファテスト）・水面・水側面の **4パス描画**
- 視錐台カリング（Frustum Culling）による描画最適化
- Painter's Algorithm による水の透明描画ソート
- 動的頂点バッファ更新（ホストマップ済みバッファで毎フレーム効率更新）

### ワールド / 地形
- マルチオクターブ Perlin ノイズによるプロシージャル地形生成
- 16 × 256 × 16 チャンク単位のブロック管理
- プレイヤー位置に追従した動的チャンクロード / アンロード
- チャンク境界での面カリング（隣接チャンクを参照した最適化）

### 物理 / プレイヤー
- AABB 掃引法（Sweep）による正確なブロック衝突検出
- 重力・ジャンプ・水中物理（減衰・浮力）
- DDA アルゴリズムによるレイキャスト（ブロック選択・設置）
- フライモード切り替え（`F` キー）

### アーキテクチャ
- **RHI（Rendering Hardware Interface）抽象化** による Vulkan 依存の分離
- Material / RenderPipeline による描画レシピの宣言的記述
- Vulkan 詳細を知らないレンダリング層（差し替え可能な設計）

---

## 🏗️ アーキテクチャ

```
main.cpp
├── World / Chunk        ← 地形生成・ブロック管理
├── Player / Physics     ← 入力・物理・衝突
└── RenderPipeline       ← 描画管理（Vulkan を知らない）
    └── IRHIDevice       ← RHI 抽象化インターフェース
        └── VulkanDevice ← Vulkan 実装
            └── VulkanApp ← スワップチェーン・同期オブジェクト管理
```

---

## 🛠️ 使用技術

| カテゴリ | 技術 |
|---|---|
| 言語 | C++17 |
| グラフィックス API | Vulkan 1.0 |
| ウィンドウ / 入力 | GLFW 3 |
| 数学ライブラリ | GLM |
| 画像読み込み | stb_image |
| ビルドシステム | CMake |

---

## 🚀 ビルド方法

**必要環境**
- Vulkan SDK（LunarG）
- CMake 3.20 以上
- C++17 対応コンパイラ（MSVC / GCC / Clang）

```bash
git clone https://github.com/YOUR_USERNAME/minecraft-vulkan.git
cd minecraft-vulkan
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

シェーダーは事前に GLSL → SPIR-V へコンパイルが必要です。

```bash
glslc shaders/vert.glsl -o shaders/vert.spv
glslc shaders/frag.glsl -o shaders/frag.spv
# ... 他のシェーダーも同様
```

---

## 🎮 操作方法

| キー / 操作 | 動作 |
|---|---|
| `W` `A` `S` `D` | 移動 |
| マウス | 視点回転 |
| `Space` | ジャンプ / 水中で浮上 |
| `Left Shift` | しゃがみ / 水中で潜る |
| `Left Ctrl` | ダッシュ |
| `F` | フライモード切り替え |
| `1` ～ `5` | ブロック選択 |
| 左クリック | ブロック破壊 |
| 右クリック | ブロック設置 |
| `Esc` | 終了 |

---

## 📁 ディレクトリ構成

```
├── backend/vulkan/   Vulkan バックエンド実装
├── renderer/         描画パイプライン・メッシュ管理
├── rhi/              RHI 抽象化インターフェース
├── shaders/          GLSL シェーダー / SPIR-V
├── textures/blocks/  ブロックテクスチャ (PNG)
├── third_party/      stb_image
├── Chunk.h / World.h 地形・ブロック管理
├── Player.h          プレイヤー・入力処理
├── Physics.h         物理・衝突検出
└── main.cpp          エントリポイント
```

---

## 📝 開発メモ・工夫した点

- **RHI 抽象化**: `IRHIDevice` / `IRHICommandList` を挟むことで描画ロジックを Vulkan から切り離し、将来的に Direct3D 12 や Metal へ差し替えやすい設計にしました。
- **水の描画**: 水面（両面・アルファブレンド）と水側面（背面カリング）を分離し、遠い順にソートする Painter's Algorithm を採用して正しい透明合成を実現しました。
- **チャンク境界の最適化**: `shouldDrawFace()` が隣接チャンクのブロックを参照することで、チャンク境界に不要な面が生成されないようにしました。
- **AABB 掃引法**: 単純な AABB 重複判定ではなく、移動量を軸ごとに分解する掃引法を実装し、高速移動時のすり抜けを防いでいます。

---

## 📄 ライセンス

MIT License
