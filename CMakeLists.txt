cmake_minimum_required(VERSION 3.15)

project(cpp_vulkan_conan_template CXX)

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(VulkanHeaders REQUIRED)
find_package(vk-bootstrap REQUIRED)
find_package(vulkan-memory-allocator REQUIRED)
find_package(imgui REQUIRED)
find_package(stb REQUIRED)
find_package(spdlog REQUIRED)
find_package(EnTT REQUIRED)
find_package(shaderc REQUIRED)
find_package(TinyGLTF REQUIRED)
find_package(tinyobjloader REQUIRED)

file(GLOB_RECURSE SOURCES
    src/*.hpp
    src/*.cpp
)


add_executable( ${PROJECT_NAME}
                ${SOURCES}              
                build/imgui_impl_glfw.cpp
                build/imgui_impl_glfw.h
                build/imgui_impl_vulkan.h
                thirdparty/SPIRV-Reflect/spirv_reflect.cpp
                thirdparty/SPIRV-Reflect/spirv_reflect.h)

target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} glm::glm)
target_link_libraries(${PROJECT_NAME} glfw)
target_link_libraries(${PROJECT_NAME} Vulkan::Headers)
target_link_libraries(${PROJECT_NAME} vk-bootstrap::vk-bootstrap)
target_link_libraries(${PROJECT_NAME} vulkan-memory-allocator::vulkan-memory-allocator)
target_link_libraries(${PROJECT_NAME} imgui::imgui)
target_link_libraries(${PROJECT_NAME} stb::stb)
target_link_libraries(${PROJECT_NAME} spdlog::spdlog)
target_link_libraries(${PROJECT_NAME} EnTT::EnTT)
target_link_libraries(${PROJECT_NAME} shaderc::shaderc)
target_link_libraries(${PROJECT_NAME} TinyGLTF::TinyGLTF)
target_link_libraries(${PROJECT_NAME} tinyobjloader::tinyobjloader)

target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.hpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "build/${CMAKE_BUILD_TYPE}")