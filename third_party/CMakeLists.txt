add_compile_options(-w -O0 -g)

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE stb/)

set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(sdl EXCLUDE_FROM_ALL)

add_subdirectory(volk EXCLUDE_FROM_ALL)
add_subdirectory(glm EXCLUDE_FROM_ALL)

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
target_compile_definitions(imgui PRIVATE
    IMGUI_IMPL_VULKAN_USE_VOLK VK_NO_PROTOTYPES)
target_include_directories(imgui SYSTEM PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends)
target_link_libraries(imgui PRIVATE SDL3::SDL3 volk::volk Vulkan::Vulkan)

set(EMBDFS_PATH "${PROJECT_SOURCE_DIR}/assets")
add_subdirectory(embdfs EXCLUDE_FROM_ALL)

set(HTTPLIB_INSTALL OFF CACHE BOOL "" FORCE)
set(HTTPLIB_TEST OFF CACHE BOOL "" FORCE)
set(HTTPLIB_REQUIRE_OPENSSL ON CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZSTD_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
add_subdirectory(httplib EXCLUDE_FROM_ALL)
