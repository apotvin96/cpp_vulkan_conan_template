import platform
import subprocess

def RunConan(build_type):
    subprocess.run((
        'conan', 'install', '.', 
        '--build', 'missing', 
        '--output-folder=./build', 
        f'--settings=build_type={build_type}'
    ))

def RunCMake():
    print("Running CMake")

    match platform.system():
        case "Windows":
            subprocess.run((
                "cmake", "..", "-G", "Visual Studio 17 2022", "-DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake"
            ), cwd="./build")
        case "Linux":
            subprocess.run((
                'cmake', '..',
                '-DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake',
                '-DCMAKE_BUILD_TYPE=Debug'
            ), cwd='./build')
        case "Darwin":
            subprocess.run((
                'cmake', '..',
                '-DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake',
                '-DCMAKE_BUILD_TYPE=Debug'
            ), cwd='./build')
        case _:
            subprocess.run((
                'cmake', '..',
                '-DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake',
                '-DCMAKE_BUILD_TYPE=Debug'
            ), cwd='./build')

if __name__ == "__main__":
    RunConan("Debug")
    #RunConan("Release")
    RunCMake()  