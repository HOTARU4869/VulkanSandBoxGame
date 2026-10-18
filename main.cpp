//---main.cpp---

#include "VulkanApp.h"
#include "backend/vulkan/vulkan_device.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <chrono>
#include <thread> 
#include "renderer/render_pipeline.h"
#include "Player.h"
#include "Utils.h"
#include <set>
#include "chunk_coord.h"

// マウスコールバック（グローバル関数として定義）
static bool   s_firstMouse = true;
static double s_lastMouseX = 0.0;
static double s_lastMouseY = 0.0;
static Player* s_player = nullptr;  // main() で設定する

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (s_firstMouse) {
        s_lastMouseX = xpos;
        s_lastMouseY = ypos;
        s_firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos - s_lastMouseX);
    float yoffset = static_cast<float>(s_lastMouseY - ypos);

    s_lastMouseX = xpos;
    s_lastMouseY = ypos;

    if (s_player) {
        s_player->processMouseMovement(xoffset, yoffset);
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (s_player) {
        s_player->processMouseButton(button, action);
    }
}

int main() {
    // Worldとプレイヤーを作成
    World world(12345);

    // ★ まず地形を生成してからプレイヤーを配置
    world.updateLoadedChunks(0, 0, 8);

    // ★ スポーン地点の地形の高さを取得
    int spawnX = 0;
    int spawnZ = 0;
    int spawnY = 70;

    // 地表を探す
    for (int y = CHUNK_SIZE_Y - 1; y >= 0; y--) {
        Block block = world.getBlock(spawnX, y, spawnZ);
        if (block.isSolid()) {
            spawnY = y + 2;  // ブロックの上 + 少し余裕
            break;
        }
    }

    Player player(&world, glm::vec3(spawnX, spawnY, spawnZ));
    s_player = &player;
    player.setFlyMode(true);  // ★ 追加：最初はフライモードで開始

    // VulkanAppを作成
    VulkanApp app;


    // 前フレームのチャンク座標を保存するセット
    std::set<ChunkCoord> prevChunkCoords;

    try {
        AppConfig config;
        config.appName = "Minecraft Vulkan";

        app.config = config;
        app.initWindow();

        // マウスコールバックを設定
        GLFWwindow* window = app.window;
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        app.initVulkan();

        // RHI と RenderPipeline だけ知っていればいい
        auto device = std::make_unique<VulkanDevice>(&app);
        auto pipeline = std::make_unique<RenderPipeline>(
            device.get(), resolvePath("textures/blocks"));

        // アトラスをGPUテクスチャとして登録
        const TextureAtlas& atlas = pipeline->getAtlas();
        RHITextureDesc texDesc;
        texDesc.width = atlas.getWidth();
        texDesc.height = atlas.getHeight();
        RHITextureRef atlasTex = device->createTexture(texDesc, atlas.getData().data());
        app.setAtlasTexture(atlasTex);

        // ゲームループ
        constexpr double TARGET_FRAME_MS = 1000.0 / 60.0;
        auto lastFrameTime = std::chrono::high_resolution_clock::now();
        while (!glfwWindowShouldClose(window)) {
            auto frameStart = std::chrono::high_resolution_clock::now();

            float deltaTime = std::chrono::duration<float>(frameStart - lastFrameTime).count();
            lastFrameTime = frameStart;

            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, true);
            }
            player.processKeyboard(window, deltaTime);
            player.update(deltaTime);

            // チャンク更新
            glm::vec3 pos = player.getPosition();
            world.updateLoadedChunks(
                static_cast<int>(std::floor(pos.x)) / CHUNK_SIZE_X,
                static_cast<int>(std::floor(pos.z)) / CHUNK_SIZE_Z, 8);

            const auto& currentChunks = world.getChunks();

            // 前フレームに存在して今フレームに存在しないチャンクを削除
            for (const auto& coord : prevChunkCoords) {
                if (currentChunks.find(coord) == currentChunks.end()) {
                    pipeline->removeChunk(coord);
                }
            }

            // dirty なチャンクのメッシュを更新
            for (auto& [coord, chunk] : currentChunks) {
                if (!chunk->isDirty()) continue;

                MeshData md;
                chunk->generateMesh(
                    md.opaqueVertices, md.opaqueIndices,
                    md.leavesVertices, md.leavesIndices,
                    md.waterSurfaceVertices, md.waterSurfaceIndices,
                    md.waterSideVertices, md.waterSideIndices);

                pipeline->submitChunk(coord, md);
            }

            // 今フレームの状態を保存
            prevChunkCoords.clear();
            for (const auto& [coord, chunk] : currentChunks) {
                prevChunkCoords.insert(coord);
            }

			const Camera& cam = player.getCamera();
			float aspect = static_cast<float>(app.swapChainExtent.width) 
                / app.swapChainExtent.height;
			glm::mat4 view = cam.getViewMatrix();
			glm::mat4 proj = cam.getProjectionMatrix(aspect);
			proj[1][1] *= -1;  // VulkanのNDCに合わせてY軸を反転

            pipeline->render(view, proj, cam.getPosition(), player.isEyeInWater() ? 1 : 0);

            // FPS表示
            static int frameCount = 0;
            static float fpsTimer = 0.0f;
            frameCount++;
            fpsTimer += deltaTime;

            if (fpsTimer >= 1.0f) {
                char title[256];
                glm::vec3 pos = player.getPosition();
                snprintf(title, sizeof(title),
                    "Minecraft Vulkan | FPS: %d | Pos: (%.1f, %.1f, %.1f) | Flying: %s | InWater: %s | EyeInWater: %s | Chunks: %zu",
                    frameCount, pos.x, pos.y, pos.z,
                    player.isFlying() ? "ON" : "OFF",
                    player.isInWater() ? "ON" : "OFF",
                    player.isEyeInWater() ? "ON" : "OFF",
                    world.getChunks().size());
                glfwSetWindowTitle(window, title);
                frameCount = 0;
                fpsTimer = 0.0f;
            }

            glfwPollEvents();

            auto frameEnd = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();
            if (elapsed < TARGET_FRAME_MS) {
                std::this_thread::sleep_for(
                    std::chrono::duration<double, std::milli>(TARGET_FRAME_MS - elapsed));
            }
        }

        vkDeviceWaitIdle(app.device);
        pipeline.reset();
        atlasTex.reset();
        device.reset();
        app.cleanup();

    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}