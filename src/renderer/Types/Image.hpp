#include "../../pch.hpp"

struct AllocatedImage {
    VkImage image;
    VmaAllocation allocation;
};