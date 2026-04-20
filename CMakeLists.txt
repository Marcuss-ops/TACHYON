cmake_minimum_required(VERSION 3.15)
project(TACHYON VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Determine platform and compiler details
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/wd4324) # structure was padded due to alignment specifier (Embree)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Header-only JSON dependency for the first scene/job parser slice
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# FreeType for scalable font rendering (Musk-YouTube Edition)
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG        VER-2-13-3
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(SKIP_INSTALL_ALL ON CACHE BOOL "Skip FreeType install" FORCE)
set(FT_WITH_ZLIB OFF CACHE BOOL "Disable ZLib for FreeType" FORCE)
set(FT_WITH_BZIP2 OFF CACHE BOOL "Disable BZip2 for FreeType" FORCE)
set(FT_WITH_PNG OFF CACHE BOOL "Disable PNG for FreeType" FORCE)
set(FT_WITH_HARFBUZZ OFF CACHE BOOL "Disable HarfBuzz for FreeType" FORCE)
# CMake 4.0 dropped backward compat with cmake_minimum_required < 3.5.
# Override policy so FreeType's old CMakeLists.txt does not abort configure.
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
FetchContent_MakeAvailable(freetype)

# HarfBuzz for OpenType shaping and ligatures.
FetchContent_Declare(
    harfbuzz
    GIT_REPOSITORY https://github.com/harfbuzz/harfbuzz.git
    GIT_TAG        14.1.0
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(HB_BUILD_UTILS OFF CACHE BOOL "Disable HarfBuzz command line utilities" FORCE)
set(HB_BUILD_TESTS OFF CACHE BOOL "Disable HarfBuzz tests" FORCE)
set(HB_BUILD_DOCS OFF CACHE BOOL "Disable HarfBuzz docs" FORCE)
set(HB_BUILD_SUBSET OFF CACHE BOOL "Disable HarfBuzz subset library" FORCE)
set(HB_HAVE_FREETYPE ON CACHE BOOL "Enable HarfBuzz FreeType integration" FORCE)
FetchContent_MakeAvailable(harfbuzz)

# Intel Embree CPU Ray Tracing Kernels
FetchContent_Declare(
    embree
    GIT_REPOSITORY https://github.com/embree/embree.git
    GIT_TAG        v4.3.3
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(EMBREE_TUTORIALS OFF CACHE BOOL "Disable Embree tutorials" FORCE)
set(EMBREE_BACKEND_ISPC OFF CACHE BOOL "Disable Embree ISPC backend" FORCE)
set(EMBREE_ISPC_SUPPORT OFF CACHE BOOL "Disable ISPC support" FORCE)
set(EMBREE_STATIC_LIB ON CACHE BOOL "Build Embree as static library" FORCE)
set(EMBREE_RAY_MASK ON CACHE BOOL "Enable Embree ray masking" FORCE)
set(EMBREE_FILTER_FUNCTION ON CACHE BOOL "Enable Embree filter functions" FORCE)
set(EMBREE_TASKING_SYSTEM INTERNAL CACHE STRING "Use internal tasking system for Embree" FORCE)
# Build only for the host architecture
set(EMBREE_MAX_ISA "SSE4.2" CACHE STRING "Max ISA for Embree" FORCE) 
# tinygltf for glTF mesh loading
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG        v2.9.3
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(tinygltf)

# OpenImageDenoise (OIDN) for high-quality path tracing results at low SPP
FetchContent_Declare(
    oidn
    GIT_REPOSITORY https://github.com/OpenImageDenoise/oidn.git
    GIT_TAG        v2.3.0
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(OIDN_FILTER_RT ON CACHE BOOL "Enable Ray Tracing filter" FORCE)
set(OIDN_APPS OFF CACHE BOOL "Disable OIDN apps" FORCE)
FetchContent_MakeAvailable(oidn)

# earcut.hpp for 3D polygon triangulation (Extrusion)
FetchContent_Declare(
    earcut
    GIT_REPOSITORY https://github.com/mapbox/earcut.hpp.git
    GIT_TAG        v2.2.4
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(earcut)
# earcut just provides the 'earcut.hpp' header inside its include/mapbox folder.
# We add its include directory globally
include_directories(${earcut_SOURCE_DIR}/include)

FetchContent_MakeAvailable(embree)

# Version header generation
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/tachyon/core/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/tachyon/version.h
)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)

# Subdirectories
add_subdirectory(src)
add_subdirectory(tests)
# add_subdirectory(examples)
