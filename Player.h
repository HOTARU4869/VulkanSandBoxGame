//---Player.h---

#pragma once
#include "Camera.h"
#include "Physics.h"
#include "World.h"
#include <GLFW/glfw3.h>

class Player {
public:
    Player(World* world, const glm::vec3& startPosition = glm::vec3(0, 70, 0));

    // 更新
    void update(float deltaTime);

    // 入力処理
    void processKeyboard(GLFWwindow* window, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset);
    void processMouseButton(int button, int action);

    // カメラの取得
    Camera& getCamera() { return camera; }
    const Camera& getCamera() const { return camera; }

    // プレイヤー情報
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getVelocity() const { return velocity; }
    bool isOnGround() const { return onGround; }
    bool isFlying() const { return flying; }

    // 水中判定 (地面用と目の位置用)
    bool isEyeInWater() const { return eyeInWater; }
    bool isInWater()    const { return inWater; }

    // モード切り替え
    void toggleFlyMode() { flying = !flying; }
    void setFlyMode(bool enabled) { flying = enabled; }

    // 選択中のブロックタイプ
    void setSelectedBlockType(BlockType type) { selectedBlockType = type; }
    BlockType getSelectedBlockType() const { return selectedBlockType; }

private:
    World* world;
    Physics physics;
    Camera camera;

    // プレイヤーの状態
    glm::vec3 position;
    glm::vec3 velocity;
    bool onGround;
    bool flying;

	// 水中判定
    bool inWater = false;
    bool eyeInWater = false;  // フォグ用

    // プレイヤー設定
    float walkSpeed;
    float sprintSpeed;
    float flySpeed;
    float jumpStrength;

    // ブロック操作
    BlockType selectedBlockType;
    float blockInteractionCooldown;
    static constexpr float BLOCK_INTERACTION_DELAY = 0.2f;
    static constexpr float REACH_DISTANCE = 5.0f;

    // 地面チェック
    void checkOnGround();

    // ブロック操作
    void breakBlock();
    void placeBlock();
};

// Player実装
inline Player::Player(World* world, const glm::vec3& startPosition)
    : world(world),
    physics(world),
    camera(startPosition),
    position(startPosition),
    velocity(0.0f),
    onGround(false),
    flying(false),
    walkSpeed(4.3f),
    sprintSpeed(5.6f),
    flySpeed(10.0f),
    jumpStrength(9.0f),
    selectedBlockType(BlockType::Stone),
    blockInteractionCooldown(0.0f) {
}

inline void Player::update(float deltaTime) {
    if (blockInteractionCooldown > 0) {
        blockInteractionCooldown -= deltaTime;
    }

    inWater = physics.isInWater(position);
    eyeInWater = physics.isEyeInWater(position);

    if (flying) {
        velocity = glm::vec3(0.0f);
    }
    else {
        onGround = physics.updatePlayer(position, velocity, onGround, inWater, deltaTime);
    }

    // カメラ位置を更新
    camera.setPosition(position + glm::vec3(0.0f, Physics::PLAYER_EYE_HEIGHT, 0.0f));
}

inline void Player::checkOnGround() {
    // プレイヤーの足元より少し下をチェック
    glm::vec3 checkPos = position;
    checkPos.y -= 0.05f;  // 少し下をチェック

    AABB playerBox;
    playerBox.min = checkPos - glm::vec3(0.3f, 0.0f, 0.3f);
    playerBox.max = checkPos + glm::vec3(0.3f, 0.01f, 0.3f);  // 薄いボックス

    int minX = static_cast<int>(std::floor(playerBox.min.x));
    int minY = static_cast<int>(std::floor(playerBox.min.y));
    int minZ = static_cast<int>(std::floor(playerBox.min.z));
    int maxX = static_cast<int>(std::floor(playerBox.max.x));
    int maxY = static_cast<int>(std::floor(playerBox.max.y));
    int maxZ = static_cast<int>(std::floor(playerBox.max.z));

    onGround = false;
    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            for (int z = minZ; z <= maxZ; z++) {
                Block block = world->getBlock(x, y, z);
                if (block.isSolid()) {
                    AABB blockBox(glm::vec3(x, y, z), glm::vec3(x + 1, y + 1, z + 1));
                    if (playerBox.intersects(blockBox)) {
                        onGround = true;
                        return;
                    }
                }
            }
        }
    }
}

