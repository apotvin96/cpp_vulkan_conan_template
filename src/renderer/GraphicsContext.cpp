#include "../pch.hpp"
#include "GraphicsContext.hpp"

#include "Helper/Conversions.hpp"
#include "Helper/Debug.hpp"
#include "Helper/Initializers.hpp"
#include "../Logger.hpp"

GraphicsContext::GraphicsContext(std::shared_ptr<Window> windowRef, VkInstance instance,
                                 VkDevice device, VkPhysicalDevice physicalDevice,
                                 VkDebugUtilsMessengerEXT debugMessenger,
                                 VkPhysicalDeviceProperties physicalDeviceProperties,
                                 VkQueue graphicsQueue, uint32_t graphicsQueueFamily,
                                 VkQueue transferQueue, uint32_t transferQueueFamily,
                                 VkSurfaceKHR surface)
    : windowRef(windowRef), instance(instance), device(device), physicalDevice(physicalDevice),
      debugMessenger(debugMessenger), physicalDeviceProperties(physicalDeviceProperties),
      graphicsQueue(graphicsQueue), graphicsQueueFamily(graphicsQueueFamily),
      transferQueue(transferQueue), transferQueueFamily(transferQueueFamily), surface(surface) {

    numFrames = 0;

    glfwSetWindowUserPointer(windowRef->get(), this);

    initDescriptorPool();

    initAllocators();

    initSwapchain();

    initSwapchainRenderPass();

    initUploadStructures();

    initSamplers();
}

GraphicsContext::~GraphicsContext() { destroy(); }

void GraphicsContext::destroy() {
    Logger::renderer_logger->info("Destroying Graphics Context");

    vkDestroySampler(device, mainSampler, nullptr);

    vkDestroyFence(device, uploadFence, nullptr);

    vkDestroyCommandPool(device, uploadCommandPool, nullptr);

    for (auto& swapchainFramebuffer : swapchainFramebuffers) {
        vkDestroyFramebuffer(device, swapchainFramebuffer, nullptr);
    }

    vkDestroyRenderPass(device, swapchainRenderPass, nullptr);

    vkDestroyImageView(device, depthImageView, nullptr);
    vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);

    for (unsigned int i = 0; i < swapchainImageViews.size(); i++) {
        vkDestroyImageView(device, swapchainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);

    vmaDestroyAllocator(allocator);

    vkDestroySurfaceKHR(instance, surface, nullptr);

    vkDestroyDescriptorPool(device, globalDescriptorPool, nullptr);

    vkDestroyDevice(device, nullptr);
    vkb::destroy_debug_utils_messenger(instance, debugMessenger);
    vkDestroyInstance(instance, nullptr);
}

bool GraphicsContext::isSwapchainResized() { return swapchainResized; }

void GraphicsContext::waitOnFence(std::shared_ptr<FrameBasedFence> fence, int frameIndexOffset) {
    uint32_t frameIndex = getCurrentFrameBasedIndex(frameIndexOffset);

    VK_CHECK(vkWaitForFences(device, 1, &fence->fences[frameIndex], true, 1000000000));
    VK_CHECK(vkResetFences(device, 1, &fence->fences[frameIndex]));
}

uint32_t GraphicsContext::newFrame(std::shared_ptr<FrameBasedSemaphore> signalSemaphore) {
    if (swapchainResized) {
        swapchainResized = false;
    }

    uint32_t swapchainImageIndex;
    VkResult getSwapchainResult = vkAcquireNextImageKHR(
        device, swapchain, 1000000000, signalSemaphore->semaphores[getCurrentFrameBasedIndex()],
        nullptr, &swapchainImageIndex);

    if (getSwapchainResult == VK_ERROR_OUT_OF_DATE_KHR || getSwapchainResult == VK_SUBOPTIMAL_KHR) {
        Logger::renderer_logger->info("Acquire Image: Swapchain out of date");

        vkDeviceWaitIdle(device);

        for (auto& swapchainFramebuffer : swapchainFramebuffers) {
            vkDestroyFramebuffer(device, swapchainFramebuffer, nullptr);
        }

        vkDestroyRenderPass(device, swapchainRenderPass, nullptr);

        vkDestroyImageView(device, depthImageView, nullptr);
        vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);

        for (unsigned int i = 0; i < swapchainImageViews.size(); i++) {
            vkDestroyImageView(device, swapchainImageViews[i], nullptr);
        }

        vkDestroySwapchainKHR(device, swapchain, nullptr);

        initSwapchain();

        initSwapchainRenderPass();

        swapchainResized = true;
    }

    return swapchainImageIndex;
}

void GraphicsContext::beginRecording(std::shared_ptr<CommandBuffer> commandBuffer) {
    VK_CHECK(vkResetCommandBuffer(commandBuffer->commandBuffer, 0));
    VkCommandBufferBeginInfo cmdBeginInfo =
        helper::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(commandBuffer->commandBuffer, &cmdBeginInfo));
}

void GraphicsContext::beginRecording(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer) {
    uint32_t frameIndex = getCurrentFrameBasedIndex();

    VK_CHECK(vkResetCommandBuffer(commandBuffer->commandBuffers[frameIndex], 0));
    VkCommandBufferBeginInfo cmdBeginInfo =
        helper::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(commandBuffer->commandBuffers[frameIndex], &cmdBeginInfo));
}

void GraphicsContext::beginSwapchainRenderPass(
    std::shared_ptr<FrameBasedCommandBuffer> commandBuffer, uint32_t frameIndex,
    glm::vec4 clearColor) {
    VkClearValue clearValue;
    clearValue.color = { { clearColor.r, clearColor.g, clearColor.b, clearColor.a } };

    VkClearValue depthClear;
    depthClear.depthStencil.depth = 1.0f;

    VkClearValue clearValues[] = { clearValue, depthClear };

    VkRenderPassBeginInfo rpBeginInfo = {};
    rpBeginInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.pNext                 = nullptr;
    rpBeginInfo.renderPass            = swapchainRenderPass;
    rpBeginInfo.renderArea.offset.x   = 0;
    rpBeginInfo.renderArea.offset.y   = 0;
    rpBeginInfo.renderArea.extent     = currentSwapchainExtent;
    rpBeginInfo.framebuffer           = swapchainFramebuffers[frameIndex];
    rpBeginInfo.clearValueCount       = 2;
    rpBeginInfo.pClearValues          = &clearValues[0];

    vkCmdBeginRenderPass(commandBuffer->commandBuffers[getCurrentFrameBasedIndex()], &rpBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
}

void GraphicsContext::beginRenderPass(std::shared_ptr<CommandBuffer> commandBuffer,
                                      std::shared_ptr<RenderPass> renderPass, uint32_t width,
                                      uint32_t height) {
    std::vector<VkClearValue> clearValues(renderPass->images.size());
    for (auto& clearValue : clearValues) {
        clearValue.color              = { { 0.0f, 0.0f, 0.0f, 1.0f } };
        clearValue.depthStencil.depth = 1.0f;
    }

    VkRenderPassBeginInfo rpBeginInfo = {};
    rpBeginInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.pNext                 = nullptr;
    rpBeginInfo.renderPass            = renderPass->renderPass;
    rpBeginInfo.renderArea.offset.x   = 0;
    rpBeginInfo.renderArea.offset.y   = 0;
    VkExtent2D extent;
    extent.width                  = width;
    extent.height                 = height;
    rpBeginInfo.renderArea.extent = extent;
    rpBeginInfo.framebuffer       = renderPass->framebuffer;
    rpBeginInfo.clearValueCount   = (uint32_t)clearValues.size();
    rpBeginInfo.pClearValues      = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer->commandBuffer, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void GraphicsContext::beginRenderPass(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer,
                                      std::shared_ptr<RenderPass> renderPass, uint32_t width,
                                      uint32_t height) {
    std::vector<VkClearValue> clearValues(renderPass->images.size());
    for (auto& clearValue : clearValues) {
        clearValue.color              = { { 0.0f, 0.0f, 0.0f, 1.0f } };
        clearValue.depthStencil.depth = 1.0f;
    }

    VkRenderPassBeginInfo rpBeginInfo = {};
    rpBeginInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBeginInfo.pNext                 = nullptr;
    rpBeginInfo.renderPass            = renderPass->renderPass;
    rpBeginInfo.renderArea.offset.x   = 0;
    rpBeginInfo.renderArea.offset.y   = 0;
    VkExtent2D extent;
    extent.width                  = width;
    extent.height                 = height;
    rpBeginInfo.renderArea.extent = extent;
    rpBeginInfo.framebuffer       = renderPass->framebuffer;
    rpBeginInfo.clearValueCount   = (uint32_t)clearValues.size();
    rpBeginInfo.pClearValues      = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer->commandBuffers[getCurrentFrameBasedIndex()], &rpBeginInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
}

void GraphicsContext::bindPipeline(std::shared_ptr<CommandBuffer> commandBuffer,
                                   std::shared_ptr<Pipeline> pipeline) {
    vkCmdBindPipeline(commandBuffer->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipeline->pipeline);
}

void GraphicsContext::bindPipeline(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer,
                                   std::shared_ptr<Pipeline> pipeline) {
    vkCmdBindPipeline(commandBuffer->commandBuffers[getCurrentFrameBasedIndex()],
                      VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
}

void* GraphicsContext::mapDescriptorBuffer(std::shared_ptr<DescriptorSet> descriptorSet,
                                           uint32_t binding) {
    uint32_t index           = getCurrentFrameBasedIndex();
    VmaAllocation allocation = descriptorSet->allocations[index][binding];
    void* data;
    vmaMapMemory(allocator, allocation, &data);

    return data;
}

void GraphicsContext::unmapDescriptorBuffer(std::shared_ptr<DescriptorSet> descriptorSet,
                                            uint32_t binding) {
    vmaUnmapMemory(allocator, descriptorSet->allocations[getCurrentFrameBasedIndex()][binding]);
}

void GraphicsContext::bindDescriptorSet(std::shared_ptr<CommandBuffer> commandBuffer,
                                        uint32_t setIndex,
                                        std::shared_ptr<DescriptorSet> descriptorSet) {
    vkCmdBindDescriptorSets(commandBuffer->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            descriptorSet->pipelineLayout, setIndex, 1,
                            &descriptorSet->descriptorSets[getCurrentFrameBasedIndex()], 0,
                            nullptr);
}

void GraphicsContext::bindDescriptorSet(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer,
                                        uint32_t setIndex,
                                        std::shared_ptr<DescriptorSet> descriptorSet) {
    uint32_t frameIndex = getCurrentFrameBasedIndex();
    vkCmdBindDescriptorSets(commandBuffer->commandBuffers[frameIndex],
                            VK_PIPELINE_BIND_POINT_GRAPHICS, descriptorSet->pipelineLayout,
                            setIndex, 1, &descriptorSet->descriptorSets[frameIndex], 0, nullptr);
}

void GraphicsContext::pushConstants(std::shared_ptr<CommandBuffer> commandBuffer,
                                    std::shared_ptr<Pipeline> pipeline, uint32_t offset,
                                    uint32_t size, void* data) {
    vkCmdPushConstants(commandBuffer->commandBuffer, pipeline->layout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, size, data);
}

void GraphicsContext::pushConstants(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer,
                                    std::shared_ptr<Pipeline> pipeline, uint32_t offset,
                                    uint32_t size, void* data) {
    vkCmdPushConstants(commandBuffer->commandBuffers[getCurrentFrameBasedIndex()], pipeline->layout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, size, data);
}

void GraphicsContext::bindVertexBuffer(std::shared_ptr<CommandBuffer> commandBuffer,
                                       std::shared_ptr<VertexBuffer> vertexBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer->commandBuffer, 0, 1, &vertexBuffer->buffer, &offset);
}

void GraphicsContext::bindVertexBuffer(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer,
                                       std::shared_ptr<VertexBuffer> vertexBuffer) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer->commandBuffers[getCurrentFrameBasedIndex()], 0, 1,
                           &vertexBuffer->buffer, &offset);
}

