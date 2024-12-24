#pragma once

enum class AccessType { SRC, DST };

enum class ImageLayout { UNDEFINED, ATTACHMENT, SHADER_READ, PRESENT, TRANSFER_SRC, TRANSFER_DST };

enum class LoadOp { DONT_CARE, LOAD, CLEAR };

enum class StoreOp { DONT_CARE, STORE };

enum class ColorSpace { LINEAR, SRGB };

enum class Format {
    R16_FLOAT,
    R32_FLOAT,
    RG16_FLOAT,
    RG32_FLOAT,
    RGB16_FLOAT,
    RGB32_FLOAT,
    RGBA16_FLOAT,
    RGBA32_FLOAT
};

struct RenderPassAttachmentDescription {
    LoadOp loadOp;
    StoreOp storeOp;
    ImageLayout initialLayout;
    ImageLayout finalLayout;
    Format format;
    uint32_t width;
    uint32_t height;
};

struct RenderPass {
    VkDevice device;
    VmaAllocator allocator;

    VkRenderPass renderPass;
    VkFramebuffer framebuffer;

    std::vector<VkImage> images;
    std::vector<VmaAllocation> allocations;
    std::vector<VkImageView> imageViews;

    RenderPass(VkDevice device, VmaAllocator allocator, VkRenderPass renderPass,
               VkFramebuffer framebuffer, std::vector<VkImage> images,
               std::vector<VmaAllocation> allocations, std::vector<VkImageView> imageViews);

    ~RenderPass();
};
