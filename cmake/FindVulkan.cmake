set(Vulkan_FOUND TRUE)
set(VULKAN_FOUND TRUE)

set(Vulkan_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/vulkan/include")
set(Vulkan_INCLUDE_DIRS "${Vulkan_INCLUDE_DIR}")

if(NOT TARGET Vulkan::Vulkan)
    add_library(Vulkan::Vulkan INTERFACE IMPORTED)
    set_target_properties(Vulkan::Vulkan PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}")
endif()

if(NOT TARGET Vulkan::Headers)
    add_library(Vulkan::Headers INTERFACE IMPORTED)
    set_target_properties(Vulkan::Headers PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}")
endif()