void GraphicsContext::draw(std::shared_ptr<CommandBuffer> commandBuffer, uint32_t vertexCount,
                           uint32_t numInstances, uint32_t firstVertex, uint32_t firstInstance) {
    vkCmdDraw(commandBuffer->commandBuffer, vertexCount, numInstances, firstVertex, firstInstance);
}

void GraphicsContext::draw(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer,
                           uint32_t vertexCount, uint32_t numInstances, uint32_t firstVertex,
                           uint32_t firstInstance) {
    vkCmdDraw(commandBuffer->commandBuffers[getCurrentFrameBasedIndex()], vertexCount, numInstances,
              firstVertex, firstInstance);
}

void GraphicsContext::endRenderPass(std::shared_ptr<CommandBuffer> commandBuffer) {
    vkCmdEndRenderPass(commandBuffer->commandBuffer);
}

void GraphicsContext::endRenderPass(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer) {
    vkCmdEndRenderPass(commandBuffer->commandBuffers[getCurrentFrameBasedIndex()]);
}

void GraphicsContext::transitionRenderPassImages(std::shared_ptr<CommandBuffer> commandBuffer,
                                                 std::shared_ptr<RenderPass> renderPass,
                                                 ImageLayout initialLayout,
                                                 ImageLayout finalLayout) {
    VkImageSubresourceRange range;
    range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel   = 0;
    range.levelCount     = 1;
    range.baseArrayLayer = 0;
    range.layerCount     = 1;

    VkImageMemoryBarrier imageBarrier_toTransfer = {};
    imageBarrier_toTransfer.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

    imageBarrier_toTransfer.oldLayout        = helper::getVkImageLayout(initialLayout);
    imageBarrier_toTransfer.newLayout        = helper::getVkImageLayout(finalLayout);
    imageBarrier_toTransfer.image            = renderPass->images[0]; // TODO: DO for all of them
    imageBarrier_toTransfer.subresourceRange = range;

    imageBarrier_toTransfer.srcAccessMask =
        helper::getVkAccessFlags(initialLayout, AccessType::SRC, false);
    imageBarrier_toTransfer.dstAccessMask =
        helper::getVkAccessFlags(finalLayout, AccessType::DST, false);

    // barrier the image into the transfer-receive layout
    vkCmdPipelineBarrier(commandBuffer->commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageBarrier_toTransfer);
}

void GraphicsContext::copyRenderPassImageToCubemap(std::shared_ptr<CommandBuffer> commandBuffer,
                                                   std::shared_ptr<RenderPass> renderPass,
                                                   uint32_t attachmentIndex,
                                                   std::shared_ptr<Texture> cubemap,
                                                   uint32_t arrayLayerIndex, uint32_t mipLevel,
                                                   uint32_t copyWidth, uint32_t copyHeight) {
    VkImageSubresourceRange range;
    range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel   = mipLevel;
    range.levelCount     = 1;
    range.baseArrayLayer = arrayLayerIndex;
    range.layerCount     = 1;

    VkImageMemoryBarrier imageBarrier_toTransfer = {};
    imageBarrier_toTransfer.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

    imageBarrier_toTransfer.oldLayout        = helper::getVkImageLayout(ImageLayout::SHADER_READ);
    imageBarrier_toTransfer.newLayout        = helper::getVkImageLayout(ImageLayout::TRANSFER_DST);
    imageBarrier_toTransfer.image            = cubemap->image;
    imageBarrier_toTransfer.subresourceRange = range;

    imageBarrier_toTransfer.srcAccessMask =
        helper::getVkAccessFlags(ImageLayout::SHADER_READ, AccessType::SRC, false);
    imageBarrier_toTransfer.dstAccessMask =
        helper::getVkAccessFlags(ImageLayout::TRANSFER_DST, AccessType::DST, false);

    // barrier the image into the transfer-receive layout
    vkCmdPipelineBarrier(commandBuffer->commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageBarrier_toTransfer);

    VkImageCopy copyRegion                   = {};
    copyRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.mipLevel       = 0;
    copyRegion.srcSubresource.layerCount     = 1;
    copyRegion.srcOffset                     = { 0, 0, 0 };

    copyRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.baseArrayLayer = arrayLayerIndex;
    copyRegion.dstSubresource.mipLevel       = mipLevel;
    copyRegion.dstSubresource.layerCount     = 1;
    copyRegion.dstOffset                     = { 0, 0, 0 };

    copyRegion.extent.width  = copyWidth;
    copyRegion.extent.height = copyHeight;
    copyRegion.extent.depth  = 1;

    vkCmdCopyImage(commandBuffer->commandBuffer, renderPass->images[attachmentIndex],
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cubemap->image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel   = 0;
    range.levelCount     = 1;
    range.baseArrayLayer = 0;
    range.layerCount     = 1;

    imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

    imageBarrier_toTransfer.oldLayout        = helper::getVkImageLayout(ImageLayout::TRANSFER_SRC);
    imageBarrier_toTransfer.newLayout        = helper::getVkImageLayout(ImageLayout::ATTACHMENT);
    imageBarrier_toTransfer.image            = renderPass->images[attachmentIndex];
    imageBarrier_toTransfer.subresourceRange = range;

    imageBarrier_toTransfer.srcAccessMask =
        helper::getVkAccessFlags(ImageLayout::TRANSFER_SRC, AccessType::SRC, false);
    imageBarrier_toTransfer.dstAccessMask =
        helper::getVkAccessFlags(ImageLayout::ATTACHMENT, AccessType::DST, false);

    // barrier the image into the transfer-receive layout
    vkCmdPipelineBarrier(commandBuffer->commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageBarrier_toTransfer);

    range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel   = mipLevel;
    range.levelCount     = 1;
    range.baseArrayLayer = arrayLayerIndex;
    range.layerCount     = 1;

    imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

    imageBarrier_toTransfer.oldLayout        = helper::getVkImageLayout(ImageLayout::TRANSFER_DST);
    imageBarrier_toTransfer.newLayout        = helper::getVkImageLayout(ImageLayout::SHADER_READ);
    imageBarrier_toTransfer.image            = cubemap->image;
    imageBarrier_toTransfer.subresourceRange = range;

    imageBarrier_toTransfer.srcAccessMask =
        helper::getVkAccessFlags(ImageLayout::TRANSFER_DST, AccessType::SRC, false);
    imageBarrier_toTransfer.dstAccessMask =
        helper::getVkAccessFlags(ImageLayout::SHADER_READ, AccessType::DST, false);

    // barrier the image into the transfer-receive layout
    vkCmdPipelineBarrier(commandBuffer->commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageBarrier_toTransfer);
}

void GraphicsContext::blitRenderPassImageToCubemap(std::shared_ptr<CommandBuffer> commandBuffer,
                                                   std::shared_ptr<RenderPass> renderPass,
                                                   uint32_t attachmentIndex,
                                                   std::shared_ptr<Texture> cubemap,
                                                   uint32_t arrayLayerIndex, uint32_t mipLevel,
                                                   uint32_t copySrcWidth, uint32_t copySrcHeight,
                                                   uint32_t copyDstWidth, uint32_t copyDstHeight) {
    VkImageSubresourceRange range;
    range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel   = mipLevel;
    range.levelCount     = 1;
    range.baseArrayLayer = arrayLayerIndex;
    range.layerCount     = 1;

    VkImageMemoryBarrier imageBarrier_toTransfer = {};
    imageBarrier_toTransfer.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

    imageBarrier_toTransfer.oldLayout        = helper::getVkImageLayout(ImageLayout::SHADER_READ);
    imageBarrier_toTransfer.newLayout        = helper::getVkImageLayout(ImageLayout::TRANSFER_DST);
    imageBarrier_toTransfer.image            = cubemap->image;
    imageBarrier_toTransfer.subresourceRange = range;

    imageBarrier_toTransfer.srcAccessMask =
        helper::getVkAccessFlags(ImageLayout::SHADER_READ, AccessType::SRC, false);
    imageBarrier_toTransfer.dstAccessMask =
        helper::getVkAccessFlags(ImageLayout::TRANSFER_DST, AccessType::DST, false);

    // barrier the image into the transfer-receive layout
    vkCmdPipelineBarrier(commandBuffer->commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageBarrier_toTransfer);
    /*
    VkImageCopy copyRegion                   = {};
    copyRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.srcSubresource.baseArrayLayer = 0;
    copyRegion.srcSubresource.mipLevel       = 0;
    copyRegion.srcSubresource.layerCount     = 1;
    copyRegion.srcOffset                     = { 0, 0, 0 };

    copyRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.dstSubresource.baseArrayLayer = arrayLayerIndex;
    copyRegion.dstSubresource.mipLevel       = mipLevel;
    copyRegion.dstSubresource.layerCount     = 1;
    copyRegion.dstOffset                     = { 0, 0, 0 };

    copyRegion.extent.width  = copyWidth;
    copyRegion.extent.height = copyHeight;
    copyRegion.extent.depth  = 1;

    vkCmdCopyImage(commandBuffer->commandBuffer, renderPass->images[attachmentIndex],
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cubemap->image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
                   */

    VkImageBlit imageBlit                   = {};
    imageBlit.srcOffsets[0]                 = { 0, 0, 0 };
    imageBlit.srcOffsets[1]                 = { (int)copySrcWidth, (int)copySrcHeight, 1 };
    imageBlit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlit.srcSubresource.baseArrayLayer = 0;
    imageBlit.srcSubresource.layerCount     = 1;
    imageBlit.srcSubresource.mipLevel       = 0;
    imageBlit.dstOffsets[0]                 = { 0, 0, 0 };
    imageBlit.dstOffsets[1]                 = { (int)copyDstWidth, (int)copyDstHeight, 1 };
    imageBlit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    imageBlit.dstSubresource.baseArrayLayer = arrayLayerIndex;
    imageBlit.dstSubresource.layerCount     = 1;
    imageBlit.dstSubresource.mipLevel       = mipLevel;

    vkCmdBlitImage(commandBuffer->commandBuffer, renderPass->images[attachmentIndex],
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cubemap->image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

    range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel   = 0;
    range.levelCount     = 1;
    range.baseArrayLayer = 0;
    range.layerCount     = 1;

    imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

    imageBarrier_toTransfer.oldLayout        = helper::getVkImageLayout(ImageLayout::TRANSFER_SRC);
    imageBarrier_toTransfer.newLayout        = helper::getVkImageLayout(ImageLayout::ATTACHMENT);
    imageBarrier_toTransfer.image            = renderPass->images[attachmentIndex];
    imageBarrier_toTransfer.subresourceRange = range;

    imageBarrier_toTransfer.srcAccessMask =
        helper::getVkAccessFlags(ImageLayout::TRANSFER_SRC, AccessType::SRC, false);
    imageBarrier_toTransfer.dstAccessMask =
        helper::getVkAccessFlags(ImageLayout::ATTACHMENT, AccessType::DST, false);

    // barrier the image into the transfer-receive layout
    vkCmdPipelineBarrier(commandBuffer->commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageBarrier_toTransfer);

    range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel   = mipLevel;
    range.levelCount     = 1;
    range.baseArrayLayer = arrayLayerIndex;
    range.layerCount     = 1;

    imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

    imageBarrier_toTransfer.oldLayout        = helper::getVkImageLayout(ImageLayout::TRANSFER_DST);
    imageBarrier_toTransfer.newLayout        = helper::getVkImageLayout(ImageLayout::SHADER_READ);
    imageBarrier_toTransfer.image            = cubemap->image;
    imageBarrier_toTransfer.subresourceRange = range;

    imageBarrier_toTransfer.srcAccessMask =
        helper::getVkAccessFlags(ImageLayout::TRANSFER_DST, AccessType::SRC, false);
    imageBarrier_toTransfer.dstAccessMask =
        helper::getVkAccessFlags(ImageLayout::SHADER_READ, AccessType::DST, false);

    // barrier the image into the transfer-receive layout
    vkCmdPipelineBarrier(commandBuffer->commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                         &imageBarrier_toTransfer);
}

void GraphicsContext::endRecording(std::shared_ptr<CommandBuffer> commandBuffer) {
    VK_CHECK(vkEndCommandBuffer(commandBuffer->commandBuffer));
}

void GraphicsContext::endRecording(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer) {
    uint32_t frameIndex = getCurrentFrameBasedIndex();

    VK_CHECK(vkEndCommandBuffer(commandBuffer->commandBuffers[frameIndex]));
}

void GraphicsContext::submit(std::shared_ptr<FrameBasedCommandBuffer> commandBuffer,
                             std::shared_ptr<FrameBasedSemaphore> waitSemaphore,
                             std::shared_ptr<FrameBasedSemaphore> signalSemaphore,
                             std::shared_ptr<FrameBasedFence> signalFence) {
    uint32_t frameIndex = getCurrentFrameBasedIndex();

    VkSubmitInfo submit = {};
    submit.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.pNext        = nullptr;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit.pWaitDstStageMask       = &waitStage;

    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores    = &waitSemaphore->semaphores[frameIndex];

    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores    = &signalSemaphore->semaphores[frameIndex];

    submit.commandBufferCount = 1;
    submit.pCommandBuffers    = &commandBuffer->commandBuffers[frameIndex];

    VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, signalFence->fences[frameIndex]));
}

