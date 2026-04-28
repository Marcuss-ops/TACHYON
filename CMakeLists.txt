cmake_minimum_required(VERSION 3.15)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
endif()
# Suppress deprecation warnings from dependencies (e.g., freetype with old cmake_minimum_required)
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "Suppress deprecation warnings" FORCE)
project(TACHYON VERSION 0.1.0 LANGUAGES CXX)

# Set FetchContent base directory before including FetchContent
if(NOT DEFINED FETCHCONTENT_BASE_DIR)
    if(WIN32)
        set(_fc_suffix "ninja-msvc-x64")
    elseif(APPLE)
        set(_fc_suffix "ninja-clang-macos")
    else()
        set(_fc_suffix "ninja-gcc-linux")
    endif()
    set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.cache/fetchcontent/${_fc_suffix}" CACHE PATH "Base directory for FetchContent downloads")
endif()
include(FetchContent)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Studio-grade performance tracking: Tracy Profiler
option(TACHYON_ENABLE_TRACY "Enable Tracy profiler for development" OFF)
if(TACHYON_ENABLE_TRACY)
    FetchContent_Declare(tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy
        GIT_TAG v0.11.1
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(tracy)
endif()

# Unity Build support (Jumbo) - applied only to Tachyon targets
option(TACHYON_UNITY_BUILD "Enable unity build for Tachyon targets" OFF)
if(TACHYON_UNITY_BUILD)
    set(TACHYON_UNITY_TARGETS TachyonCore tachyon TachyonTests)
    foreach(tgt IN LISTS TACHYON_UNITY_TARGETS)
        if(TARGET ${tgt})
            set_target_properties(${tgt} PROPERTIES
                UNITY_BUILD ON
                UNITY_BUILD_BATCH_SIZE 8
            )
        endif()
    endforeach()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TACHYON_ENABLE_COMPILER_CACHE "Enable sccache/clcache when available" ON)
find_package(OpenMP REQUIRED)
option(TACHYON_ENABLE_PCH "Enable precompiled headers when supported" ON)
option(TACHYON_ENABLE_ASAN "Enable AddressSanitizer on Tachyon targets" OFF)
option(TACHYON_TREAT_WARNINGS_AS_ERRORS "Treat warnings as errors (Studio-grade strictness)" ON)

if(MSVC)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")
    add_compile_options(/FS)
else()
    # GCC/Clang Release optimizations
    add_compile_options($<$<CONFIG:Release>:-O3>)
endif()

if(TACHYON_ENABLE_ASAN)
    # ASAN-specific flags here if needed
endif()

# Determine platform and compiler details
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DNOMINMAX)
    add_compile_options(/W4 /wd4324) # /W4 enables common warnings, wd4324 disables padding warning
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(TACHYON_ENABLE_COMPILER_CACHE)
    find_program(TACHYON_COMPILER_LAUNCHER NAMES sccache ccache clcache)
    if(TACHYON_COMPILER_LAUNCHER)
        message(STATUS "Using compiler launcher: ${TACHYON_COMPILER_LAUNCHER}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${TACHYON_COMPILER_LAUNCHER}" CACHE FILEPATH "Compiler launcher" FORCE)
        set(CMAKE_C_COMPILER_LAUNCHER "${TACHYON_COMPILER_LAUNCHER}" CACHE FILEPATH "Compiler launcher" FORCE)
    endif()
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
    CMAKE_CACHE_ARGS
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        -DSKIP_INSTALL_ALL:BOOL=ON
        -DFT_WITH_ZLIB:BOOL=OFF
        -DFT_WITH_BZIP2:BOOL=OFF
        -DFT_WITH_PNG:BOOL=OFF
        -DFT_WITH_HARFBUZZ:BOOL=OFF
        -DCMAKE_WARN_DEPRECATED:BOOL=OFF
        -DCMAKE_POLICY_DEFAULT_CMP0048:NEW
        -DCMAKE_POLICY_DEFAULT_CMP0141:NEW
)
# CMake 4.0 dropped backward compat with cmake_minimum_required < 3.5.
# Override policy so FreeType's old CMakeLists.txt does not abort configure.
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0141 NEW)
FetchContent_MakeAvailable(freetype)

# Fix FreeType stddef.h issue on Windows: add UCRT include path
if(MSVC AND freetype_SOURCE_DIR)
    # Find Windows SDK UCRT path
    if(EXISTS "C:/Program Files (x86)/Windows Kits/10/Include")
        file(GLOB SDK_DIRS "C:/Program Files (x86)/Windows Kits/10/Include/*")
        if(SDK_DIRS)
            # Sort to get latest SDK version
            list(SORT SDK_DIRS ORDER DESCENDING)
            list(GET SDK_DIRS 0 LATEST_SDK_DIR)
            get_filename_component(SDK_VERSION ${LATEST_SDK_DIR} NAME)
            set(UCRT_INCLUDE_PATH "C:/Program Files (x86)/Windows Kits/10/Include/${SDK_VERSION}/ucrt")
            if(EXISTS ${UCRT_INCLUDE_PATH})
                message(STATUS "[Tachyon] Adding UCRT path for FreeType: ${UCRT_INCLUDE_PATH}")
                # Add include path to freetype targets
                get_property(freetype_targets DIRECTORY "${freetype_SOURCE_DIR}" PROPERTY BUILDSYSTEM_TARGETS)
                if(freetype_targets)
                    foreach(target IN LISTS freetype_targets)
                        if(TARGET ${target})
                            get_target_property(_type ${target} TYPE)
                            if(_type STREQUAL "INTERFACE_LIBRARY")
                                target_include_directories(${target} INTERFACE ${UCRT_INCLUDE_PATH})
                            else()
                                target_include_directories(${target} PUBLIC ${UCRT_INCLUDE_PATH})
                            endif()
                        endif()
                    endforeach()
                endif()
            endif()
        endif()
    endif()
