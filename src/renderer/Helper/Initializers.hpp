#include "../../pch.hpp"

namespace helper {
    // Images
    VkImageCreateInfo imageCreateInfo(VkFormat format, VkImageUsageFlags flags, VkExtent3D extent);

    VkImageViewCreateInfo imageViewCreateInfo(VkFormat format, VkImage image,
                                              VkImageAspectFlags aspectFlags);

    // Commands
    VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamily,
                                                  VkCommandPoolCreateFlags flags = 0);

    VkCommandBufferAllocateInfo
    commandBufferAllocateInfo(VkCommandPool commandPool, uint32_t count = 1,
                              VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags);

    VkSubmitInfo submitInfo(VkCommandBuffer* cmd);

    // Sync Objects
    VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags = 0);

    // Pipelines
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo();

} // namespace helper