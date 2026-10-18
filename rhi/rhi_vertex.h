//rhi/rhi_vertex.h
#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include "../renderer/mesh_data.h"

struct Vertex {
    float    position[3];
    float    texCoord[2];
    float    normal[3];
    uint32_t blockType;
    uint32_t textureIndex;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bd{};
        bd.binding = 0;
        bd.stride = sizeof(Vertex);
        bd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bd;
    }

    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attrs{};

        attrs[0].binding = 0; attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset = offsetof(Vertex, position);

        attrs[1].binding = 0; attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32G32_SFLOAT;
        attrs[1].offset = offsetof(Vertex, texCoord);

        attrs[2].binding = 0; attrs[2].location = 2;
        attrs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[2].offset = offsetof(Vertex, normal);

        attrs[3].binding = 0; attrs[3].location = 3;
        attrs[3].format = VK_FORMAT_R32_UINT;
        attrs[3].offset = offsetof(Vertex, blockType);

        return attrs;
    }
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec4 viewPos;
};

static_assert(sizeof(Vertex) == sizeof(MeshVertex),
    "Vertex and MeshVertex must have identical memory layout");
static_assert(offsetof(Vertex, position) == offsetof(MeshVertex, position));
static_assert(offsetof(Vertex, texCoord) == offsetof(MeshVertex, texCoord));
static_assert(offsetof(Vertex, normal) == offsetof(MeshVertex, normal));
static_assert(offsetof(Vertex, blockType) == offsetof(MeshVertex, blockType));