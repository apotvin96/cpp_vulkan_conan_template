#include "../../pch.hpp"
#include "Conversions.hpp"

VkAttachmentLoadOp helper::getVkAttachmentLoadOp(LoadOp loadOp) {
    switch (loadOp) {
    case LoadOp::DONT_CARE:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        break;
    case LoadOp::LOAD:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
        break;
    case LoadOp::CLEAR:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
        break;
    }

    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}

VkAttachmentStoreOp helper::getVkAttachmentStoreOp(StoreOp storeOp) {
    switch (storeOp) {
    case StoreOp::DONT_CARE:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        break;
    case StoreOp::STORE:
        return VK_ATTACHMENT_STORE_OP_STORE;
        break;
    }

    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
}

VkImageLayout helper::getVkImageLayout(ImageLayout imageLayout, bool isDepthImage) {
    switch (imageLayout) {
    case ImageLayout::UNDEFINED:
        return VK_IMAGE_LAYOUT_UNDEFINED;
        break;
    case ImageLayout::ATTACHMENT:
        if (isDepthImage) {
            return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        } else {
            return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        break;
    case ImageLayout::SHADER_READ:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        break;
    case ImageLayout::PRESENT:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        break;
    case ImageLayout::TRANSFER_SRC:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        break;
    case ImageLayout::TRANSFER_DST:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        break;
    }

    return VK_IMAGE_LAYOUT_UNDEFINED;
}

VkAccessFlagBits helper::getVkAccessFlags(ImageLayout imageLayout, AccessType accessType,
                                          bool isDepthImage) {
    switch (imageLayout) {
    case ImageLayout::UNDEFINED:
        return VK_ACCESS_FLAG_BITS_MAX_ENUM;
        break;
    case ImageLayout::ATTACHMENT:
        if (isDepthImage) {
            if (accessType == AccessType::SRC) {
                return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            } else {
                return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            }
        } else {
            if (accessType == AccessType::SRC) {
                return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            } else {
                return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            }
        }
        break;
    case ImageLayout::SHADER_READ:
        return VK_ACCESS_SHADER_READ_BIT;
        break;
    case ImageLayout::TRANSFER_SRC:
        return VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case ImageLayout::TRANSFER_DST:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    }

    return VK_ACCESS_FLAG_BITS_MAX_ENUM;
}

uint32_t helper::vkFormatAsBytes(VkFormat format) {
    uint32_t result = 0;
    switch (format) {
    case VK_FORMAT_UNDEFINED:
        result = 0;
        break;
    case VK_FORMAT_R4G4_UNORM_PACK8:
        result = 1;
        break;
    case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
        result = 2;
        break;
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
        result = 2;
        break;
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
        result = 2;
        break;
    case VK_FORMAT_B5G6R5_UNORM_PACK16:
        result = 2;
        break;
    case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
        result = 2;
        break;
    case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
        result = 2;
        break;
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
        result = 2;
        break;
    case VK_FORMAT_R8_UNORM:
        result = 1;
        break;
    case VK_FORMAT_R8_SNORM:
        result = 1;
        break;
    case VK_FORMAT_R8_USCALED:
        result = 1;
        break;
    case VK_FORMAT_R8_SSCALED:
        result = 1;
        break;
    case VK_FORMAT_R8_UINT:
        result = 1;
        break;
    case VK_FORMAT_R8_SINT:
        result = 1;
        break;
    case VK_FORMAT_R8_SRGB:
        result = 1;
        break;
    case VK_FORMAT_R8G8_UNORM:
        result = 2;
        break;
    case VK_FORMAT_R8G8_SNORM:
        result = 2;
        break;
    case VK_FORMAT_R8G8_USCALED:
        result = 2;
        break;
    case VK_FORMAT_R8G8_SSCALED:
        result = 2;
        break;
    case VK_FORMAT_R8G8_UINT:
        result = 2;
        break;
    case VK_FORMAT_R8G8_SINT:
        result = 2;
        break;
    case VK_FORMAT_R8G8_SRGB:
        result = 2;
        break;
    case VK_FORMAT_R8G8B8_UNORM:
        result = 3;
        break;
    case VK_FORMAT_R8G8B8_SNORM:
        result = 3;
        break;
    case VK_FORMAT_R8G8B8_USCALED:
        result = 3;
        break;
    case VK_FORMAT_R8G8B8_SSCALED:
        result = 3;
        break;
    case VK_FORMAT_R8G8B8_UINT:
        result = 3;
        break;
    case VK_FORMAT_R8G8B8_SINT:
        result = 3;
        break;
    case VK_FORMAT_R8G8B8_SRGB:
        result = 3;
        break;
    case VK_FORMAT_B8G8R8_UNORM:
        result = 3;
        break;
    case VK_FORMAT_B8G8R8_SNORM:
        result = 3;
        break;
    case VK_FORMAT_B8G8R8_USCALED:
        result = 3;
        break;
    case VK_FORMAT_B8G8R8_SSCALED:
        result = 3;
        break;
    case VK_FORMAT_B8G8R8_UINT:
        result = 3;
        break;
    case VK_FORMAT_B8G8R8_SINT:
        result = 3;
        break;
    case VK_FORMAT_B8G8R8_SRGB:
        result = 3;
        break;
    case VK_FORMAT_R8G8B8A8_UNORM:
        result = 4;
        break;
    case VK_FORMAT_R8G8B8A8_SNORM:
        result = 4;
        break;
    case VK_FORMAT_R8G8B8A8_USCALED:
        result = 4;
        break;
    case VK_FORMAT_R8G8B8A8_SSCALED:
        result = 4;
        break;
    case VK_FORMAT_R8G8B8A8_UINT:
        result = 4;
        break;
    case VK_FORMAT_R8G8B8A8_SINT:
        result = 4;
        break;
    case VK_FORMAT_R8G8B8A8_SRGB:
        result = 4;
        break;
    case VK_FORMAT_B8G8R8A8_UNORM:
        result = 4;
        break;
    case VK_FORMAT_B8G8R8A8_SNORM:
        result = 4;
        break;
    case VK_FORMAT_B8G8R8A8_USCALED:
        result = 4;
        break;
    case VK_FORMAT_B8G8R8A8_SSCALED:
        result = 4;
        break;
    case VK_FORMAT_B8G8R8A8_UINT:
        result = 4;
        break;
    case VK_FORMAT_B8G8R8A8_SINT:
        result = 4;
        break;
    case VK_FORMAT_B8G8R8A8_SRGB:
        result = 4;
        break;
    case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2R10G10B10_UINT_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2R10G10B10_SINT_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        result = 4;
        break;
    case VK_FORMAT_A2B10G10R10_SINT_PACK32:
        result = 4;
        break;
    case VK_FORMAT_R16_UNORM:
        result = 2;
        break;
    case VK_FORMAT_R16_SNORM:
        result = 2;
        break;
    case VK_FORMAT_R16_USCALED:
        result = 2;
        break;
    case VK_FORMAT_R16_SSCALED:
        result = 2;
        break;
    case VK_FORMAT_R16_UINT:
        result = 2;
        break;
    case VK_FORMAT_R16_SINT:
        result = 2;
        break;
    case VK_FORMAT_R16_SFLOAT:
        result = 2;
        break;
    case VK_FORMAT_R16G16_UNORM:
        result = 4;
        break;
    case VK_FORMAT_R16G16_SNORM:
        result = 4;
        break;
    case VK_FORMAT_R16G16_USCALED:
        result = 4;
        break;
    case VK_FORMAT_R16G16_SSCALED:
        result = 4;
        break;
    case VK_FORMAT_R16G16_UINT:
        result = 4;
        break;
    case VK_FORMAT_R16G16_SINT:
        result = 4;
        break;
    case VK_FORMAT_R16G16_SFLOAT:
        result = 4;
        break;
    case VK_FORMAT_R16G16B16_UNORM:
        result = 6;
        break;
    case VK_FORMAT_R16G16B16_SNORM:
        result = 6;
        break;
    case VK_FORMAT_R16G16B16_USCALED:
        result = 6;
        break;
    case VK_FORMAT_R16G16B16_SSCALED:
        result = 6;
        break;
    case VK_FORMAT_R16G16B16_UINT:
        result = 6;
        break;
    case VK_FORMAT_R16G16B16_SINT:
        result = 6;
        break;
    case VK_FORMAT_R16G16B16_SFLOAT:
        result = 6;
        break;
    case VK_FORMAT_R16G16B16A16_UNORM:
        result = 8;
        break;
    case VK_FORMAT_R16G16B16A16_SNORM:
        result = 8;
        break;
    case VK_FORMAT_R16G16B16A16_USCALED:
        result = 8;
        break;
    case VK_FORMAT_R16G16B16A16_SSCALED:
        result = 8;
        break;
    case VK_FORMAT_R16G16B16A16_UINT:
        result = 8;
        break;
    case VK_FORMAT_R16G16B16A16_SINT:
        result = 8;
        break;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        result = 8;
        break;
    case VK_FORMAT_R32_UINT:
        result = 4;
        break;
    case VK_FORMAT_R32_SINT:
        result = 4;
        break;
    case VK_FORMAT_R32_SFLOAT:
        result = 4;
        break;
    case VK_FORMAT_R32G32_UINT:
        result = 8;
        break;
    case VK_FORMAT_R32G32_SINT:
        result = 8;
        break;
    case VK_FORMAT_R32G32_SFLOAT:
        result = 8;
        break;
    case VK_FORMAT_R32G32B32_UINT:
        result = 12;
        break;
    case VK_FORMAT_R32G32B32_SINT:
        result = 12;
        break;
    case VK_FORMAT_R32G32B32_SFLOAT:
        result = 12;
        break;
    case VK_FORMAT_R32G32B32A32_UINT:
        result = 16;
        break;
    case VK_FORMAT_R32G32B32A32_SINT:
        result = 16;
        break;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        result = 16;
        break;
    case VK_FORMAT_R64_UINT:
        result = 8;
        break;
    case VK_FORMAT_R64_SINT:
        result = 8;
        break;
    case VK_FORMAT_R64_SFLOAT:
        result = 8;
        break;
    case VK_FORMAT_R64G64_UINT:
        result = 16;
        break;
    case VK_FORMAT_R64G64_SINT:
        result = 16;
        break;
    case VK_FORMAT_R64G64_SFLOAT:
        result = 16;
        break;
    case VK_FORMAT_R64G64B64_UINT:
        result = 24;
        break;
    case VK_FORMAT_R64G64B64_SINT:
        result = 24;
        break;
    case VK_FORMAT_R64G64B64_SFLOAT:
        result = 24;
        break;
    case VK_FORMAT_R64G64B64A64_UINT:
        result = 32;
        break;
    case VK_FORMAT_R64G64B64A64_SINT:
        result = 32;
        break;
    case VK_FORMAT_R64G64B64A64_SFLOAT:
        result = 32;
        break;
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        result = 4;
        break;
    case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
        result = 4;
        break;

    default:
        break;
    }
    return result;
}

VkFormat helper::getVkFormat(Format format) {
    switch (format) {
    case Format::R16_FLOAT: {
        return VK_FORMAT_R16_SFLOAT;
        break;
    }
    case Format::R32_FLOAT: {
        return VK_FORMAT_R32_SFLOAT;
        break;
    }
    case Format::RG16_FLOAT: {
        return VK_FORMAT_R16G16_SFLOAT;
        break;
    }
    case Format::RG32_FLOAT: {
        return VK_FORMAT_R32G32_SFLOAT;
        break;
    }
    case Format::RGB16_FLOAT: {
        return VK_FORMAT_R16G16B16_SFLOAT;
        break;
    }
    case Format::RGB32_FLOAT: {
        return VK_FORMAT_R32G32B32_SFLOAT;
        break;
    }
    case Format::RGBA16_FLOAT: {
        return VK_FORMAT_R16G16B16A16_SFLOAT;
        break;
    }
    case Format::RGBA32_FLOAT: {
        return VK_FORMAT_R32G32B32A32_SFLOAT;
        break;
    }
    }

    return VK_FORMAT_UNDEFINED;
}

VkDescriptorType helper::getVkDescriptorType(DescriptorType type) {
    switch (type) {
    case DescriptorType::UNIFORM_BUFFER: {
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        break;
    }
    case DescriptorType::STORAGE_BUFFER: {
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        break;
    }
    }

    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
}
