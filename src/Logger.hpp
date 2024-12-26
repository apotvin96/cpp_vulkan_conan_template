#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <vulkan/vulkan.h>

class Logger {
public:
    static std::vector<spdlog::sink_ptr> sinks;
    static std::shared_ptr<spdlog::logger> audio_logger;
    static std::shared_ptr<spdlog::logger> ecs_logger;
    static std::shared_ptr<spdlog::logger> main_logger;
    static std::shared_ptr<spdlog::logger> physics_logger;
    static std::shared_ptr<spdlog::logger> renderer_logger;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {

        // if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        //     renderer_logger->warn(callbackData->pMessage);
        // } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        //     renderer_logger->error(callbackData->pMessage);
        // }

        return VK_FALSE;
    }

    static void init();

    Logger() = delete;
};