void GraphicsContext::immediateSubmit(std::shared_ptr<CommandBuffer> commandBuffer) {
    VkSubmitInfo submit = helper::submitInfo(&commandBuffer->commandBuffer);

    auto fence = createFrameBasedFence(false);

    // submit command buffer to the queue and execute it.
    // _uploadFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, fence->fences[0]));

    vkWaitForFences(device, 1, &fence->fences[0], true, 9999999999);
    vkResetFences(device, 1, &fence->fences[0]);

    // clear the command pool. This will free the command buffer too
    vkResetCommandPool(device, commandBuffer->commandPool, 0);
}

void GraphicsContext::present(uint32_t frameIndex,
                              std::shared_ptr<FrameBasedSemaphore> waitSemaphore) {
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext            = nullptr;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains    = &swapchain;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &waitSemaphore->semaphores[getCurrentFrameBasedIndex()];

    presentInfo.pImageIndices = &frameIndex;

    VkResult result = vkQueuePresentKHR(graphicsQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        Logger::renderer_logger->info("Present: Swapchain out of date");

        vkDeviceWaitIdle(device);

        for (auto& swapchainFramebuffer : swapchainFramebuffers) {
            vkDestroyFramebuffer(device, swapchainFramebuffer, nullptr);
        }

        vkDestroyRenderPass(device, swapchainRenderPass, nullptr);

        vkDestroyImageView(device, depthImageView, nullptr);
        vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);

        for (unsigned int i = 0; i < swapchainImageViews.size(); i++) {
            vkDestroyImageView(device, swapchainImageViews[i], nullptr);
        }

        vkDestroySwapchainKHR(device, swapchain, nullptr);

        initSwapchain();

        initSwapchainRenderPass();

        swapchainResized = true;
    }

    numFrames++;
}

void GraphicsContext::waitIdle() { vkDeviceWaitIdle(device); }

std::shared_ptr<CommandBuffer> GraphicsContext::createCommandBuffer() {
    VkCommandPool commandPool;

    VkCommandPoolCreateInfo commandPoolCreateInfo = helper::commandPoolCreateInfo(
        graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VK_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool));

    VkCommandBufferAllocateInfo commandAllocInfo =
        helper::commandBufferAllocateInfo(commandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VkCommandBuffer commandBuffer;
    VK_CHECK(vkAllocateCommandBuffers(device, &commandAllocInfo, &commandBuffer));

    return std::make_shared<CommandBuffer>(device, commandPool, commandBuffer);
}

std::shared_ptr<FrameBasedCommandBuffer> GraphicsContext::createFrameBasedCommandBuffer() {
    std::array<VkCommandPool, FRAME_OVERLAP> commandPools;
    std::array<VkCommandBuffer, FRAME_OVERLAP> commandBuffers;

    VkCommandPoolCreateInfo commandPoolCreateInfo = helper::commandPoolCreateInfo(
        graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPools[i]));

        VkCommandBufferAllocateInfo commandAllocInfo =
            helper::commandBufferAllocateInfo(commandPools[i], 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

        VK_CHECK(vkAllocateCommandBuffers(device, &commandAllocInfo, &commandBuffers[i]));
    }

    return std::make_shared<FrameBasedCommandBuffer>(device, commandPools, commandBuffers);
}

std::shared_ptr<FrameBasedFence> GraphicsContext::createFrameBasedFence(bool createSignaled) {
    VkFenceCreateInfo fenceCreateInfo =
        helper::fenceCreateInfo(createSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0U);

    std::array<VkFence, FRAME_OVERLAP> fences;
    for (auto& fence : fences) {
        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence));
    }

    return std::make_shared<FrameBasedFence>(device, fences);
}

std::shared_ptr<FrameBasedSemaphore> GraphicsContext::createFrameBasedSemaphore() {
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext                 = nullptr;
    semaphoreCreateInfo.flags                 = 0;

    std::array<VkSemaphore, FRAME_OVERLAP> semaphores;
    for (auto& semaphore : semaphores) {
        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore));
    }

    return std::make_shared<FrameBasedSemaphore>(device, semaphores);
}

std::shared_ptr<RenderPass> GraphicsContext::createRenderPass(
    std::vector<RenderPassAttachmentDescription> renderPassAttachmentDescriptions,
    bool useDepthAttachment, RenderPassAttachmentDescription depthAttachmentDescription) {

    std::vector<VkAttachmentDescription> colorDescriptions;
    std::vector<VkAttachmentReference> colorReferences;
    for (int i = 0; i < renderPassAttachmentDescriptions.size(); i++) {
        VkAttachmentDescription colorDescription = {};
        colorDescription.flags                   = 0;
        colorDescription.format  = helper::getVkFormat(renderPassAttachmentDescriptions[i].format);
        colorDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        colorDescription.loadOp =
            helper::getVkAttachmentLoadOp(renderPassAttachmentDescriptions[i].loadOp);
        colorDescription.storeOp =
            helper::getVkAttachmentStoreOp(renderPassAttachmentDescriptions[i].storeOp);
        colorDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorDescription.initialLayout =
            helper::getVkImageLayout(renderPassAttachmentDescriptions[i].initialLayout);
        colorDescription.finalLayout =
            helper::getVkImageLayout(renderPassAttachmentDescriptions[i].finalLayout);
        colorDescriptions.push_back(colorDescription);

        VkAttachmentReference colorReference = {};
        colorReference.attachment            = i;
        colorReference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorReferences.push_back(colorReference);
    }

    VkAttachmentDescription depthStencilDescription = {};
    VkAttachmentReference depthStencilReference     = {};
    if (useDepthAttachment) {
        depthStencilDescription.flags   = 0;
        depthStencilDescription.format  = helper::getVkFormat(depthAttachmentDescription.format);
        depthStencilDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        depthStencilDescription.loadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthStencilDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthStencilDescription.stencilLoadOp =
            helper::getVkAttachmentLoadOp(depthAttachmentDescription.loadOp);
        depthStencilDescription.stencilStoreOp =
            helper::getVkAttachmentStoreOp(depthAttachmentDescription.storeOp);
        depthStencilDescription.initialLayout =
            helper::getVkImageLayout(depthAttachmentDescription.initialLayout);
        depthStencilDescription.finalLayout =
            helper::getVkImageLayout(depthAttachmentDescription.finalLayout);

        depthStencilReference.attachment = (uint32_t)colorReferences.size();
        depthStencilReference.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpassDescription = {};
    subpassDescription.flags                = 0;
    subpassDescription.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments    = nullptr;
    subpassDescription.colorAttachmentCount = (uint32_t)colorReferences.size();
    subpassDescription.pColorAttachments    = colorReferences.data();
    subpassDescription.pResolveAttachments  = nullptr;
    subpassDescription.pDepthStencilAttachment =
        useDepthAttachment ? &depthStencilReference : nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments    = nullptr;

    std::vector<VkAttachmentDescription> allAttachmentDescriptions = colorDescriptions;
    if (useDepthAttachment) {
        allAttachmentDescriptions.push_back(depthStencilDescription);
    }

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext                  = nullptr;
    renderPassCreateInfo.flags                  = 0;
    renderPassCreateInfo.attachmentCount        = (uint32_t)allAttachmentDescriptions.size();
    renderPassCreateInfo.pAttachments           = allAttachmentDescriptions.data();
    renderPassCreateInfo.subpassCount           = 1;
    renderPassCreateInfo.pSubpasses             = &subpassDescription;
    renderPassCreateInfo.dependencyCount        = 0;
    renderPassCreateInfo.pDependencies          = nullptr;

    VkRenderPass renderPass;
    VK_CHECK(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &renderPass));

    std::vector<VkImage> framebufferImages;
    std::vector<VmaAllocation> framebufferImageAllocations;
    std::vector<VkImageView> framebufferImageViews;
    for (int i = 0; i < allAttachmentDescriptions.size(); i++) {
        bool isColorAttachment = true;
        if (i >= renderPassAttachmentDescriptions.size()) {
            isColorAttachment = false;
        }

        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.pNext             = nullptr;
        imageCreateInfo.flags             = 0;
        imageCreateInfo.imageType         = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format            = allAttachmentDescriptions[i].format;
        imageCreateInfo.extent            = isColorAttachment
                       ? VkExtent3D{ renderPassAttachmentDescriptions[i].width,
                          renderPassAttachmentDescriptions[i].height, 1 }
                       : VkExtent3D{ depthAttachmentDescription.width, depthAttachmentDescription.height, 1 };
        imageCreateInfo.mipLevels         = 1;
        imageCreateInfo.arrayLayers       = 1;
        imageCreateInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage       = isColorAttachment ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                                                        : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.queueFamilyIndexCount = 0;
        imageCreateInfo.pQueueFamilyIndices   = nullptr;
        imageCreateInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo imageAllocationInfo = {};
        imageAllocationInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;

        VkImage image;
        VmaAllocation allocation;

        VK_CHECK(vmaCreateImage(allocator, &imageCreateInfo, &imageAllocationInfo, &image,
                                &allocation, nullptr));

        framebufferImages.push_back(image);
        framebufferImageAllocations.push_back(allocation);

        VkImageViewCreateInfo imageViewCreateInfo = helper::imageViewCreateInfo(
            allAttachmentDescriptions[i].format, image,
            isColorAttachment ? VK_IMAGE_ASPECT_COLOR_BIT : VK_IMAGE_ASPECT_DEPTH_BIT);
        VkImageView imageView;
        VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView));

        framebufferImageViews.push_back(imageView);
    }

    VkFramebufferCreateInfo framebufferCreateInfo = {};
    framebufferCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.pNext                   = nullptr;
    framebufferCreateInfo.flags                   = 0;
    framebufferCreateInfo.renderPass              = renderPass;
    framebufferCreateInfo.attachmentCount         = (uint32_t)framebufferImageViews.size();
    framebufferCreateInfo.pAttachments            = framebufferImageViews.data();
    framebufferCreateInfo.width                   = renderPassAttachmentDescriptions[0].width;
    framebufferCreateInfo.height                  = renderPassAttachmentDescriptions[0].height;
    framebufferCreateInfo.layers                  = 1;

    VkFramebuffer framebuffer;
    VK_CHECK(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffer));

    return std::make_shared<RenderPass>(device, allocator, renderPass, framebuffer,
                                        framebufferImages, framebufferImageAllocations,
                                        framebufferImageViews);
}

