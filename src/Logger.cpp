#include "pch.hpp"
#include "Logger.hpp"

std::vector<spdlog::sink_ptr> Logger::sinks;
std::shared_ptr<spdlog::logger> Logger::audio_logger;
std::shared_ptr<spdlog::logger> Logger::ecs_logger;
std::shared_ptr<spdlog::logger> Logger::main_logger;
std::shared_ptr<spdlog::logger> Logger::physics_logger;
std::shared_ptr<spdlog::logger> Logger::renderer_logger;

void Logger::init() {
    spdlog::set_pattern("[%H:%M:%S:%e][%n][%^%l%$] %v");

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

#ifdef NDEBUG
    console_sink->set_level(spdlog::level::warn);
#else
    console_sink->set_level(spdlog::level::info);
#endif

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("game.log");
    file_sink->set_level(spdlog::level::trace);

    sinks.push_back(console_sink);
    sinks.push_back(file_sink);

    audio_logger    = std::make_shared<spdlog::logger>("audio", begin(sinks), end(sinks));
    ecs_logger      = std::make_shared<spdlog::logger>("ecs", begin(sinks), end(sinks));
    main_logger     = std::make_shared<spdlog::logger>("main", begin(sinks), end(sinks));
    physics_logger  = std::make_shared<spdlog::logger>("physics", begin(sinks), end(sinks));
    renderer_logger = std::make_shared<spdlog::logger>("renderer", begin(sinks), end(sinks));

    main_logger->set_level(spdlog::level::trace);

    main_logger->trace("----- Logger initialized -----");
}