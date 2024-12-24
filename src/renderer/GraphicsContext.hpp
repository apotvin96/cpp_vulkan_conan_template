#pragma once

#include "Config.hpp"
#include "Types/Buffer.hpp"
#include "Types/Commands.hpp"
#include "Types/Image.hpp"
#include "Types/Pipeline.hpp"
#include "Types/Renderpass.hpp"
#include "Types/Synchronization.hpp"
#include "Types/Texture.hpp"
#include "Window.hpp"

class GraphicsContext {
public:
    GraphicsContext(std::shared_ptr<Window> windowRef, VkInstance instance, VkDevice device,
                    VkPhysicalDevice physicalDevice, VkDebugUtilsMessengerEXT debugMessenger,
                    VkPhysicalDeviceProperties physicalDeviceProperties, VkQueue graphicsQueue,
                    uint32_t graphicsQueueFamily, VkQueue transferQueue,
                    uint32_t transferQueueFamily, VkSurfaceKHR surface);

    ~GraphicsContext();

    void destroy();

    bool isSwapchainResized();

    void waitOnFence(std::shared_ptr<FrameBasedFence> fence, int frameIndexOffset = 0);

    uint32_t newFrame(std::shared_ptr<FrameBasedSemaphore> signalSemaphore);

    void beginRecording(std::shared_ptr<CommandBuffer> commandBuffer);

    void beginRecording(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer);

    void beginSwapchainRenderPass(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer,
                                  uint32_t frameIndex, glm::vec4 clearColor);

    void beginRenderPass(std::shared_ptr<CommandBuffer> commandBuffer,
                         std::shared_ptr<RenderPass> renderPass, uint32_t width, uint32_t height);

    void beginRenderPass(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer,
                         std::shared_ptr<RenderPass> renderPass, uint32_t width, uint32_t height);

    void bindPipeline(std::shared_ptr<CommandBuffer> commandBuffer,
                      std::shared_ptr<Pipeline> pipeline);

    void bindPipeline(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer,
                      std::shared_ptr<Pipeline> pipeline);

    void* mapDescriptorBuffer(std::shared_ptr<DescriptorSet> descriptorSet, uint32_t binding);

    void unmapDescriptorBuffer(std::shared_ptr<DescriptorSet> descriptorSet, uint32_t binding);

    void bindDescriptorSet(std::shared_ptr<CommandBuffer> commandBuffer, uint32_t setIndex,
                           std::shared_ptr<DescriptorSet> descriptorSet);

    void bindDescriptorSet(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer,
                           uint32_t setIndex, std::shared_ptr<DescriptorSet> descriptorSet);

    void pushConstants(std::shared_ptr<CommandBuffer> commandBuffer,
                       std::shared_ptr<Pipeline> pipeline, uint32_t offset, uint32_t size,
                       void* data);

    void pushConstants(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer,
                       std::shared_ptr<Pipeline> pipeline, uint32_t offset, uint32_t size,
                       void* data);

    void bindVertexBuffer(std::shared_ptr<CommandBuffer> commandBuffer,
                          std::shared_ptr<VertexBuffer> vertexBuffer);

    void bindVertexBuffer(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer,
                          std::shared_ptr<VertexBuffer> vertexBuffer);

    void draw(std::shared_ptr<CommandBuffer> commandBuffer, uint32_t vertexCount,
              uint32_t numInstances, uint32_t firstVertex, uint32_t firstInstance);

    void draw(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer, uint32_t vertexCount,
              uint32_t numInstances, uint32_t firstVertex, uint32_t firstInstance);

    void endRenderPass(std::shared_ptr<CommandBuffer> commandBuffer);

    void endRenderPass(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer);

    void transitionRenderPassImages(std::shared_ptr<CommandBuffer> commandBuffer,
                                    std::shared_ptr<RenderPass> renderPass,
                                    ImageLayout initialLayout, ImageLayout finalLayout);

    void copyRenderPassImageToCubemap(std::shared_ptr<CommandBuffer> commandBuffer,
                                      std::shared_ptr<RenderPass> renderPass,
                                      uint32_t attachmentIndex, std::shared_ptr<Texture> cubemap,
                                      uint32_t arrayLayerIndex, uint32_t mipLevel,
                                      uint32_t copyWidth, uint32_t copyHeight);