int getDescriptorSetIndex(ShaderModule& shaderModule, uint32_t setNumber) {
    for (int i = 0; i < shaderModule.reflectionData.descriptorSets.size(); i++) {
        if (shaderModule.reflectionData.descriptorSets[i].set_number == setNumber) {
            return i;
        }
    }

    return -1;
}

uint32_t getMaxSet(std::vector<DescriptorSetLayoutData>& sets) {
    uint32_t max = 0;
    for (auto& set : sets) {
        if (set.set_number >= max) {
            max = set.set_number + 1; // Indices to count
        }
    }

    return max;
}

uint32_t getMaxBinding(uint32_t currentMax, ShaderModule& shaderModule, uint32_t currentSet) {
    uint32_t setIndex = getDescriptorSetIndex(shaderModule, currentSet);

    if (setIndex != -1) {
        for (auto& binding : shaderModule.reflectionData.descriptorSets[setIndex].bindings) {
            if (binding.binding >= currentMax) {
                currentMax = binding.binding + 1;
            }
        }
    }

    return currentMax;
}

std::shared_ptr<Pipeline> GraphicsContext::createPipeline(PipelineCreateInfo* pipelineCreateInfo) {
    // Create the shader modules
    ShaderModule vertexShaderModule   = loadShaderModule(pipelineCreateInfo->vertexShaderPath);
    ShaderModule fragmentShaderModule = loadShaderModule(pipelineCreateInfo->fragmentShaderPath);

    std::vector<VkPushConstantRange> combinedPushConstants =
        vertexShaderModule.reflectionData.pushConstants;

    for (auto& fragConstant : fragmentShaderModule.reflectionData.pushConstants) {
        bool exists = false;
        for (auto& constant : combinedPushConstants) {
            if (fragConstant.offset == constant.offset && fragConstant.size == constant.size) {
                if (fragConstant.stageFlags != constant.stageFlags) {
                    constant.stageFlags |= fragConstant.stageFlags;
                }
                exists = true;
            }
        }

        if (!exists) {
            combinedPushConstants.push_back(fragConstant);
        }
    }

    uint32_t maxDescriptorSetCount =
        std::max(getMaxSet(vertexShaderModule.reflectionData.descriptorSets),
                 getMaxSet(fragmentShaderModule.reflectionData.descriptorSets));

    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    for (unsigned int i = 0; i < maxDescriptorSetCount; i++) {
        VkDescriptorSetLayoutCreateInfo setLayoutInfo = {};
        setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        setLayoutInfo.pNext = nullptr;
        setLayoutInfo.flags = 0;

        uint32_t maxBindingCount = 0;
        maxBindingCount          = getMaxBinding(maxBindingCount, vertexShaderModule, i);
        maxBindingCount          = getMaxBinding(maxBindingCount, fragmentShaderModule, i);

        int vertexShaderSetIndex   = getDescriptorSetIndex(vertexShaderModule, i);
        int fragmentShaderSetIndex = getDescriptorSetIndex(fragmentShaderModule, i);

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        for (unsigned int j = 0; j < maxBindingCount; j++) {
            bool haveIt = false;
            if (vertexShaderSetIndex != -1) {
                for (auto& binding :
                     vertexShaderModule.reflectionData.descriptorSets[vertexShaderSetIndex]
                         .bindings) {
                    if (binding.binding == j) {
                        if (!haveIt) {
                            bindings.push_back(binding);
                            haveIt = true;
                        } else {
                            bindings[j].stageFlags |= binding.stageFlags;
                        }
                    }
                }
            }
            if (fragmentShaderSetIndex != -1) {
                for (auto& binding :
                     fragmentShaderModule.reflectionData.descriptorSets[fragmentShaderSetIndex]
                         .bindings) {
                    if (binding.binding == j) {
                        if (!haveIt) {
                            bindings.push_back(binding);
                            haveIt = true;
                        } else {
                            bindings[j].stageFlags |= binding.stageFlags;
                        }
                    }
                }
            }
        }

        setLayoutInfo.pBindings    = bindings.data();
        setLayoutInfo.bindingCount = (uint32_t)bindings.size();

        VkDescriptorSetLayout layout;
        VK_CHECK(vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &layout));
        descriptorSetLayouts.push_back(layout);
    }

    // Create Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = helper::pipelineLayoutCreateInfo();

    pipelineLayoutCreateInfo.pPushConstantRanges    = combinedPushConstants.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = (uint32_t)combinedPushConstants.size();
    pipelineLayoutCreateInfo.pSetLayouts            = descriptorSetLayouts.data();
    pipelineLayoutCreateInfo.setLayoutCount         = (uint32_t)descriptorSetLayouts.size();

    VkPipelineLayout pipelineLayout;
    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext                        = nullptr;
    pipelineInfo.stageCount                   = 2;
    VkPipelineShaderStageCreateInfo shaderStages[2] = { vertexShaderModule.shaderStageInfo,
                                                        fragmentShaderModule.shaderStageInfo };
    pipelineInfo.pStages                            = &shaderStages[0];

    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.pNext = nullptr;
    if (vertexShaderModule.reflectionData.hasVertexBindingDescription) {
        vertexInputState.vertexBindingDescriptionCount = 1;
    } else {
        vertexInputState.vertexBindingDescriptionCount = 0;
    }
    vertexInputState.pVertexBindingDescriptions =
        &vertexShaderModule.reflectionData.inputBindingDescription;
    vertexInputState.vertexAttributeDescriptionCount =
        (uint32_t)vertexShaderModule.reflectionData.inputDescriptions.size();
    vertexInputState.pVertexAttributeDescriptions =
        vertexShaderModule.reflectionData.inputDescriptions.data();
    pipelineInfo.pVertexInputState = &vertexInputState;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.pNext = nullptr;
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;
    inputAssemblyState.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInfo.pInputAssemblyState          = &inputAssemblyState;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext         = nullptr;
    viewportState.viewportCount = 1;
    VkViewport viewport;
    viewport.x                 = 0;
    viewport.y                 = 0;
    viewport.width             = (float)pipelineCreateInfo->viewportWidth;
    viewport.height            = (float)pipelineCreateInfo->viewportHeight;
    viewport.minDepth          = 0.0f;
    viewport.maxDepth          = 1.0f;
    viewportState.pViewports   = &viewport;
    viewportState.scissorCount = 1;
    VkRect2D scissor;
    scissor.offset              = { 0, 0 };
    scissor.extent              = { (unsigned int)pipelineCreateInfo->viewportWidth,
                                    (unsigned int)pipelineCreateInfo->viewportHeight };
    viewportState.pScissors     = &scissor;
    pipelineInfo.pViewportState = &viewportState;

    VkPipelineRasterizationStateCreateInfo rasterizationState = {};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.pNext = nullptr;
    rasterizationState.depthClampEnable        = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    rasterizationState.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode =
        (pipelineCreateInfo->culling) ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    rasterizationState.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthBiasEnable         = VK_FALSE;
    rasterizationState.depthBiasConstantFactor = 0.0f;
    rasterizationState.depthBiasClamp          = 0.0f;
    rasterizationState.depthBiasSlopeFactor    = 0.0f;
    rasterizationState.lineWidth               = 1.0f;
    pipelineInfo.pRasterizationState           = &rasterizationState;

    VkPipelineMultisampleStateCreateInfo multisampleState = {};
    multisampleState.sType               = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.pNext               = nullptr;
    multisampleState.sampleShadingEnable = VK_FALSE;
    multisampleState.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampleState.minSampleShading      = 1.0f;
    multisampleState.pSampleMask           = nullptr;
    multisampleState.alphaToCoverageEnable = VK_FALSE;
    multisampleState.alphaToOneEnable      = VK_FALSE;
    pipelineInfo.pMultisampleState         = &multisampleState;

    VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.pNext            = nullptr;
    depthStencilState.depthTestEnable  = (pipelineCreateInfo->depthTesting) ? VK_TRUE : VK_FALSE;
    depthStencilState.depthWriteEnable = (pipelineCreateInfo->depthTesting) ? VK_TRUE : VK_FALSE;
    depthStencilState.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.stencilTestEnable     = VK_FALSE;
    depthStencilState.minDepthBounds        = 0.0f;
    depthStencilState.maxDepthBounds        = 1.0f;
    pipelineInfo.pDepthStencilState         = &depthStencilState;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable                    = VK_FALSE;
    VkPipelineColorBlendStateCreateInfo colorBlendState = {};
    colorBlendState.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.pNext           = nullptr;
    colorBlendState.logicOpEnable   = VK_FALSE;
    colorBlendState.logicOp         = VK_LOGIC_OP_COPY;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments    = &colorBlendAttachment;
    pipelineInfo.pColorBlendState   = &colorBlendState;

    pipelineInfo.layout = pipelineLayout;
    if (pipelineCreateInfo->renderPass) {
        pipelineInfo.renderPass = pipelineCreateInfo->renderPass->renderPass;
    } else {
        pipelineInfo.renderPass = swapchainRenderPass;
    }

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) !=
        VK_SUCCESS) {
        Logger::renderer_logger->error("Failed to create pipeline");
        return VK_NULL_HANDLE;
    }

    return std::make_shared<Pipeline>(device, pipeline, pipelineLayout, descriptorSetLayouts);
}

