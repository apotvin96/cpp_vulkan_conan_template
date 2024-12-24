#pragma once

#include "../../pch.hpp"

#include "../Config.hpp"

struct CommandBuffer {
    VkDevice device;

    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;

    CommandBuffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer);

    ~CommandBuffer();
};

struct FrameBasedCommandBuffer {
    VkDevice device;

    std::array<VkCommandPool, FRAME_OVERLAP> commandPools;
    std::array<VkCommandBuffer, FRAME_OVERLAP> commandBuffers;

    FrameBasedCommandBuffer(VkDevice device, std::array<VkCommandPool, FRAME_OVERLAP> commandPools,
                            std::array<VkCommandBuffer, FRAME_OVERLAP> commandBuffers);

    ~FrameBasedCommandBuffer();
};
