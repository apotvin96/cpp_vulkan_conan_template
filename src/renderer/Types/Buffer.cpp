#include "../../pch.hpp"
#include "Buffer.hpp"

#include "../../Logger.hpp"

VertexBuffer::VertexBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation)
    : allocator(allocator), buffer(buffer), allocation(allocation) {}

VertexBuffer::~VertexBuffer() {
    Logger::renderer_logger->info("Destroying Vertex Buffer");

    if (allocator != VK_NULL_HANDLE && buffer != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }
}