std::shared_ptr<DescriptorSet>
GraphicsContext::createDescriptorSet(std::shared_ptr<Pipeline> pipeline, uint32_t setLayoutIndex) {

    VkDescriptorSetAllocateInfo allocateInfo = {};
    allocateInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.pNext                       = nullptr;
    allocateInfo.descriptorPool              = globalDescriptorPool;
    allocateInfo.descriptorSetCount          = 1;
    if (pipeline->descriptorSetLayouts.size() > setLayoutIndex) {
        allocateInfo.pSetLayouts = &pipeline->descriptorSetLayouts[setLayoutIndex];
    } else {
        Logger::renderer_logger->error("Invalid descriptor set index specified");
    }

    std::array<VkDescriptorSet, FRAME_OVERLAP> descriptorSets;
    for (unsigned int i = 0; i < FRAME_OVERLAP; i++) {
        vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSets[i]);
    }

    return std::make_shared<DescriptorSet>(allocator, descriptorSets, pipeline->layout);
}

void GraphicsContext::descriptorSetAddBuffer(std::shared_ptr<DescriptorSet> descriptorSet,
                                             uint32_t binding, DescriptorType type,
                                             uint32_t bufferSize) {

    for (unsigned int i = 0; i < FRAME_OVERLAP; i++) {
        VkBufferCreateInfo bufferCreateInfo = {};
        bufferCreateInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCreateInfo.pNext              = nullptr;
        bufferCreateInfo.size               = bufferSize;
        if (type == DescriptorType::UNIFORM_BUFFER) {
            bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        } else if (type == DescriptorType::STORAGE_BUFFER) {
            bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }

        VmaAllocationCreateInfo vmaallocInfo = {};
        vmaallocInfo.usage                   = VMA_MEMORY_USAGE_CPU_TO_GPU;

        VmaAllocation allocation;
        VkBuffer buffer;

        // allocate the buffer
        VK_CHECK(vmaCreateBuffer(allocator, &bufferCreateInfo, &vmaallocInfo, &buffer, &allocation,
                                 nullptr));

        descriptorSet->buffers[i][binding]     = buffer;
        descriptorSet->allocations[i][binding] = allocation;

        assert(descriptorSet->allocations[i][binding]);

        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer                 = descriptorSet->buffers[i][binding];
        bufferInfo.offset                 = 0;
        bufferInfo.range                  = bufferSize;

        VkWriteDescriptorSet setWrite = {};
        setWrite.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        setWrite.pNext                = nullptr;
        setWrite.dstBinding           = binding;
        setWrite.dstSet               = descriptorSet->descriptorSets[i];
        setWrite.descriptorCount      = 1;
        setWrite.descriptorType       = helper::getVkDescriptorType(type);
        setWrite.pBufferInfo          = &bufferInfo;

        vkUpdateDescriptorSets(device, 1, &setWrite, 0, nullptr);
    }
}

void GraphicsContext::descriptorSetAddImage(std::shared_ptr<DescriptorSet> descriptorSet,
                                            uint32_t binding, std::shared_ptr<Texture> image) {
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.sampler               = mainSampler;
    imageInfo.imageView             = image->imageView;
    imageInfo.imageLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VkWriteDescriptorSet write = {};
        write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext                = nullptr;

        write.dstBinding      = binding;
        write.dstSet          = descriptorSet->descriptorSets[i];
        write.descriptorCount = 1;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo      = &imageInfo;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
}

void GraphicsContext::descriptorSetAddRenderPassAttachment(
    std::shared_ptr<DescriptorSet> descriptorSet, uint32_t binding,
    std::shared_ptr<RenderPass> renderPass,
    uint32_t attachmentIndex) { // TODO: Check that the index is within correct range
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.sampler               = mainSampler;
    imageInfo.imageView             = renderPass->imageViews[attachmentIndex];
    imageInfo.imageLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VkWriteDescriptorSet write = {};
        write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext                = nullptr;

        write.dstBinding      = binding;
        write.dstSet          = descriptorSet->descriptorSets[i];
        write.descriptorCount = 1;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo      = &imageInfo;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }
}

std::shared_ptr<VertexBuffer> GraphicsContext::createVertexBuffer(void* data, uint32_t size) {
    // TODO: Queue families need to be specified to use multiple I believe (uploading on transfer
    // queue, use later on graphics/present queue) SEE:
    // https://vulkan-tutorial.com/Vertex_buffers/Staging_buffer
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.pNext              = nullptr;
    bufferCreateInfo.size               = size;
    bufferCreateInfo.usage              = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage                   = VMA_MEMORY_USAGE_CPU_TO_GPU;

    VkBuffer buffer;
    VmaAllocation allocation;
    VK_CHECK(vmaCreateBuffer(allocator, &bufferCreateInfo, &allocCreateInfo, &buffer, &allocation,
                             nullptr));

    void* dataDest;
    vmaMapMemory(allocator, allocation, &dataDest);
    memcpy(dataDest, data, size);
    vmaUnmapMemory(allocator, allocation);

    return std::make_shared<VertexBuffer>(allocator, buffer, allocation);
}

std::shared_ptr<Texture> GraphicsContext::createTexture(int width, int height, int numComponents,
                                                        ColorSpace colorSpace, unsigned char* data,
                                                        bool genMipmaps) {
    VkDeviceSize imageSize = width * height * numComponents;
    VkFormat imageFormat;
    if (numComponents == 3) {
        if (colorSpace == ColorSpace::SRGB) {
            imageFormat = VK_FORMAT_R8G8B8_SRGB;
        } else {
            imageFormat = VK_FORMAT_R8G8B8_UNORM;
        }
    } else if (numComponents == 4) {
        if (colorSpace == ColorSpace::SRGB) {
            imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        } else {
            imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
        }
    } else {
        Logger::renderer_logger->error("Invalid number of components for texture");
        assert(false);
    }

    VkBufferCreateInfo cpuTransferBufferInfo = {};
    cpuTransferBufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    cpuTransferBufferInfo.pNext              = nullptr;
    cpuTransferBufferInfo.size               = imageSize;
    cpuTransferBufferInfo.usage              = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo vmaAllocCreateInfo = {};
    vmaAllocCreateInfo.usage                   = VMA_MEMORY_USAGE_CPU_ONLY;

    VmaAllocation cpuTransferAllocation;
    VkBuffer cpuTransferBuffer;
    VK_CHECK(vmaCreateBuffer(allocator, &cpuTransferBufferInfo, &vmaAllocCreateInfo,
                             &cpuTransferBuffer, &cpuTransferAllocation, nullptr));

    void* cpuTransferDataDest;
    vmaMapMemory(allocator, cpuTransferAllocation, &cpuTransferDataDest);
    memcpy(cpuTransferDataDest, data, imageSize);
    vmaUnmapMemory(allocator, cpuTransferAllocation);

    VkExtent3D imageExtent;
    imageExtent.width  = width;
    imageExtent.height = height;
    imageExtent.depth  = 1;

    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext             = nullptr;
    imageCreateInfo.flags             = 0;
    imageCreateInfo.imageType         = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format            = imageFormat;
    imageCreateInfo.extent            = imageExtent;
    imageCreateInfo.mipLevels         = (genMipmaps) ? mipLevels : 1;
    imageCreateInfo.arrayLayers       = 1;
    imageCreateInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageCreateInfo.initialLayout               = VK_IMAGE_LAYOUT_UNDEFINED;
    VmaAllocationCreateInfo imageAllocationInfo = {};
    imageAllocationInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImage transferImage;
    VmaAllocation transferAllocation;
    vmaCreateImage(allocator, &imageCreateInfo, &imageAllocationInfo, &transferImage,
                   &transferAllocation, nullptr);

    immediateSubmit([&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel   = 0;
        range.levelCount     = (genMipmaps) ? mipLevels : 1;
        range.baseArrayLayer = 0;
        range.layerCount     = 1;

        VkImageMemoryBarrier imageBarrierForTransfer = {};
        imageBarrierForTransfer.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrierForTransfer.pNext                = nullptr;
        imageBarrierForTransfer.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrierForTransfer.newLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrierForTransfer.image                = transferImage;
        imageBarrierForTransfer.subresourceRange     = range;
        imageBarrierForTransfer.srcAccessMask        = 0;
        imageBarrierForTransfer.dstAccessMask        = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &imageBarrierForTransfer);

        VkBufferImageCopy copyRegion               = {};
        copyRegion.bufferOffset                    = 0;
        copyRegion.bufferRowLength                 = 0;
        copyRegion.bufferImageHeight               = 0;
        copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel       = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount     = 1;
        copyRegion.imageExtent                     = imageExtent;

        vkCmdCopyBufferToImage(cmd, cpuTransferBuffer, transferImage,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        // Do mips here
        if (genMipmaps) {
            VkImageSubresourceRange singleMipRange = {};
            singleMipRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
            singleMipRange.baseArrayLayer          = 0;
            singleMipRange.layerCount              = 1;
            singleMipRange.levelCount              = 1;

            int32_t mipWidth  = width;
            int32_t mipHeight = height;

            for (unsigned int i = 1; i < mipLevels; i++) {
                singleMipRange.baseMipLevel = i - 1;

                VkImageMemoryBarrier
                    singleMipBarrier = {}; // TODO: Pull out of loop and just set the image resource
                                           // range already inside the struct instead of making new
                                           // one
                singleMipBarrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                singleMipBarrier.pNext            = nullptr;
                singleMipBarrier.image            = transferImage;
                singleMipBarrier.subresourceRange = singleMipRange;
                singleMipBarrier.srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;
                singleMipBarrier.dstAccessMask    = VK_ACCESS_TRANSFER_READ_BIT;
                singleMipBarrier.oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                singleMipBarrier.newLayout        = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                                     &singleMipBarrier);

                VkImageBlit mipBlit                   = {};
                mipBlit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                mipBlit.srcSubresource.mipLevel       = i - 1;
                mipBlit.srcSubresource.baseArrayLayer = 0;
                mipBlit.srcSubresource.layerCount     = 1;
                mipBlit.srcOffsets[0]                 = { 0, 0, 0 };
                mipBlit.srcOffsets[1]                 = { mipWidth, mipHeight, 1 };
                mipBlit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                mipBlit.dstSubresource.mipLevel       = i;
                mipBlit.dstSubresource.baseArrayLayer = 0;
                mipBlit.dstSubresource.layerCount     = 1;
                mipBlit.dstOffsets[0]                 = { 0, 0, 0 };
                mipBlit.dstOffsets[1]                 = { (mipWidth > 1) ? mipWidth / 2 : 1,
                                          (mipHeight > 1) ? mipHeight / 2 : 1, 1 };

                vkCmdBlitImage(cmd, transferImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               transferImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &mipBlit,
                               VK_FILTER_LINEAR);

                singleMipBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                singleMipBarrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                singleMipBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                singleMipBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                                     nullptr, 1, &singleMipBarrier);

                if (mipWidth > 1)
                    mipWidth /= 2;
                if (mipHeight > 1)
                    mipHeight /= 2;
            }

            singleMipRange.baseMipLevel = mipLevels - 1;

            VkImageMemoryBarrier singleMipBarrier = {}; // TODO: Pull out of loop and just set the
                                                        // image resource range already inside the
                                                        // struct instead of making new one
            singleMipBarrier.sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            singleMipBarrier.pNext            = nullptr;
            singleMipBarrier.image            = transferImage;
            singleMipBarrier.subresourceRange = singleMipRange;
            singleMipBarrier.srcAccessMask    = VK_ACCESS_TRANSFER_WRITE_BIT;
            singleMipBarrier.dstAccessMask    = VK_ACCESS_SHADER_READ_BIT;
            singleMipBarrier.oldLayout        = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            singleMipBarrier.newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                                 1, &singleMipBarrier);
        } else {
            VkImageMemoryBarrier imageBarrierToFinal = imageBarrierForTransfer;
            imageBarrierToFinal.oldLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageBarrierToFinal.newLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageBarrierToFinal.srcAccessMask        = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageBarrierToFinal.dstAccessMask        = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                                 1, &imageBarrierToFinal);
        }
    });

    vmaDestroyBuffer(allocator, cpuTransferBuffer, cpuTransferAllocation);

    VkImageView imageView;
    VkImageViewCreateInfo imageViewInfo           = {};
    imageViewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.pNext                           = nullptr;
    imageViewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.image                           = transferImage;
    imageViewInfo.format                          = imageFormat;
    imageViewInfo.subresourceRange.baseMipLevel   = 0;
    imageViewInfo.subresourceRange.levelCount     = (genMipmaps) ? mipLevels : 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount     = 1;
    imageViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    vkCreateImageView(device, &imageViewInfo, nullptr, &imageView);

    return std::make_shared<Texture>(device, allocator, transferAllocation, transferImage,
                                     imageView);
}

