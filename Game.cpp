#include "Game.h"
#include "Rendering/Public/RenderFactory.h"
#include <iostream>

Game::Game() = default;

Game::~Game() {
    shutdown();
}

void Game::initialize() {
    // Render Pipeline の作成（Vulkan backend）
    renderPipeline = Rendering::RenderFactory::createRenderPipeline(
        Rendering::RenderBackend::Vulkan);

    renderPipeline->initialize(1920, 1080, "VulkanMinecraft");

    // World の作成
    world = std::make_unique<World>(12345);
    world->updateLoadedChunks(0, 0, 8);

    // Player の作成
    player = std::make_unique<Player>(world.get(), glm::vec3(0, 70, 0));

    lastFrameTime = std::chrono::high_resolution_clock::now();
}

void Game::run() {
    while (!renderPipeline->shouldClose()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
        lastFrameTime = currentTime;

        renderPipeline->pollEvents();

        updateLogic(deltaTime);
        render();
    }

    renderPipeline->waitForFrameReady();
}

void Game::shutdown() {
    if (renderPipeline) {
        renderPipeline->shutdown();
    }
}

void Game::updateLogic(float deltaTime) {
    if (!player) return;

    // プレイヤー入力処理など
    // player->update(deltaTime);
}

void Game::render() {
    if (!renderPipeline) return;

    renderPipeline->beginFrame();

    // ここに描画コマンドを記録
    // TODO: MeshManagerを使用した描画

    renderPipeline->endFrame();
}