    void blitRenderPassImageToCubemap(std::shared_ptr<CommandBuffer> commandBuffer,
                                      std::shared_ptr<RenderPass> renderPass,
                                      uint32_t attachmentIndex, std::shared_ptr<Texture> cubemap,
                                      uint32_t arrayLayerIndex, uint32_t mipLevel,
                                      uint32_t copySrcWidth, uint32_t copySrcHeight,
                                      uint32_t copyDstWidth, uint32_t copyDstHeight);

    void endRecording(std::shared_ptr<CommandBuffer> commandBuffer);

    void endRecording(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer);

    void submit(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer,
                std::shared_ptr<FrameBasedSemaphore> waitSemaphore,
                std::shared_ptr<FrameBasedSemaphore> signalSemaphore,
                std::shared_ptr<FrameBasedFence> signalFence);

    void immediateSubmit(std::shared_ptr<CommandBuffer> commandBuffer);

    void present(uint32_t frameIndex, std::shared_ptr<FrameBasedSemaphore> waitSemaphore);

    void waitIdle();

    std::shared_ptr<CommandBuffer> createCommandBuffer();

    std::shared_ptr<FrameBasedCommandBuffer> createFrameBasedCommandBuffer();

    std::shared_ptr<FrameBasedFence> createFrameBasedFence(bool createSignaled);

    std::shared_ptr<FrameBasedSemaphore> createFrameBasedSemaphore();

    std::shared_ptr<RenderPass>
    createRenderPass(std::vector<RenderPassAttachmentDescription> renderPassAttachmentDescriptions,
                     bool useDepthAttachment,
                     RenderPassAttachmentDescription depthAttachmentDescription = {});

    std::shared_ptr<Pipeline> createPipeline(PipelineCreateInfo* pipelineInfo);

    std::shared_ptr<DescriptorSet> createDescriptorSet(std::shared_ptr<Pipeline> pipeline,
                                                       uint32_t setLayoutIndex);

    void descriptorSetAddBuffer(std::shared_ptr<DescriptorSet> descriptorSet, uint32_t binding,
                                DescriptorType type, uint32_t bufferSize);

    void descriptorSetAddImage(std::shared_ptr<DescriptorSet> descriptorSet, uint32_t binding,
                               std::shared_ptr<Texture> image);

    void descriptorSetAddRenderPassAttachment(std::shared_ptr<DescriptorSet> descriptorSet,
                                              uint32_t binding,
                                              std::shared_ptr<RenderPass> renderPass,
                                              uint32_t attachmentIndex);

    std::shared_ptr<VertexBuffer> createVertexBuffer(void* data, uint32_t size);

    std::shared_ptr<Texture> createTexture(int width, int height, int numComponents,
                                           ColorSpace colorSpace, unsigned char* data,
                                           bool genMipmaps = false); // TODO: RGB Textures broken

    std::shared_ptr<Texture> createHDRTexture(int width, int height, int numComponents, float* data,
                                              bool genMipmaps = false); // TODO: RGB Texture broken

    std::shared_ptr<Texture> createCubemap(Format format, uint32_t width, uint32_t height,
                                           bool reserveMipMaps = false);

    static std::unique_ptr<GraphicsContext> create(std::shared_ptr<Window> windowRef);

protected:
private:
    uint32_t getCurrentFrameBasedIndex(int indexOffset = 0);

    void windowResize(int width, int height);

    void initDescriptorPool();

    void initAllocators();

    void initSwapchain();

    void initSwapchainRenderPass();

    void initUploadStructures();

    void initSamplers();

    ShaderModule loadShaderModule(const char* shaderFilePath);

    void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

    std::shared_ptr<Window> windowRef;

    uint32_t numFrames;

    VkInstance instance;
    VkDevice device;
    VkPhysicalDevice physicalDevice;

    VkDebugUtilsMessengerEXT debugMessenger;

    VkPhysicalDeviceProperties physicalDeviceProperties;

    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamily;

    VkQueue transferQueue;
    uint32_t transferQueueFamily;

    VkFence uploadFence;
    VkCommandPool uploadCommandPool;

    VkDescriptorPool globalDescriptorPool;

    VmaAllocator allocator;

    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;
    VkFormat depthFormat;
    AllocatedImage depthImage;
    VkImageView depthImageView;
    VkRenderPass swapchainRenderPass;
    std::vector<VkFramebuffer> swapchainFramebuffers;
    VkExtent2D currentSwapchainExtent;
    bool swapchainResized = false;

    VkSampler mainSampler;

    friend class Window;
};
