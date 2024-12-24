#pragma once
#include "../../pch.hpp"

#include "../Types/Pipeline.hpp"
#include "../Types/Renderpass.hpp"

namespace helper {
    VkAttachmentLoadOp getVkAttachmentLoadOp(LoadOp loadOp);

    VkAttachmentStoreOp getVkAttachmentStoreOp(StoreOp storeOp);

    VkImageLayout getVkImageLayout(ImageLayout imageLayout, bool isDepthImage = false);

    VkAccessFlagBits getVkAccessFlags(ImageLayout imageLayout, AccessType accessType,
                                      bool isDepthImage = false);

    uint32_t vkFormatAsBytes(VkFormat format);

    VkFormat getVkFormat(Format format);

    VkDescriptorType getVkDescriptorType(DescriptorType type);
} // namespace helper