std::shared_ptr<Texture> GraphicsContext::createHDRTexture(int width, int height, int numComponents,
                                                           float* data, bool genMipmaps) {
    VkDeviceSize imageSize = width * height * numComponents * sizeof(float);
    VkFormat imageFormat;
    if (numComponents == 3) {
        imageFormat = VK_FORMAT_R32G32B32_SFLOAT;
    } else if (numComponents == 4) {
        imageFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    } else {
        Logger::renderer_logger->error("Invalid number of components for texture");
        assert(false);
    }

    VkBufferCreateInfo cpuTransferBufferInfo = {};
    cpuTransferBufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    cpuTransferBufferInfo.pNext              = nullptr;
    cpuTransferBufferInfo.size               = imageSize;
    cpuTransferBufferInfo.usage              = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo vmaAllocCreateInfo = {};
    vmaAllocCreateInfo.usage                   = VMA_MEMORY_USAGE_CPU_ONLY;

    VmaAllocation cpuTransferAllocation;
    VkBuffer cpuTransferBuffer;
    VK_CHECK(vmaCreateBuffer(allocator, &cpuTransferBufferInfo, &vmaAllocCreateInfo,
                             &cpuTransferBuffer, &cpuTransferAllocation, nullptr));

    void* cpuTransferDataDest;
    vmaMapMemory(allocator, cpuTransferAllocation, &cpuTransferDataDest);
    memcpy(cpuTransferDataDest, data, imageSize);
    vmaUnmapMemory(allocator, cpuTransferAllocation);

    VkExtent3D imageExtent = {};
    imageExtent.width      = width;
    imageExtent.height     = height;
    imageExtent.depth      = 1;

    VkImageCreateInfo imageCreateInfo =
        helper::imageCreateInfo(imageFormat,
                                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                imageExtent);
    imageCreateInfo.tiling                      = VK_IMAGE_TILING_OPTIMAL;
    VmaAllocationCreateInfo imageAllocationInfo = {};
    imageAllocationInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImage transferImage;
    VmaAllocation transferAllocation;
    vmaCreateImage(allocator, &imageCreateInfo, &imageAllocationInfo, &transferImage,
                   &transferAllocation, nullptr);

    VkImage realFinalImage;
    VmaAllocation realFinalAllocation;

    immediateSubmit([&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel   = 0;
        range.levelCount     = 1;
        range.baseArrayLayer = 0;
        range.layerCount     = 1;

        VkImageMemoryBarrier imageBarrierForTransfer = {};
        imageBarrierForTransfer.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarrierForTransfer.pNext                = nullptr;
        imageBarrierForTransfer.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrierForTransfer.newLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrierForTransfer.image                = transferImage;
        imageBarrierForTransfer.subresourceRange     = range;
        imageBarrierForTransfer.srcAccessMask        = 0;
        imageBarrierForTransfer.dstAccessMask        = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &imageBarrierForTransfer);

        VkBufferImageCopy copyRegion               = {};
        copyRegion.bufferOffset                    = 0;
        copyRegion.bufferRowLength                 = 0;
        copyRegion.bufferImageHeight               = 0;
        copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel       = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount     = 1;
        copyRegion.imageExtent                     = imageExtent;

        vkCmdCopyBufferToImage(cmd, cpuTransferBuffer, transferImage,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        VkImageMemoryBarrier imageBarrierToFinal = imageBarrierForTransfer;
        imageBarrierToFinal.oldLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        // imageBarrierToFinal.newLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageBarrierToFinal.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageBarrierToFinal.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        // imageBarrierToFinal.dstAccessMask        = VK_ACCESS_SHADER_READ_BIT;
        imageBarrierToFinal.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &imageBarrierToFinal);

        VkImageCreateInfo realFinalImageCreateInfo = {};
        realFinalImageCreateInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        realFinalImageCreateInfo.pNext             = nullptr;
        realFinalImageCreateInfo.imageType         = VK_IMAGE_TYPE_2D;
        realFinalImageCreateInfo.format            = (imageFormat == VK_FORMAT_R32G32B32A32_SFLOAT)
                       ? VK_FORMAT_R16G16B16A16_SFLOAT
                       : VK_FORMAT_R16G16B16_SFLOAT;
        realFinalImageCreateInfo.extent            = imageExtent;
        realFinalImageCreateInfo.mipLevels         = 1;
        realFinalImageCreateInfo.arrayLayers       = 1;
        realFinalImageCreateInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
        realFinalImageCreateInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
        realFinalImageCreateInfo.usage =
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        vmaCreateImage(allocator, &realFinalImageCreateInfo, &imageAllocationInfo, &realFinalImage,
                       &realFinalAllocation, nullptr);

        VkImageMemoryBarrier finalImageToTransferDst = {};
        finalImageToTransferDst.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        finalImageToTransferDst.pNext                = nullptr;
        finalImageToTransferDst.oldLayout            = VK_IMAGE_LAYOUT_UNDEFINED;
        finalImageToTransferDst.newLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        finalImageToTransferDst.image                = realFinalImage;
        finalImageToTransferDst.subresourceRange     = range;
        finalImageToTransferDst.srcAccessMask        = 0;
        finalImageToTransferDst.dstAccessMask        = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &finalImageToTransferDst);

        VkImageBlit imageBlit               = {};
        imageBlit.srcOffsets[1].x           = width;
        imageBlit.srcOffsets[1].y           = height;
        imageBlit.srcOffsets[1].z           = 1;
        imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.srcSubresource.layerCount = 1;
        imageBlit.srcSubresource.mipLevel   = 0;
        imageBlit.dstOffsets[1].x           = width;
        imageBlit.dstOffsets[1].y           = height;
        imageBlit.dstOffsets[1].z           = 1;
        imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBlit.dstSubresource.layerCount = 1;
        imageBlit.dstSubresource.mipLevel   = 0;

        vkCmdBlitImage(cmd, transferImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, realFinalImage,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

        VkImageMemoryBarrier finalImageToShaderReadDst = {};
        finalImageToShaderReadDst.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        finalImageToShaderReadDst.pNext                = nullptr;
        finalImageToShaderReadDst.oldLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        finalImageToShaderReadDst.newLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        finalImageToShaderReadDst.image                = realFinalImage;
        finalImageToShaderReadDst.subresourceRange     = range;
        finalImageToShaderReadDst.srcAccessMask        = 0;
        finalImageToShaderReadDst.dstAccessMask        = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &finalImageToShaderReadDst);
    });

    vmaDestroyBuffer(allocator, cpuTransferBuffer, cpuTransferAllocation);
    vmaDestroyImage(allocator, transferImage, transferAllocation);

    VkImageView imageView;
    VkImageViewCreateInfo imageinfo = helper::imageViewCreateInfo(
        (imageFormat == VK_FORMAT_R32G32B32A32_SFLOAT) ? VK_FORMAT_R16G16B16A16_SFLOAT
                                                       : VK_FORMAT_R16G16B16_SFLOAT,
        realFinalImage, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(device, &imageinfo, nullptr, &imageView);

    return std::make_shared<Texture>(device, allocator, realFinalAllocation, realFinalImage,
                                     imageView);
}

std::shared_ptr<Texture> GraphicsContext::createCubemap(Format format, uint32_t width,
                                                        uint32_t height, bool reserveMipMaps) {
    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    VkImageCreateInfo cubemapCreateInfo = {};
    cubemapCreateInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    cubemapCreateInfo.pNext             = nullptr;
    cubemapCreateInfo.flags             = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    cubemapCreateInfo.imageType         = VK_IMAGE_TYPE_2D;
    cubemapCreateInfo.format            = helper::getVkFormat(format);
    VkExtent3D extent                   = {};
    extent.width                        = width;
    extent.height                       = height;
    extent.depth                        = 1;
    cubemapCreateInfo.extent            = { width, height, 1 };
    cubemapCreateInfo.mipLevels         = (reserveMipMaps) ? mipLevels : 1;
    cubemapCreateInfo.arrayLayers       = 6;
    cubemapCreateInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
    cubemapCreateInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
    cubemapCreateInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    cubemapCreateInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    cubemapCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo cubemapAllocationInfo = {};
    cubemapAllocationInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImage cubemapImage;
    VmaAllocation cubemapAllocation;
    vmaCreateImage(allocator, &cubemapCreateInfo, &cubemapAllocationInfo, &cubemapImage,
                   &cubemapAllocation, nullptr);

    immediateSubmit([&](VkCommandBuffer cmd) {
        VkImageSubresourceRange cubemapSubresourceRange = {};
        cubemapSubresourceRange.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        cubemapSubresourceRange.baseMipLevel            = 0;
        cubemapSubresourceRange.levelCount              = (reserveMipMaps) ? mipLevels : 1;
        cubemapSubresourceRange.baseArrayLayer          = 0;
        cubemapSubresourceRange.layerCount              = 6;

        VkImageMemoryBarrier cubemapBarrierInfo = {};
        cubemapBarrierInfo.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        cubemapBarrierInfo.pNext                = nullptr;
        cubemapBarrierInfo.srcAccessMask        = 0;
        cubemapBarrierInfo.dstAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        cubemapBarrierInfo.oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
        cubemapBarrierInfo.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        cubemapBarrierInfo.image         = cubemapImage;
        cubemapBarrierInfo.subresourceRange = cubemapSubresourceRange;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &cubemapBarrierInfo);
    });

    VkImageViewCreateInfo cubemapImageViewCreateInfo = {};
    cubemapImageViewCreateInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    cubemapImageViewCreateInfo.pNext                 = nullptr;
    cubemapImageViewCreateInfo.flags                 = 0;
    cubemapImageViewCreateInfo.image                 = cubemapImage;
    cubemapImageViewCreateInfo.viewType              = VK_IMAGE_VIEW_TYPE_CUBE;
    cubemapImageViewCreateInfo.format                = helper::getVkFormat(format);
    cubemapImageViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    cubemapImageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
    cubemapImageViewCreateInfo.subresourceRange.levelCount     = (reserveMipMaps) ? mipLevels : 1;
    cubemapImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    cubemapImageViewCreateInfo.subresourceRange.layerCount     = 6;

    VkImageView cubemapImageView;
    vkCreateImageView(device, &cubemapImageViewCreateInfo, nullptr, &cubemapImageView);

    return std::make_shared<Texture>(device, allocator, cubemapAllocation, cubemapImage,
                                     cubemapImageView);
}

