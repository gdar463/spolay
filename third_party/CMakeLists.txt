add_compile_options(-w -O0 -g)

# Needed for zlib for crosscompiling
include(FetchContent)

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE stb/)

set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(sdl)

add_subdirectory(volk)
add_subdirectory(glm)

add_library(vma INTERFACE)
target_include_directories(vma SYSTEM INTERFACE vma/include)

set(IMGUI_DIR ${PROJECT_SOURCE_DIR}/third_party/imgui)
file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_VULKAN_USE_VOLK VK_NO_PROTOTYPES)
target_include_directories(imgui SYSTEM PUBLIC 
    ${IMGUI_DIR} 
    ${IMGUI_DIR}/backends)
target_link_libraries(imgui PRIVATE SDL3::SDL3 volk::volk Vulkan::Vulkan)

FetchContent_Declare(
    zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG e3dc0a85b7032e98380dec011bc8f2c2ee0d8fca
    OVERRIDE_FIND_PACKAGE
)

set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ZLIB_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(zlib)

set(LIBROMFS_PROJECT_NAME "spolay")
set(LIBROMFS_RESOURCE_LOCATION "${PROJECT_SOURCE_DIR}/assets")
add_subdirectory(romfs)
set(LIBROMFS_LIBRARY ${LIBROMFS_LIBRARY} PARENT_SCOPE)
