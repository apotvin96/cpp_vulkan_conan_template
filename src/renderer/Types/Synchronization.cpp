#include "../../pch.hpp"
#include "Synchronization.hpp"

#include "../../Logger.hpp"

FrameBasedFence::FrameBasedFence(VkDevice device, std::array<VkFence, FRAME_OVERLAP> fences)
    : device(device), fences(fences) {}

FrameBasedFence::~FrameBasedFence() {
    Logger::renderer_logger->info("Destroying FrameBased Fence");

    if (device != VK_NULL_HANDLE) {
        for (auto& fence : fences) {
            if (fence != VK_NULL_HANDLE) {
                vkDestroyFence(device, fence, nullptr);
            }
        }
    }
}

FrameBasedSemaphore::FrameBasedSemaphore(VkDevice device,
                                         std::array<VkSemaphore, FRAME_OVERLAP> semaphores)
    : device(device), semaphores(semaphores) {}

FrameBasedSemaphore::~FrameBasedSemaphore() {
    Logger::renderer_logger->info("Destroying Semaphore");

    if (device != VK_NULL_HANDLE) {
        for (auto& semaphore : semaphores) {
            if (semaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(device, semaphore, nullptr);
            }
        }
    }
}
