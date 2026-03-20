//---Game.h---

#pragma once
#include "World.h"
#include "Player.h"
#include <GLFW/glfw3.h>
#include <chrono>

class Game {
public:
    Game();
    ~Game();

    // 初期化
    bool initialize();

    // ゲームループ
    void run();

    // クリーンアップ
    void cleanup();

    World* getWorld() { return &world; }
    Player* getPlayer() { return &player; }
private:
    // GLFW
    GLFWwindow* window;
    int windowWidth;
    int windowHeight;

    // ゲームオブジェクト
    World world;
    Player player;

    // タイミング
    std::chrono::high_resolution_clock::time_point lastFrameTime;
    float deltaTime;

    // マウス
    bool firstMouse;
    double lastMouseX;
    double lastMouseY;

    // 設定
    int renderDistance;

    // コールバック
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    // 更新
    void update();
    void render();

    // 入力処理
    void processInput();
};

// Game実装
inline Game::Game()
    : window(nullptr),
    windowWidth(1920),
    windowHeight(1080),
    world(12345),
    player(&world, glm::vec3(0, 70, 0)),
    deltaTime(0.0f),
    firstMouse(true),
    lastMouseX(0.0),
    lastMouseY(0.0),
    renderDistance(8) {
}

inline Game::~Game() {
    cleanup();
}

inline bool Game::initialize() {
    // GLFWの初期化
    if (!glfwInit()) {
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(windowWidth, windowHeight, "Minecraft Clone", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return false;
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetKeyCallback(window, keyCallback);

    // マウスカーソルを無効化
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // 初期チャンクをロード
    world.updateLoadedChunks(0, 0, renderDistance);

    lastFrameTime = std::chrono::high_resolution_clock::now();

    return true;
}

inline void Game::run() {
    while (!glfwWindowShouldClose(window)) {
        // デルタタイムの計算
        auto currentTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        // 更新と描画
        processInput();
        update();
        render();

        glfwPollEvents();
    }
}

inline void Game::cleanup() {
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

inline void Game::processInput() {
    // ESCキーで終了
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // プレイヤーの入力処理
    player.processKeyboard(window, deltaTime);
}

inline void Game::update() {
    // プレイヤーの更新
    player.update(deltaTime);

    // チャンクの更新
    glm::vec3 playerPos = player.getPosition();
    int playerChunkX = static_cast<int>(std::floor(playerPos.x)) / CHUNK_SIZE_X;
    int playerChunkZ = static_cast<int>(std::floor(playerPos.z)) / CHUNK_SIZE_Z;

    world.updateLoadedChunks(playerChunkX, playerChunkZ, renderDistance);
}

inline void Game::render() {
    // ここにVulkanの描画コードを追加
    // 各チャンクのメッシュを生成して描画

    // デバッグ情報の表示（オプション）
    static int frameCount = 0;
    static float fpsTimer = 0.0f;
    frameCount++;
    fpsTimer += deltaTime;

    if (fpsTimer >= 1.0f) {
        char title[256];
        glm::vec3 pos = player.getPosition();
        snprintf(title, sizeof(title),
            "Minecraft Clone | FPS: %d | Pos: (%.1f, %.1f, %.1f) | Flying: %s | Chunks: %zu",
            frameCount, pos.x, pos.y, pos.z,
            player.isFlying() ? "ON" : "OFF",
            world.getChunks().size());
        glfwSetWindowTitle(window, title);
        frameCount = 0;
        fpsTimer = 0.0f;
    }
}

inline void Game::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    auto game = reinterpret_cast<Game*>(glfwGetWindowUserPointer(window));
    game->windowWidth = width;
    game->windowHeight = height;
    // スワップチェーンの再作成が必要
}

inline void Game::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    auto game = reinterpret_cast<Game*>(glfwGetWindowUserPointer(window));

    if (game->firstMouse) {
        game->lastMouseX = xpos;
        game->lastMouseY = ypos;
        game->firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos - game->lastMouseX);
    float yoffset = static_cast<float>(game->lastMouseY - ypos); // Y座標は反転

    game->lastMouseX = xpos;
    game->lastMouseY = ypos;

    game->player.processMouseMovement(xoffset, yoffset);
}

inline void Game::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto game = reinterpret_cast<Game*>(glfwGetWindowUserPointer(window));
    game->player.processMouseButton(button, action);
}

inline void Game::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // 追加のキーコールバック処理（必要に応じて）
}