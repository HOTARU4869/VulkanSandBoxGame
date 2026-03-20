//---Camera.h---

#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 70.0f, 0.0f));

    // カメラの更新
    void processKeyboard(float deltaTime, bool forward, bool backward,
        bool left, bool right, bool up, bool down);
    void processMouseMovement(float xoffset, float yoffset);

    // 行列の取得
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspect) const;

    // カメラ情報の取得
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getFront() const { return front; }
    glm::vec3 getRight() const { return right; }
    glm::vec3 getUp() const { return up; }

    float getYaw() const { return yaw; }
    float getPitch() const { return pitch; }

    // カメラ設定
    void setPosition(const glm::vec3& pos) { position = pos; }
    void setMovementSpeed(float speed) { movementSpeed = speed; }
    void setSensitivity(float sens) { mouseSensitivity = sens; }
    void setFov(float fov) { this->fov = fov; }

private:
    // カメラ属性
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    // オイラー角
    float yaw;
    float pitch;

    // カメラオプション
    float movementSpeed;
    float mouseSensitivity;
    float fov;
    float nearPlane;
    float farPlane;

    // カメラベクトルの更新
    void updateCameraVectors();
};

// Camera実装
inline Camera::Camera(glm::vec3 position)
    : position(position),
    front(glm::vec3(0.0f, 0.0f, -1.0f)),
    worldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
    yaw(-90.0f),
    pitch(0.0f),
    movementSpeed(10.0f),
    mouseSensitivity(0.1f),
    fov(45.0f),
    nearPlane(0.1f),
    farPlane(1000.0f) {
    updateCameraVectors();
}

inline void Camera::processKeyboard(float deltaTime, bool forward, bool backward,
    bool left, bool right, bool up, bool down) {
    float velocity = movementSpeed * deltaTime;

    if (forward)
        position += front * velocity;
    if (backward)
        position -= front * velocity;
    if (left)
        position -= this->right * velocity;
    if (right)
        position += this->right * velocity;
    if (up)
        position += worldUp * velocity;
    if (down)
        position -= worldUp * velocity;
}

inline void Camera::processMouseMovement(float xoffset, float yoffset) {
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // ピッチの制限
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    updateCameraVectors();
}

inline glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

inline glm::mat4 Camera::getProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

inline void Camera::updateCameraVectors() {
    // 新しい前方ベクトルを計算
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);

    // 右ベクトルと上ベクトルを再計算
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}