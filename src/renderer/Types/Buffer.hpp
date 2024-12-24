#pragma once

#include "../../pch.hpp"

struct VertexBuffer {
    VmaAllocator allocator;

    VkBuffer buffer;
    VmaAllocation allocation;

    VertexBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation);

    ~VertexBuffer();
};