# Spolay

Minimal viewer/overlay for Spotify

## Dependencies

This project uses the following dependencies:

- [VulkanSDK](https://vulkan.lunarg.com/) (INSTALL SEPARATELY)
- [OpenSSL](https://github.com/openssl/openssl) (if crosscompiling for windows, cmake will download and use msys2 openssl)
- [stb](https://github.com/nothings/stb)
- [SDL3](https://github.com/libsdl-org/SDL)
- [volk](https://github.com/zeux/volk)
- [glm](https://github.com/g-truc/glm)
- [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [embdfs](https://github.com/gdar463/embdfs)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [nlohmann::json](https://github.com/nlohmann/json)

## Building

### For linux

Either use CMake as you normally would or run [configure.sh](configure.sh)

### For windows

NYI

### For windows (cross-compiling from linux using llvm-mingw)

First of all install `llvm-mingw` and add its directory to PATH, then either use CMake with the flag `-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/windows-mingw.cmake` or run [configure.sh](configure.sh) with the arg `wsl`.

## Running

### For linux

Just launch `spolay` from the build directory (if using [configure.sh](configure.sh), the default build dir is `build/linux/debug`).

### For windows

Copy `SDL3.dll` from `third_party/sdl` in the build directory (if using [configure.sh](configure.sh), the default build dir is `build/windows/debug`) in the same folder as `spolay.exe` (found in the build directory) and then launch the latter.

## License

To see how this repo is licensed checkout [LICENSE.md](LICENSE.md).
