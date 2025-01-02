#include "../../pch.hpp"
#include "Commands.hpp"

#include "../../Logger.hpp"

CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool commandPool,
                             VkCommandBuffer commandBuffer)
    : device(device), commandPool(commandPool), commandBuffer(commandBuffer) {}

CommandBuffer::~CommandBuffer() {
    Logger::renderer_logger->info("Destroying Command Buffer");

    if (device != VK_NULL_HANDLE) {
        if (commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device, commandPool, nullptr);
        }
    }
}

FrameBasedCommandBuffer::FrameBasedCommandBuffer(
    VkDevice device, std::array<VkCommandPool, FRAME_OVERLAP> commandPools,
    std::array<VkCommandBuffer, FRAME_OVERLAP> commandBuffers)
    : device(device), commandPools(commandPools), commandBuffers(commandBuffers) {}

FrameBasedCommandBuffer::~FrameBasedCommandBuffer() {
    Logger::renderer_logger->info("Destroying FrameBased Command Buffer");

    if (device != VK_NULL_HANDLE) {
        for (auto& pool : commandPools) {
            if (pool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(device, pool, nullptr);
            }
        }
    }
}
