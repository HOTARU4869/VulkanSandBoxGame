//---VulkanApp.h---

#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "backend/vulkan/ivulkan_context.h"

#include <GLFW/glfw3.h>
#include <vector>
#include <optional>
#include <array>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "AppConfig.h"
#include "rhi/rhi_texture.h"
#include "rhi/rhi_vertex.h"

#include "renderer/mesh_data.h"
#include "backend/vulkan/vulkan_texture.h"
#include "Utils.h"


const int MAX_FRAMES_IN_FLIGHT = 2;  // 固定値に変更

class VulkanDevice;
class VulkanSwapChain;
class VulkanPipeline;

class VulkanApp : public IVulkanContext {
public:
    VulkanApp();
    ~VulkanApp();

    // IVulkanContext の実装
    VkDevice         getDevice()         const override { return device; }
    void             waitIdle()                override { vkDeviceWaitIdle(device); }
    VkPipelineLayout getPipelineLayout() const override { return pipelineLayout; }
    VkRenderPass     getRenderPass()     const override { return renderPass; }

    void initWindow();
    void initVulkan();
    void cleanup();

    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    void recordCommandBuffer(VkCommandBuffer commandBuffer,
        uint32_t imageIndex,
        uint32_t currentFrame);

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    void populateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger);

    void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator);

    static void framebufferResizeCallback(GLFWwindow* window,
        int width, int height);
    void recreateSwapChain();
    void cleanupSwapChain();

    void createUniformBuffers();
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();
    void createDepthResources();
    void updateUniformBuffer(uint32_t currentImage,
        const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& viewPos,
        int eyeInWater);
    void createBuffer(VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    uint32_t findMemoryType(uint32_t typeFilter,
        VkMemoryPropertyFlags properties);
    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features);
    void createImage(uint32_t width, uint32_t height,
        VkFormat format, VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format,
        VkImageAspectFlags aspectFlags);

    GLFWwindow* window = nullptr;
    VkDevice    device = VK_NULL_HANDLE;
    VkExtent2D  swapChainExtent;
    AppConfig   config;

    void destroyBuffer(VkBuffer buffer, VkDeviceMemory memory) {
        vkDestroyBuffer(device, buffer, nullptr);
        vkFreeMemory(device, memory, nullptr);
    }

    uint32_t        beginFrame(const glm::mat4& view,
        const glm::mat4& proj,
        const glm::vec3& viewPos,
        int eyeInWater) override;
    void            endFrame(uint32_t imageIndex) override;
    VkCommandBuffer getCommandBuffer(uint32_t imageIndex) override {
        return commandBuffers[imageIndex];
    }

    RHITextureRef createTexture(uint32_t width, uint32_t height,
        const void* pixels) override;
    void          setAtlasTexture(RHITextureRef tex);
    void          transitionImageLayout(VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout);
    void          copyBufferToImage(VkBuffer buffer, VkImage image,
        uint32_t width, uint32_t height);

    RHITextureRef atlasTexture;

private:
    VkBuffer       vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer       indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    uint32_t       indexCount = 0;

    std::vector<VkBuffer>       uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*>          uniformBuffersMapped;

    VkDescriptorSetLayout        descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool             descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    VkImage        depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    VkImageView    depthImageView = VK_NULL_HANDLE;

    uint32_t semaphoreIndex = 0;

    VkInstance               instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR             surface = VK_NULL_HANDLE;
    VkPhysicalDevice         physicalDevice = VK_NULL_HANDLE;
    VkQueue                  graphicsQueue = VK_NULL_HANDLE;
    VkQueue                  presentQueue = VK_NULL_HANDLE;

    VkSwapchainKHR               swapChain = VK_NULL_HANDLE;
    std::vector<VkImage>         swapChainImages;
    VkFormat                     swapChainImageFormat = VK_FORMAT_UNDEFINED;
    std::vector<VkImageView>     swapChainImageViews;
    std::vector<VkFramebuffer>   swapChainFramebuffers;

    VkRenderPass     renderPass = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    // graphicsPipeline と transparentPipeline は削除済み
    // パイプラインは VulkanDevice::createMaterial() が生成・管理する

    VkCommandPool                commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence>     inFlightFences;

    size_t currentFrame = 0;
    bool   framebufferResized = false;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR        capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };

    QueueFamilyIndices      findQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    bool                    isDeviceSuitable(VkPhysicalDevice device);
    bool                    checkDeviceExtensionSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR      chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR        chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D              chooseSwapExtent(
        const VkSurfaceCapabilitiesKHR& capabilities);
};