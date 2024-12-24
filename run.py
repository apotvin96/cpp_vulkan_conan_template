import platform
import subprocess

if __name__ == "__main__":
    result = subprocess.run((
        'cmake', '--build', '.'
    ), cwd='./build')

    if (result.returncode == 0):
        match platform.system():
            case "Windows":
                subprocess.run((
                    './build/Debug/cpp_vulkan_conan_template.exe'
                ))
            case "Linux":
                subprocess.run((
                    './build/cpp_vulkan_conan_template'
                ))
            case "Darwin":
                subprocess.run((
                    './build/cpp_vulkan_conan_template'
                ))
            case _:
                subprocess.run((
                    './build/cpp_vulkan_conan_template'
                ))
