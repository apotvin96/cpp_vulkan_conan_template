from conan import ConanFile
from conan.tools.files import copy
import platform

class CompressorRecipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("spdlog/1.14.1")
        self.requires("glfw/3.4")
        self.requires("vulkan-headers/1.3.290.0", force=True)
        self.requires("vk-bootstrap/0.7")
        self.requires("vulkan-memory-allocator/cci.20231120")
        self.requires("glm/cci.20230113") 
        self.requires("imgui/cci.20230105+1.89.2.docking")
        self.requires("stb/cci.20230920")
        self.requires("entt/3.13.2")
        self.requires("shaderc/2024.1")
        self.requires("tinyobjloader/2.0.0-rc10")
        self.requires("tinygltf/2.9.0")

    def generate(self):
        imgui = self.dependencies["imgui"]
        
        print (f"{imgui.cpp_info.includedirs[0]}/../res/bindings")
        
        copy(self, "imgui_impl_glfw.cpp", f"{imgui.cpp_info.includedirs[0]}/../res/bindings", self.build_folder)
        copy(self, "imgui_impl_glfw.h",   f"{imgui.cpp_info.includedirs[0]}/../res/bindings", self.build_folder)
        copy(self, "imgui_impl_vulkan.h", f"{imgui.cpp_info.includedirs[0]}/../res/bindings", self.build_folder)
