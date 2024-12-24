#pragma once

#include "../Config.hpp"

struct FrameBasedFence {
    VkDevice device;

    std::array<VkFence, FRAME_OVERLAP> fences;

    FrameBasedFence(VkDevice device, std::array<VkFence, FRAME_OVERLAP> fences);

    ~FrameBasedFence();
};

struct FrameBasedSemaphore {
    VkDevice device;

    std::array<VkSemaphore, FRAME_OVERLAP> semaphores;

    FrameBasedSemaphore(VkDevice device, std::array<VkSemaphore, FRAME_OVERLAP> semaphores);

    ~FrameBasedSemaphore();
};