cmake_minimum_required(VERSION 3.21)

project(
    SpoLay
    VERSION 0.1
    DESCRIPTION "Minimal viewer/overlay for Spotify"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DEBUG ON)
else()
    set(DEBUG OFF)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

###
### Compiler Flags
###
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unknown-pragmas)
endif()

if(NOT DEFINED USE_ASAN)
    set(USE_ASAN OFF)
endif()

###
### Load cmake/scripts before standard scripts
###
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

###
### External Dependencies
###
find_package(Vulkan REQUIRED)

# moved setup to separate Lists.txt to shut up all warnings
add_subdirectory(third_party)

###
### Actual project
###
# file(GLOB_RECURSE SPOLAY_HEADERS CONFIGURE_DEPENDS
#     "${PROJECT_SOURCE_DIR}/include/spolay/*.hpp")
file(GLOB_RECURSE SPOLAY_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.cpp" "${PROJECT_SOURCE_DIR}/src/*.hpp")

add_executable(spolay
    # ${SPOLAY_HEADERS}
    ${SPOLAY_SOURCES})

target_include_directories(spolay PRIVATE
    include
    src)

target_link_libraries(spolay PRIVATE
    stb
    SDL3::SDL3
    Vulkan::Vulkan
    volk::volk
    vma
    glm::glm
    imgui
    ${LIBROMFS_LIBRARY})

target_compile_features(spolay PUBLIC cxx_std_23)

# source_group(
#     TREE "${PROJECT_SOURCE_DIR}/include"
#     PREFIX "Header Files"
#     FILES ${SPOLAY_HEADERS})
source_group(
    TREE "${PROJECT_SOURCE_DIR}/src"
    PREFIX "Source Files"
    FILES ${SPOLAY_SOURCES})

if(DEBUG)
    target_compile_definitions(spolay PRIVATE DEBUG)
    if(NO_VALIDATION_LAYER)
        target_compile_definitions(spolay PRIVATE NO_VALIDATION_LAYER)
    endif()
    if(NO_DEBUG_UTILS)
        target_compile_definitions(spolay PRIVATE NO_DEBUG_UTILS)
    endif()
endif()

# if(WIN32)
#     set_target_properties(spolay PROPERTIES
#         WIN32_EXECUTABLE $<CONFIG:Relase>)
# endif()

if(UNIX AND DEBUG AND USE_ASAN)
    target_compile_options(spolay PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -g
    )

    target_link_options(spolay PRIVATE
        -fsanitize=address,undefined
    )
endif()