std::unique_ptr<GraphicsContext> GraphicsContext::create(std::shared_ptr<Window> windowRef) {
    Logger::renderer_logger->info("Creating Graphics Context");
#ifdef _DEBUG
    Logger::renderer_logger->info(" - validation layers: true");
#else
    Logger::renderer_logger->info(" - validation layers: false");
#endif

    vkb::InstanceBuilder builder;
    auto instanceReturned = builder
                                .set_app_name("VkPBR")
#ifdef _DEBUG
                                .request_validation_layers(true)
#endif
                                .require_api_version(1, 1, 0)
                                .set_debug_callback(Logger::debugUtilsMessengerCallback)
                                .build();

    if (!instanceReturned || !instanceReturned.has_value()) {
        Logger::renderer_logger->info("Failed to create instance");
    }

    vkb::Instance vkbInstance = instanceReturned.value();

    VkInstance instance                     = vkbInstance.instance;
    VkDebugUtilsMessengerEXT debugMessenger = vkbInstance.debug_messenger;

    VkSurfaceKHR surface;
    VK_CHECK(glfwCreateWindowSurface(instance, windowRef->get(), nullptr, &surface));

    vkb::PhysicalDeviceSelector selector{ vkbInstance };
    vkb::PhysicalDevice vkbPhysicalDevice =
        selector.set_minimum_version(1, 1)
            .set_surface(surface)
            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
            .select()
            .value();

    Logger::renderer_logger->info(" - using Physical Device: {0}",
                                  vkbPhysicalDevice.properties.deviceName);

    vkb::DeviceBuilder deviceBuilder{ vkbPhysicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    VkDevice device                 = vkbDevice.device;
    VkPhysicalDevice physicalDevice = vkbPhysicalDevice.physical_device;

    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    Logger::renderer_logger->info("  - Physical device has minimum buffer aligment of {0}",
                                  physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);

    VkQueue graphicsQueue        = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    uint32_t graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    Logger::renderer_logger->info("  - Graphics Queue Family: {0}", graphicsQueueFamily);

    VkQueue transferQueue;
    uint32_t transferQueueFamily;

    vkb::Result<VkQueue> transferQueueResult = vkbDevice.get_queue(vkb::QueueType::transfer);
    vkb::Result<uint32_t> transferQueueFamilyResult =
        vkbDevice.get_dedicated_queue_index(vkb::QueueType::transfer);
    if (!transferQueueResult.has_value() || !transferQueueFamilyResult.has_value()) {
        Logger::renderer_logger->warn(
            "No dedicated transfer queue available, using the graphics queue");

        transferQueue       = graphicsQueue;
        transferQueueFamily = graphicsQueueFamily;
    } else {
        transferQueue       = transferQueueResult.value();
        transferQueueFamily = transferQueueFamilyResult.value();
    }

    return std::make_unique<GraphicsContext>(
        windowRef, instance, device, physicalDevice, debugMessenger, physicalDeviceProperties,
        graphicsQueue, graphicsQueueFamily, transferQueue, transferQueueFamily, surface);
}

uint32_t GraphicsContext::getCurrentFrameBasedIndex(int frameOffset) {
    return (numFrames + frameOffset) % FRAME_OVERLAP;
}

void GraphicsContext::windowResize(int width, int height) {
    vkDestroyImageView(device, depthImageView, nullptr);
    vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);

    for (unsigned int i = 0; i < swapchainImageViews.size(); i++) {
        vkDestroyImageView(device, swapchainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);

    initSwapchain();
}

void GraphicsContext::initDescriptorPool() {
    // TODO: Add better numbers here
    std::vector<VkDescriptorPoolSize> sizes = { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
                                                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
                                                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                  1000 } };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags                      = 0;
    pool_info.maxSets                    = 100; // TODO: Add better numbers here
    pool_info.poolSizeCount              = (uint32_t)sizes.size();
    pool_info.pPoolSizes                 = sizes.data();

    vkCreateDescriptorPool(device, &pool_info, nullptr, &globalDescriptorPool);
}

void GraphicsContext::initAllocators() {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice         = physicalDevice;
    allocatorInfo.device                 = device;
    allocatorInfo.instance               = instance;

    VK_CHECK(vmaCreateAllocator(&allocatorInfo, &allocator));
}

