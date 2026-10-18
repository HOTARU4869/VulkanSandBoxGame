// vulkan_device.h

#pragma once
#include "../../rhi/rhi_device.h"
#include "../../rhi/rhi_vertex.h"
#include "vulkan_buffer.h"
#include "vulkan_command_list.h"
#include "ivulkan_context.h"
#include "../../renderer/material.h"
#include "../../Utils.h"
#include "vulkan_pipeline.h"
#include <fstream>
#include <stdexcept>
#include <vector>

namespace {
    std::vector<char> vkd_readFile(const std::string& path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open())
            throw std::runtime_error("Failed to open shader file: " + path);
        size_t size = static_cast<size_t>(file.tellg());
        std::vector<char> buf(size);
        file.seekg(0);
        file.read(buf.data(), size);
        return buf;
    }

    VkShaderModule vkd_createShaderModule(VkDevice device,
        const std::vector<char>& code) {
        VkShaderModuleCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.codeSize = code.size();
        ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule mod;
        if (vkCreateShaderModule(device, &ci, nullptr, &mod) != VK_SUCCESS)
            throw std::runtime_error("Failed to create shader module!");
        return mod;
    }
}

class VulkanDevice : public IRHIDevice {
public:
    explicit VulkanDevice(IVulkanContext* ctx) : m_ctx(ctx) {}

    RHIBufferRef createBuffer(const RHIBufferDesc& desc) override {
        VkBufferUsageFlags usage = 0;
        if (desc.isVertex)  usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        if (desc.isIndex)   usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        if (desc.isUniform) usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        VkMemoryPropertyFlags memProps = desc.hostVisible
            ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        auto buf = std::make_shared<VulkanBuffer>();
        buf->device = m_ctx->getDevice();
        buf->size = desc.size;
        m_ctx->createBuffer(desc.size, usage, memProps, buf->buffer, buf->memory);

        if (desc.hostVisible) {
            vkMapMemory(m_ctx->getDevice(), buf->memory, 0, desc.size, 0, &buf->mappedPtr);
        }

        return buf;
    }

    void updateBuffer(RHIBufferRef rhiBuf, const void* data, size_t size) override {
        auto buf = std::static_pointer_cast<VulkanBuffer>(rhiBuf);

        // ★ 既に map 済みなので memcpy するだけ
        if (buf->mappedPtr) {
            memcpy(buf->mappedPtr, data, size);
            return;
        }

        // map されていない場合（DEVICE_LOCAL）はステージングが必要
        // 現状は hostVisible のみ使うので基本ここには来ない
        void* mapped = nullptr;
        vkMapMemory(m_ctx->getDevice(), buf->memory, 0, size, 0, &mapped);
        memcpy(mapped, data, size);
        vkUnmapMemory(m_ctx->getDevice(), buf->memory);
    }

    // フレーム開始 — VulkanApp からコマンドバッファを受け取る
    RHICommandListRef beginFrame(const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& viewPos,
        int eyeInWater) override {
        m_currentImageIndex = m_ctx->beginFrame(view, proj, viewPos, eyeInWater);
        VkCommandBuffer cmd = m_ctx->getCommandBuffer(m_currentImageIndex);

        return std::make_shared<VulkanCommandList>(cmd);
    }

    // フレーム終了 — VulkanApp に submit と present を委譲
    void endFrame(RHICommandListRef) override {
        m_ctx->endFrame(m_currentImageIndex);
    }

	// バックエンドの種類を返す
    RHIBackend getBackend() const override { return RHIBackend::Vulkan; }

	// テクスチャの作成も VulkanApp に委譲
    RHITextureRef createTexture(const RHITextureDesc& desc, const void* pixels) override {
        return m_ctx->createTexture(desc.width, desc.height, pixels);
    }

    Material createMaterial(const MaterialDesc& desc) override {

        auto vert = vkd_readFile(resolvePath(desc.vertShaderPath));
        auto frag = vkd_readFile(resolvePath(desc.fragShaderPath));
        VkShaderModule vertMod = vkd_createShaderModule(m_ctx->getDevice(), vert);
        VkShaderModule fragMod = vkd_createShaderModule(m_ctx->getDevice(), frag);

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertMod;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragMod;
        stages[1].pName = "main";

        auto binding = Vertex::getBindingDescription();
        auto attrs = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &binding;
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
        vertexInput.pVertexAttributeDescriptions = attrs.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        std::vector<VkDynamicState> dynStates = {
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynState{};
        dynState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynState.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
        dynState.pDynamicStates = dynStates.data();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // desc から設定
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = desc.cullBackFace ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        VkPipelineMultisampleStateCreateInfo msaa{};
        msaa.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // desc から設定
        VkPipelineColorBlendAttachmentState blendAttach{};
        blendAttach.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blendAttach.blendEnable = desc.blendEnable ? VK_TRUE : VK_FALSE;
        blendAttach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttach.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        blendAttach.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlend{};
        colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.attachmentCount = 1;
        colorBlend.pAttachments = &blendAttach;

        // desc から設定
        VkPipelineDepthStencilStateCreateInfo depth{};
        depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depth.depthTestEnable = desc.depthTest ? VK_TRUE : VK_FALSE;
        depth.depthWriteEnable = desc.depthWrite ? VK_TRUE : VK_FALSE;
        depth.depthCompareOp = VK_COMPARE_OP_LESS;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = stages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &msaa;
        pipelineInfo.pColorBlendState = &colorBlend;
        pipelineInfo.pDepthStencilState = &depth;
        pipelineInfo.pDynamicState = &dynState;
        pipelineInfo.layout = m_ctx->getPipelineLayout();
        pipelineInfo.renderPass = m_ctx->getRenderPass();
        pipelineInfo.subpass = 0;

        auto vkPipeline = std::make_shared<VulkanPipeline>();
        vkPipeline->device = m_ctx->getDevice();

        if (vkCreateGraphicsPipelines(vkPipeline->device, VK_NULL_HANDLE, 1,
            &pipelineInfo, nullptr, &vkPipeline->pipeline)
            != VK_SUCCESS) {
            throw std::runtime_error("Failed to create material pipeline!");
        }

        vkDestroyShaderModule(vkPipeline->device, vertMod, nullptr);
        vkDestroyShaderModule(vkPipeline->device, fragMod, nullptr);

        Material mat;
        mat.desc = desc;           // レシピも保持
        mat.pipeline = vkPipeline;
        return mat;
    }

    void waitIdle() override { m_ctx->waitIdle(); }
private:
    IVulkanContext* m_ctx;
    uint32_t   m_currentImageIndex = 0;
};