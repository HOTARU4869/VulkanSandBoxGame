//---main.cpp---

#include "VulkanApp.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <chrono>

// マウスコールバック（グローバル関数として定義）
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));

    if (app->firstMouse) {
        app->lastMouseX = xpos;
        app->lastMouseY = ypos;
        app->firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos - app->lastMouseX);
    float yoffset = static_cast<float>(app->lastMouseY - ypos);

    app->lastMouseX = xpos;
    app->lastMouseY = ypos;

    if (app->getPlayer()) {
        app->getPlayer()->processMouseMovement(xoffset, yoffset);
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    if (app->getPlayer()) {
        app->getPlayer()->processMouseButton(button, action);
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
    player.setFlyMode(true);  // ★ 追加：最初はフライモードで開始

    // VulkanAppを作成
    VulkanApp app;
    app.setWorld(&world);
    app.setPlayer(&player);

    try {
        app.initWindow();

        // マウスコールバックを設定
        GLFWwindow* window = app.window;
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        app.initVulkan();

        // ゲームループ
        auto lastFrameTime = std::chrono::high_resolution_clock::now();
        while (!glfwWindowShouldClose(window)) {
            // デルタタイムの計算
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
            lastFrameTime = currentTime;

            // 入力処理
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, true);
            }
            player.processKeyboard(window, deltaTime);

            // プレイヤー更新
            player.update(deltaTime);

            // チャンク更新
            glm::vec3 playerPos = player.getPosition();
            int playerChunkX = static_cast<int>(std::floor(playerPos.x)) / CHUNK_SIZE_X;
            int playerChunkZ = static_cast<int>(std::floor(playerPos.z)) / CHUNK_SIZE_Z;
            world.updateLoadedChunks(playerChunkX, playerChunkZ, 8);

            // 描画
            app.drawFrame();

            // FPS表示
            static int frameCount = 0;
            static float fpsTimer = 0.0f;
            frameCount++;
            fpsTimer += deltaTime;

            if (fpsTimer >= 1.0f) {
                char title[256];
                glm::vec3 pos = player.getPosition();
                snprintf(title, sizeof(title),
                    "Minecraft Vulkan | FPS: %d | Pos: (%.1f, %.1f, %.1f) | Flying: %s | Chunks: %zu",
                    frameCount, pos.x, pos.y, pos.z,
                    player.isFlying() ? "ON" : "OFF",
                    world.getChunks().size());
                glfwSetWindowTitle(window, title);
                frameCount = 0;
                fpsTimer = 0.0f;
            }

            glfwPollEvents();
        }

        vkDeviceWaitIdle(app.device);

        app.cleanup();

    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}