endif()

# Fix: force /Z7 (Embedded) for all freetype targets to avoid PDB conflicts
if(MSVC AND freetype_SOURCE_DIR)
    get_property(targets DIRECTORY "${freetype_SOURCE_DIR}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS targets)
        set_property(TARGET ${target} PROPERTY MSVC_DEBUG_INFORMATION_FORMAT "Embedded")
    endforeach()
endif()

# HarfBuzz for OpenType shaping and ligatures.
FetchContent_Declare(
    harfbuzz
    GIT_REPOSITORY https://github.com/harfbuzz/harfbuzz.git
    GIT_TAG        14.1.0
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    CMAKE_CACHE_ARGS
        -DHB_BUILD_UTILS:BOOL=OFF
        -DHB_BUILD_TESTS:BOOL=OFF
        -DHB_BUILD_DOCS:BOOL=OFF
        -DHB_BUILD_SUBSET:BOOL=OFF
        -DHB_HAVE_FREETYPE:BOOL=ON
        -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        -DCMAKE_CXX_FLAGS:STRING="/wd4702"  # Suppress unreachable code warning
)
FetchContent_MakeAvailable(harfbuzz)
set_target_properties(harfbuzz PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Intel Embree CPU Ray Tracing Kernels (temporarily disabled)
# FetchContent_Declare(
#     embree
#     GIT_REPOSITORY https://github.com/embree/embree.git
#     GIT_TAG        v4.3.3
#     DOWNLOAD_EXTRACT_TIMESTAMP TRUE
# )
# set(EMBREE_TUTORIALS OFF CACHE BOOL "Disable Embree tutorials" FORCE)
# set(EMBREE_BACKEND_ISPC OFF CACHE BOOL "Disable Embree ISPC backend" FORCE)
# set(EMBREE_ISPC_SUPPORT OFF CACHE BOOL "Disable ISPC support" FORCE)
# set(EMBREE_STATIC_LIB ON CACHE BOOL "Build Embree as static library" FORCE)
# set(EMBREE_RAY_MASK ON CACHE BOOL "Enable Embree ray masking" FORCE)
# set(EMBREE_FILTER_FUNCTION ON CACHE BOOL "Enable Embree filter functions" FORCE)
# set(EMBREE_TASKING_SYSTEM INTERNAL CACHE STRING "Use internal tasking system for Embree" FORCE)
# # Build only for the host architecture
# set(EMBREE_MAX_ISA "SSE4.2" CACHE STRING "Max ISA for Embree" FORCE) 
# tinygltf for glTF mesh loading
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG        v2.9.3
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(tinygltf)
include_directories(${tinygltf_SOURCE_DIR})

# OpenImageDenoise (OIDN) — prebuilt binaries (avoids ISPC + TBB build requirements)
# Only enabled on Windows; Linux uses the stub path
if(WIN32)
    FetchContent_Declare(
        oidn
        URL https://github.com/RenderKit/oidn/releases/download/v2.3.0/oidn-2.3.0.x64.windows.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(oidn)
    set(OIDN_ROOT "${oidn_SOURCE_DIR}")
    find_package(OpenImageDenoise REQUIRED PATHS "${OIDN_ROOT}" NO_DEFAULT_PATH)
else()
    message(STATUS "[Tachyon] OIDN disabled on non-Windows platform")
endif()

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

# Clipper2 for boolean shape operations (Merge Paths)
FetchContent_Declare(
    clipper2
    GIT_REPOSITORY https://github.com/AngusJohnson/Clipper2.git
    GIT_TAG        Clipper2_1.3.0
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(CLIPPER2_UTILS OFF CACHE BOOL "Disable Clipper2 utils" FORCE)
set(CLIPPER2_EXAMPLES OFF CACHE BOOL "Disable Clipper2 examples" FORCE)
set(CLIPPER2_TESTS OFF CACHE BOOL "Disable Clipper2 tests" FORCE)
FetchContent_MakeAvailable(clipper2)
include_directories(${clipper2_SOURCE_DIR}/CPP/Clipper2Lib/include)

# GoogleTest for unit testing
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# For Windows: prevent gtest from overriding our compiler/linker options
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include_directories(${gtest_SOURCE_DIR}/googletest/include)

FetchContent_Declare(
  pugixml
  URL https://github.com/zeux/pugixml/releases/download/v1.14/pugixml-1.14.tar.gz
  URL_HASH SHA256=2f10e276870c64b1db6809050a75e11a897a8d7456c4be5c6b2e35a11168a015
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(pugixml)
include_directories(${pugixml_SOURCE_DIR})

# spdlog — structured logging (header-only)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(SPDLOG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spdlog)

# Once downloaded, don't check for updates every time (Studio-grade build optimization)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL "Disable FetchContent updates" FORCE)

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
