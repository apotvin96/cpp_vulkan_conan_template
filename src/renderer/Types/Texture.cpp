#include "../../pch.hpp"
#include "Texture.hpp"

#include "../../Logger.hpp"

Texture::Texture(VkDevice device, VmaAllocator allocator, VmaAllocation allocation, VkImage image,
                 VkImageView imageView)
    : device(device), allocator(allocator), allocation(allocation), image(image),
      imageView(imageView) {}

Texture::~Texture() {
    Logger::renderer_logger->info("Destroying Texture");

    if (device != VK_NULL_HANDLE) {
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        if (allocator != VK_NULL_HANDLE && image != VK_NULL_HANDLE &&
            allocation != VK_NULL_HANDLE) {
            vmaDestroyImage(allocator, image, allocation);
        }
    }
}
