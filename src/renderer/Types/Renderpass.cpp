#include "../../pch.hpp"
#include "Renderpass.hpp"

#include "../../Logger.hpp"

RenderPass::RenderPass(VkDevice device, VmaAllocator allocator, VkRenderPass renderPass,
                       VkFramebuffer framebuffer, std::vector<VkImage> images,
                       std::vector<VmaAllocation> allocations, std::vector<VkImageView> imageViews)
    : device(device), allocator(allocator), renderPass(renderPass), framebuffer(framebuffer),
      images(images), allocations(allocations), imageViews(imageViews) {}

RenderPass::~RenderPass() {
    Logger::renderer_logger->info("Destroying RenderPass");

    if (device != VK_NULL_HANDLE) {
        if (renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device, renderPass, nullptr);
        }

        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        for (auto& imageView : imageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        for (int i = 0; i < images.size(); i++) {
            vmaDestroyImage(allocator, images[i], allocations[i]);
        }
    }
}