void GraphicsContext::initSwapchain() {
    vkb::SwapchainBuilder swapchainBuilder{ physicalDevice, device, surface };

    Logger::renderer_logger->info("Graphics context creating swapchain with size: {0} {1}",
                                  windowRef->getWidth(), windowRef->getHeight());
    currentSwapchainExtent.width  = windowRef->getWidth();
    currentSwapchainExtent.height = windowRef->getHeight();

    auto swapchainBuildResult =
        swapchainBuilder
            .use_default_format_selection()
            //.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(currentSwapchainExtent.width, currentSwapchainExtent.height)
            .build();

    if (!swapchainBuildResult || !swapchainBuildResult.has_value()) {
        Logger::renderer_logger->error("Failed to create swapchain");
    }

    vkb::Swapchain vkbSwapchain = swapchainBuildResult.value();

    swapchain            = vkbSwapchain.swapchain;
    swapchainImages      = vkbSwapchain.get_images().value();
    swapchainImageViews  = vkbSwapchain.get_image_views().value();
    swapchainImageFormat = vkbSwapchain.image_format;

    VkExtent3D depthImageExtent = { (uint32_t)windowRef->getWidth(),
                                    (uint32_t)windowRef->getHeight(), 1 };

    depthFormat = VK_FORMAT_D32_SFLOAT;

    VkImageCreateInfo depthImageInfo = helper::imageCreateInfo(
        depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

    VmaAllocationCreateInfo depthImageAllocInfo = {};
    depthImageAllocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    depthImageAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vmaCreateImage(allocator, &depthImageInfo, &depthImageAllocInfo, &depthImage.image,
                   &depthImage.allocation, nullptr);

    VkImageViewCreateInfo depthViewInfo =
        helper::imageViewCreateInfo(depthFormat, depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

    VK_CHECK(vkCreateImageView(device, &depthViewInfo, nullptr, &depthImageView));
}

void GraphicsContext::initSwapchainRenderPass() {
    std::vector<RenderPassAttachmentDescription> renderPassAttachmentDescriptions;
    RenderPassAttachmentDescription colorAttachmentDescription = {};
    colorAttachmentDescription.loadOp                          = LoadOp::CLEAR;
    colorAttachmentDescription.storeOp                         = StoreOp::STORE;
    colorAttachmentDescription.initialLayout                   = ImageLayout::UNDEFINED;
    colorAttachmentDescription.finalLayout                     = ImageLayout::PRESENT;
    renderPassAttachmentDescriptions.push_back(colorAttachmentDescription);

    RenderPassAttachmentDescription depthAttachmentDescription = {};
    depthAttachmentDescription.loadOp                          = LoadOp::CLEAR;
    depthAttachmentDescription.storeOp                         = StoreOp::STORE;
    depthAttachmentDescription.initialLayout                   = ImageLayout::UNDEFINED;
    depthAttachmentDescription.finalLayout                     = ImageLayout::ATTACHMENT;

    std::vector<VkAttachmentDescription> attachmentDescriptions;
    std::vector<VkAttachmentReference> attachmentReferences;
    for (int i = 0; i < renderPassAttachmentDescriptions.size(); i++) {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format                  = swapchainImageFormat;
        colorAttachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp =
            helper::getVkAttachmentLoadOp(renderPassAttachmentDescriptions[i].loadOp);
        colorAttachment.storeOp =
            helper::getVkAttachmentStoreOp(renderPassAttachmentDescriptions[i].storeOp);
        colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout =
            helper::getVkImageLayout(renderPassAttachmentDescriptions[i].initialLayout);
        colorAttachment.finalLayout =
            helper::getVkImageLayout(renderPassAttachmentDescriptions[i].finalLayout);

        VkAttachmentReference colorAttachmentReference = {};
        colorAttachmentReference.attachment            = i;
        colorAttachmentReference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachmentDescriptions.push_back(colorAttachment);
        attachmentReferences.push_back(colorAttachmentReference);
    }

    VkAttachmentDescription depthDescription = {};
    depthDescription.flags                   = 0;
    depthDescription.format                  = depthFormat;
    depthDescription.samples                 = VK_SAMPLE_COUNT_1_BIT;
    depthDescription.loadOp  = helper::getVkAttachmentLoadOp(depthAttachmentDescription.loadOp);
    depthDescription.storeOp = helper::getVkAttachmentStoreOp(depthAttachmentDescription.storeOp);
    depthDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthDescription.initialLayout =
        helper::getVkImageLayout(depthAttachmentDescription.initialLayout, true);
    depthDescription.finalLayout =
        helper::getVkImageLayout(depthAttachmentDescription.finalLayout, true);

    VkAttachmentReference depthReference = {};
    depthReference.attachment            = (uint32_t)renderPassAttachmentDescriptions.size();
    depthReference.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass    = {};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = (uint32_t)attachmentReferences.size();
    subpass.pColorAttachments       = attachmentReferences.data();
    subpass.pDepthStencilAttachment = &depthReference;

    std::vector<VkAttachmentDescription> allAttachmentDescriptions = attachmentDescriptions;
    allAttachmentDescriptions.push_back(depthDescription);

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount        = (uint32_t)allAttachmentDescriptions.size();
    renderPassCreateInfo.pAttachments           = allAttachmentDescriptions.data();
    renderPassCreateInfo.subpassCount           = 1;
    renderPassCreateInfo.pSubpasses             = &subpass;

    VK_CHECK(vkCreateRenderPass(device, &renderPassCreateInfo, nullptr, &swapchainRenderPass));

    swapchainFramebuffers = std::vector<VkFramebuffer>(swapchainImages.size());

    VkFramebufferCreateInfo fbCreateInfo = {};
    fbCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbCreateInfo.pNext                   = nullptr;
    fbCreateInfo.renderPass              = swapchainRenderPass;
    fbCreateInfo.attachmentCount         = (uint32_t)allAttachmentDescriptions.size();
    fbCreateInfo.width                   = windowRef->getWidth();
    fbCreateInfo.height                  = windowRef->getHeight();
    fbCreateInfo.layers                  = 1;

    for (int i = 0; i < swapchainFramebuffers.size();
         i++) { // TODO: This doesn't work for anything but my test of the swapchain renderpass
        VkImageView attachments[2];
        attachments[0] = swapchainImageViews[i];
        attachments[1] = depthImageView;

        fbCreateInfo.pAttachments = &attachments[0];
        VK_CHECK(vkCreateFramebuffer(device, &fbCreateInfo, nullptr, &swapchainFramebuffers[i]));
    }
}

void GraphicsContext::initUploadStructures() {
    VkCommandPoolCreateInfo uploadCommandPoolInfo =
        helper::commandPoolCreateInfo(graphicsQueueFamily);
    VK_CHECK(vkCreateCommandPool(device, &uploadCommandPoolInfo, nullptr, &uploadCommandPool));

    VkFenceCreateInfo uploadFenceCreateInfo = helper::fenceCreateInfo();
    VK_CHECK(vkCreateFence(device, &uploadFenceCreateInfo, nullptr, &uploadFence));
}

void GraphicsContext::initSamplers() {
    VkSamplerCreateInfo info = {};
    info.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.pNext               = nullptr;

    info.magFilter    = VK_FILTER_LINEAR;
    info.minFilter    = VK_FILTER_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.minLod       = 0.0f;
    info.mipLodBias   = 0.0f;
    info.maxLod       = VK_LOD_CLAMP_NONE;

    vkCreateSampler(device, &info, nullptr, &mainSampler);
}

std::string readFileToString(const char* filePath) {
    std::ifstream inFile;
    inFile.open(filePath);
    if (!inFile.is_open()) {
        Logger::renderer_logger->error("Shader file missing: {0}", filePath);
    }

    return std::string((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
}

shaderc_shader_kind shaderKindFromExtension(std::string& fileExtension) {
    if (fileExtension == ".vert") {
        return shaderc_shader_kind::shaderc_vertex_shader;
    } else if (fileExtension == ".frag") {
        return shaderc_shader_kind::shaderc_fragment_shader;
    } else {
        Logger::renderer_logger->error("Invalid shader extension: {0}", fileExtension);
        return shaderc_shader_kind::shaderc_vertex_shader;
    }
}

std::string preprocessGLSL(std::string& inputGLSL, shaderc_shader_kind shaderKind,
                           const char* shaderFilePath) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    shaderc::PreprocessedSourceCompilationResult result =
        compiler.PreprocessGlsl(inputGLSL, shaderKind, "shader", options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        Logger::renderer_logger->error("Failed to preprocess glsl shader: {0}. {1}", shaderFilePath,
                                       result.GetErrorMessage());
    }

    return std::string(result.cbegin(), result.cend());
}

std::vector<uint32_t> spvBytesFromGLSL(std::string& glsl, shaderc_shader_kind shaderKind,
                                       const char* shaderFilePath) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    if (false) { // Bool optimize
        options.SetOptimizationLevel(shaderc_optimization_level_performance);
    }

    shaderc::SpvCompilationResult spv_module =
        compiler.CompileGlslToSpv(glsl, shaderKind, "name", options);

    if (spv_module.GetCompilationStatus() != shaderc_compilation_status_success) {
        Logger::renderer_logger->error("Failed to compile shader: {0}, {1}", shaderFilePath,
                                       spv_module.GetErrorMessage());
    }

    return std::vector<uint32_t>(spv_module.cbegin(), spv_module.cend());
}

VkShaderStageFlags vkShaderStageFromShaderCStage(shaderc_shader_kind shaderKind) {
    if (shaderKind == shaderc_shader_kind::shaderc_vertex_shader) {
        return VK_SHADER_STAGE_VERTEX_BIT;
    } else if (shaderKind == shaderc_shader_kind::shaderc_fragment_shader) {
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    return VK_SHADER_STAGE_VERTEX_BIT;
}

void parseInputDescriptions(SpvReflectShaderModule& shaderModule,
                            ShaderModuleReflectionData& reflectionData) {

    uint32_t inputDescriptionCount = 0;
    SpvReflectResult result =
        spvReflectEnumerateInputVariables(&shaderModule, &inputDescriptionCount, nullptr);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectInterfaceVariable*> inputVariables(inputDescriptionCount);
    result = spvReflectEnumerateInputVariables(&shaderModule, &inputDescriptionCount,
                                               inputVariables.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    for (int i = 0; i < inputVariables.size(); i++) {
        if (inputVariables[i]->built_in == -1) { // TODO: I don't know if this is correct to do, but
                                                 // all tutorials ignore specifying built-ins.
            VkVertexInputAttributeDescription inputDescription;
            inputDescription.binding  = 0;
            inputDescription.location = inputVariables[i]->location;
            inputDescription.format   = static_cast<VkFormat>(inputVariables[i]->format);

            reflectionData.inputDescriptions.push_back(inputDescription);
        }
    }

    std::sort(reflectionData.inputDescriptions.begin(), reflectionData.inputDescriptions.end(),
              [](VkVertexInputAttributeDescription a, VkVertexInputAttributeDescription b) {
                  return a.location < b.location;
              });

    uint32_t runningOffset = 0;
    for (auto& desc : reflectionData.inputDescriptions) {
        desc.offset = runningOffset;
        runningOffset += helper::vkFormatAsBytes(desc.format);
    }

    if (runningOffset > 0) {
        reflectionData.hasVertexBindingDescription = true;

        VkVertexInputBindingDescription binding = {};
        binding.binding                         = 0;
        binding.stride                          = runningOffset;
        binding.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

        reflectionData.inputBindingDescription = binding;
    } else {
        reflectionData.hasVertexBindingDescription = false;
    }
}

void parsePushConstants(SpvReflectShaderModule& shaderModule,
                        ShaderModuleReflectionData& reflectionData,
                        VkShaderStageFlags shaderStageFlags) {
    uint32_t pushConstantCount = 0;
    SpvReflectResult result =
        spvReflectEnumeratePushConstantBlocks(&shaderModule, &pushConstantCount, nullptr);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectBlockVariable*> pushConstants(pushConstantCount);
    result = spvReflectEnumeratePushConstantBlocks(&shaderModule, &pushConstantCount,
                                                   pushConstants.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    for (SpvReflectBlockVariable* variable : pushConstants) {
        VkPushConstantRange pushConstant;
        pushConstant.offset     = variable->offset;
        pushConstant.size       = variable->size;
        pushConstant.stageFlags = shaderStageFlags;

        reflectionData.pushConstants.push_back(pushConstant);
    }
}

void parseDescriptorSets(SpvReflectShaderModule& shaderModule,
                         ShaderModuleReflectionData& reflectionData) {
    uint32_t descriptorSetsCount = 0;
    SpvReflectResult result =
        spvReflectEnumerateDescriptorSets(&shaderModule, &descriptorSetsCount, nullptr);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    std::vector<SpvReflectDescriptorSet*> descriptorSets(descriptorSetsCount);
    result = spvReflectEnumerateDescriptorSets(&shaderModule, &descriptorSetsCount,
                                               descriptorSets.data());

    for (SpvReflectDescriptorSet* descriptorSet : descriptorSets) {
        DescriptorSetLayoutData data;

        data.set_number = descriptorSet->set;

        for (unsigned int i = 0; i < descriptorSet->binding_count; i++) {
            SpvReflectDescriptorBinding* binding = descriptorSet->bindings[i];

            VkDescriptorSetLayoutBinding setLayoutBinding;
            setLayoutBinding.binding         = binding->binding;
            setLayoutBinding.descriptorCount = binding->count;
            setLayoutBinding.descriptorType =
                static_cast<VkDescriptorType>(binding->descriptor_type);
            setLayoutBinding.stageFlags =
                static_cast<VkShaderStageFlagBits>(shaderModule.shader_stage);
            setLayoutBinding.pImmutableSamplers = nullptr;

            data.bindings.push_back(setLayoutBinding);
        }
        reflectionData.descriptorSets.push_back(data);
    }
}

ShaderModuleReflectionData parseReflectionDataFromSpvBytes(std::vector<uint32_t>& spvBytes,
                                                           VkShaderStageFlags shaderStageFlags) {
    SpvReflectShaderModule shaderModule;
    SpvReflectResult result = spvReflectCreateShaderModule(spvBytes.size() * sizeof(uint32_t),
                                                           spvBytes.data(), &shaderModule);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    ShaderModuleReflectionData reflectionData;

    parseInputDescriptions(shaderModule, reflectionData);

    parsePushConstants(shaderModule, reflectionData, shaderStageFlags);

    parseDescriptorSets(shaderModule, reflectionData);

    return reflectionData;
}

ShaderModule GraphicsContext::loadShaderModule(const char* shaderFilePath) {
    std::string glslString         = readFileToString(shaderFilePath);
    std::string extension          = std::filesystem::path(shaderFilePath).extension().string();
    shaderc_shader_kind shaderKind = shaderKindFromExtension(extension);

    std::string preprocessedGLSL = preprocessGLSL(glslString, shaderKind, shaderFilePath);

    std::vector<uint32_t> spvBytes = spvBytesFromGLSL(preprocessedGLSL, shaderKind, shaderFilePath);

    ShaderModuleReflectionData reflectionData =
        parseReflectionDataFromSpvBytes(spvBytes, vkShaderStageFromShaderCStage(shaderKind));

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext                    = nullptr;

    createInfo.codeSize = spvBytes.size() * sizeof(uint32_t);
    createInfo.pCode    = spvBytes.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        Logger::renderer_logger->error("Failed to create shader module: {0}", shaderFilePath);
    }

    VkPipelineShaderStageCreateInfo shaderStageInfo = {};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.pNext = nullptr;
    if (shaderKind == shaderc_vertex_shader) {
        shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    } else if (shaderKind == shaderc_fragment_shader) {
        shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName  = "main";

    return ShaderModule(device, shaderModule, shaderStageInfo, reflectionData);
}

void GraphicsContext::immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function) {
    VkCommandBufferAllocateInfo cmdAllocInfo =
        helper::commandBufferAllocateInfo(uploadCommandPool, 1);

    VkCommandBuffer cmd;
    VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &cmd));

    VkCommandBufferBeginInfo cmdBeginInfo =
        helper::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submit = helper::submitInfo(&cmd);

    VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, uploadFence));

    vkWaitForFences(device, 1, &uploadFence, true, 9999999999);
    vkResetFences(device, 1, &uploadFence);

    vkResetCommandPool(device, uploadCommandPool, 0);
}
