#pragma once

// TODO: MAKE THIS ACTUALLY USE THE LOGGER AGAIN
#define VK_CHECK(x)                                                                                \
    do {                                                                                           \
        VkResult err = x;                                                                          \
        if (err) {                                                                                 \
            abort();                                                                               \
        }                                                                                          \
    } while (0)