inline void Player::processKeyboard(GLFWwindow* window, float deltaTime) {
    bool forward = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    bool backward = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    bool left = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    bool right = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    bool up = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    bool down = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    bool sprint = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;

    if (flying) {
        camera.setMovementSpeed(flySpeed);
        camera.processKeyboard(deltaTime, forward, backward, left, right, up, down);
        position = camera.getPosition();
        position.y -= Physics::PLAYER_EYE_HEIGHT;
    }
    else if (inWater) {
        // 水中移動
        float currentSpeed = walkSpeed * 0.5f;  // 地上より遅い

        glm::vec3 moveDir(0.0f);
        if (forward)  moveDir += glm::normalize(glm::vec3(camera.getFront().x, 0, camera.getFront().z));
        if (backward) moveDir -= glm::normalize(glm::vec3(camera.getFront().x, 0, camera.getFront().z));
        if (left)     moveDir -= camera.getRight();
        if (right)    moveDir += camera.getRight();

        if (glm::length(moveDir) > 0.0f) {
            moveDir = glm::normalize(moveDir);
            velocity.x = moveDir.x * currentSpeed;
            velocity.z = moveDir.z * currentSpeed;
        }

        // スペースで浮上、Shiftで潜る
        if (up)   velocity.y = 3.0f;
        if (down) velocity.y = -3.0f;

        // 水面付近でスペースを押すと水面を突破して飛び出す
        // isInWater の判定が足元基準なので、頭が水面に近いかチェック
        if (up && !physics.isEyeInWater(position)) {
            // 頭が水面より上 = 水面突破できる距離 → ジャンプ力を与える
            velocity.y = jumpStrength;
        }
    }
    else {
        // 地上移動
        float currentSpeed = sprint ? sprintSpeed : walkSpeed;

        glm::vec3 moveDir(0.0f);
        if (forward)  moveDir += glm::normalize(glm::vec3(camera.getFront().x, 0, camera.getFront().z));
        if (backward) moveDir -= glm::normalize(glm::vec3(camera.getFront().x, 0, camera.getFront().z));
        if (left)     moveDir -= camera.getRight();
        if (right)    moveDir += camera.getRight();

        if (glm::length(moveDir) > 0.0f) {
            moveDir = glm::normalize(moveDir);
            velocity.x = moveDir.x * currentSpeed;
            velocity.z = moveDir.z * currentSpeed;
        }
        else {
            velocity.x = 0;
            velocity.z = 0;
        }

        if (up && onGround) {
            velocity.y = jumpStrength;
            onGround = false;
        }
    }

    // フライモード切り替え（Fキー）
    static bool fKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (!fKeyPressed) { toggleFlyMode(); fKeyPressed = true; }
    }
    else {
        fKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) selectedBlockType = BlockType::Grass;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) selectedBlockType = BlockType::Dirt;
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) selectedBlockType = BlockType::Stone;
    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) selectedBlockType = BlockType::Wood;
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) selectedBlockType = BlockType::Leaves;
}

inline void Player::processMouseMovement(float xoffset, float yoffset) {
    camera.processMouseMovement(xoffset, yoffset);
}

inline void Player::processMouseButton(int button, int action) {
    if (blockInteractionCooldown > 0) return;

    if (action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            breakBlock();
            blockInteractionCooldown = BLOCK_INTERACTION_DELAY;
        }
        else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            placeBlock();
            blockInteractionCooldown = BLOCK_INTERACTION_DELAY;
        }
    }
}

inline void Player::breakBlock() {
    // レイキャスト
    RaycastHit hit = physics.raycast(camera.getPosition(), camera.getFront(), REACH_DISTANCE);

    if (hit.hit) {
        // ブロックを空気に変更（破壊）
        world->setBlock(hit.blockPos.x, hit.blockPos.y, hit.blockPos.z, BlockType::Air);
    }
}

inline void Player::placeBlock() {
    // レイキャスト
    RaycastHit hit = physics.raycast(camera.getPosition(), camera.getFront(), REACH_DISTANCE);

    if (hit.hit) {
        // ヒットした面の法線方向にブロックを配置
        glm::ivec3 placePos = hit.blockPos + hit.normal;

        // プレイヤーの位置と重ならないかチェック
        AABB playerBox;
        playerBox.min = position - glm::vec3(0.3f, 0.0f, 0.3f);
        playerBox.max = position + glm::vec3(0.3f, 1.8f, 0.3f);

        AABB blockBox;
        blockBox.min = glm::vec3(placePos);
        blockBox.max = glm::vec3(placePos) + glm::vec3(1.0f);

        if (!playerBox.intersects(blockBox)) {
            world->setBlock(placePos.x, placePos.y, placePos.z, selectedBlockType);
        }
    }
}