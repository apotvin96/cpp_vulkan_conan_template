#include "../../pch.hpp"
#include "Initializers.hpp"

VkImageCreateInfo helper::imageCreateInfo(VkFormat format, VkImageUsageFlags flags,
                                          VkExtent3D extent) {
    VkImageCreateInfo info = {};
    info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext             = nullptr;
    info.imageType         = VK_IMAGE_TYPE_2D;
    info.format            = format;
    info.extent            = extent;
    info.mipLevels         = 1;
    info.arrayLayers       = 1;
    info.samples           = VK_SAMPLE_COUNT_1_BIT;
    info.tiling            = VK_IMAGE_TILING_OPTIMAL;
    info.usage             = flags;

    return info;
}

VkImageViewCreateInfo helper::imageViewCreateInfo(VkFormat format, VkImage image,
                                                  VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo info           = {};
    info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext                           = nullptr;
    info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    info.image                           = image;
    info.format                          = format;
    info.subresourceRange.baseMipLevel   = 0;
    info.subresourceRange.levelCount     = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount     = 1;
    info.subresourceRange.aspectMask     = aspectFlags;

    return info;
}

VkCommandPoolCreateInfo helper::commandPoolCreateInfo(uint32_t queueFamily,
                                                      VkCommandPoolCreateFlags flags) {
    VkCommandPoolCreateInfo info = {};
    info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext                   = nullptr;
    info.queueFamilyIndex        = queueFamily;
    info.flags                   = flags;

    return info;
}

VkCommandBufferAllocateInfo helper::commandBufferAllocateInfo(VkCommandPool commandPool,
                                                              uint32_t count,
                                                              VkCommandBufferLevel level) {
    VkCommandBufferAllocateInfo info = {};
    info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext                       = nullptr;
    info.commandPool                 = commandPool;
    info.commandBufferCount          = count;
    info.level                       = level;

    return info;
}

VkCommandBufferBeginInfo helper::commandBufferBeginInfo(VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo info = {};
    info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext                    = nullptr;
    info.flags                    = flags;

    return info;
}

VkSubmitInfo helper::submitInfo(VkCommandBuffer* cmd) {
    VkSubmitInfo info = {};
    info.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.pNext        = nullptr;

    info.waitSemaphoreCount   = 0;
    info.pWaitSemaphores      = nullptr;
    info.pWaitDstStageMask    = nullptr;
    info.commandBufferCount   = 1;
    info.pCommandBuffers      = cmd;
    info.signalSemaphoreCount = 0;
    info.pSignalSemaphores    = nullptr;

    return info;
}

VkFenceCreateInfo helper::fenceCreateInfo(VkFenceCreateFlags flags) {
    VkFenceCreateInfo info = {};
    info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext             = nullptr;
    info.flags             = flags;

    return info;
}

VkPipelineLayoutCreateInfo helper::pipelineLayoutCreateInfo() {
    VkPipelineLayoutCreateInfo info = {};
    info.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext                      = nullptr;
    info.flags                      = 0;
    info.setLayoutCount             = 0;
    info.pSetLayouts                = nullptr;
    info.pushConstantRangeCount     = 0;
    info.pPushConstantRanges        = nullptr;

    return info;
}
