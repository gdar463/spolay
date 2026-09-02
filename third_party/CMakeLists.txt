add_compile_options(-w -O0 -g)

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE stb/)

set(SDL_TEST OFF CACHE BOOL "" FORCE)
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
