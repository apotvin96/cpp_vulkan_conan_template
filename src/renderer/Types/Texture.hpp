#pragma once

struct Texture {
    VkDevice device;
    VmaAllocator allocator;

    VmaAllocation allocation;
    VkImage image;
    VkImageView imageView;

    Texture(VkDevice device, VmaAllocator allocator, VmaAllocation allocation, VkImage image,
            VkImageView imageView);

    ~Texture();
};