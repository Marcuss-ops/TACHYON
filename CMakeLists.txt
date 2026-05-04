cmake_minimum_required(VERSION 3.15)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
endif()
project(TACHYON VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)
include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TACHYON_ENABLE_COMPILER_CACHE "Enable sccache/clcache when available" ON)
option(TACHYON_ENABLE_OPENMP "Enable OpenMP parallelism when available" ON)
set(TACHYON_HAS_OPENMP OFF)
if(TACHYON_ENABLE_OPENMP)
    find_package(OpenMP QUIET)
    if(OpenMP_CXX_FOUND)
        set(TACHYON_HAS_OPENMP ON)
    else()
        message(STATUS "[Tachyon] OpenMP not found; building without OpenMP")
    endif()
endif()
option(TACHYON_ENABLE_PCH "Enable precompiled headers when supported" ON)
option(TACHYON_ENABLE_ASAN "Enable AddressSanitizer on Tachyon targets" OFF)
option(TACHYON_TREAT_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(TACHYON_ENABLE_TEXT "Enable text shaping and font rendering" ON)
option(TACHYON_ENABLE_3D "Enable 3D rendering and Embree integration" ON)
option(TACHYON_ENABLE_MEDIA "Enable media import and decoding" ON)
option(TACHYON_ENABLE_OUTPUT "Enable output encoders and sinks" ON)
option(TACHYON_ENABLE_PREVIEW_WINDOW "Enable GLFW/OpenGL preview window" OFF)
option(TACHYON_ENABLE_PRORES "Enable ProRes output sink" OFF)
option(TACHYON_ENABLE_VULKAN "Enable Vulkan backend" OFF)
option(TACHYON_ENABLE_OIDN "Enable OpenImageDenoise integration" OFF)
option(TACHYON_BUILD_TESTS "Build Tachyon tests" ON)
option(TACHYON_BUILD_EXAMPLES "Build Tachyon examples" OFF)
option(TACHYON_ENABLE_TRACKER "Enable tracker module" ON)
option(TACHYON_ENABLE_AUDIO_MUX "Enable audio muxing to MP4 (enabled by default, requires ffmpeg at runtime)" ON)
if(MSVC)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")
    add_compile_options(/FS)
endif()

if(TACHYON_ENABLE_ASAN)
    if(MSVC)
        add_compile_options(/fsanitize=address)
        add_link_options(/fsanitize=address)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        add_compile_options(-fsanitize=address)
        add_link_options(-fsanitize=address)
    endif()
endif()

# Determine platform and compiler details
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DNOMINMAX)
    add_compile_options(/wd4324) # structure was padded due to alignment specifier (Embree)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(TACHYON_TREAT_WARNINGS_AS_ERRORS)
    if(MSVC)
        add_compile_options(/WX)
    else()
        add_compile_options(-Werror)
    endif()
endif()

if(TACHYON_ENABLE_COMPILER_CACHE)
    find_program(TACHYON_COMPILER_LAUNCHER NAMES sccache ccache clcache)
    if(TACHYON_COMPILER_LAUNCHER)
        message(STATUS "Using compiler launcher: ${TACHYON_COMPILER_LAUNCHER}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${TACHYON_COMPILER_LAUNCHER}" CACHE FILEPATH "Compiler launcher" FORCE)
        set(CMAKE_C_COMPILER_LAUNCHER "${TACHYON_COMPILER_LAUNCHER}" CACHE FILEPATH "Compiler launcher" FORCE)
    endif()
endif()

# Include directories are now managed per-target via target_include_directories()
# See src/CMakeLists.txt for TachyonCore includes
# See tests/CMakeLists.txt for TachyonTests includes

# FreeType/HarfBuzz for scalable font rendering and shaping.
if(TACHYON_ENABLE_TEXT)
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
else()
    message(STATUS "[Tachyon] Text feature disabled; skipping FreeType and HarfBuzz fetches")
endif()

# Intel Embree CPU Ray Tracing Kernels
if(TACHYON_ENABLE_3D)
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
    FetchContent_MakeAvailable(embree)
else()
    message(STATUS "[Tachyon] 3D feature disabled; skipping Embree fetch")
endif()

# tinygltf + pugixml for media loading and SVG parsing
if(TACHYON_ENABLE_MEDIA)
    FetchContent_Declare(
        tinygltf
        GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
        GIT_TAG        v2.9.3
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(tinygltf)

    FetchContent_Declare(
        pugixml
        GIT_REPOSITORY https://github.com/zeux/pugixml.git
        GIT_TAG        v1.14
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(pugixml)
else()
    message(STATUS "[Tachyon] Media feature disabled; skipping tinygltf and pugixml fetches")
endif()

# OpenImageDenoise (OIDN) — prebuilt binaries (avoids ISPC + TBB build requirements)
# Only enabled when TACHYON_ENABLE_OIDN is ON; mainly for Windows
if(TACHYON_ENABLE_OIDN AND WIN32)
    FetchContent_Declare(
        oidn
        URL https://github.com/RenderKit/oidn/releases/download/v2.3.0/oidn-2.3.0.x64.windows.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(oidn)
    set(OIDN_ROOT "${oidn_SOURCE_DIR}")
    find_package(OpenImageDenoise REQUIRED PATHS "${OIDN_ROOT}" NO_DEFAULT_PATH)
else()
    message(STATUS "[Tachyon] OIDN disabled (not requested or not on Windows)")
endif()

# earcut.hpp for 3D polygon triangulation (Extrusion)
if(TACHYON_ENABLE_MEDIA)
    FetchContent_Declare(
        earcut
        GIT_REPOSITORY https://github.com/mapbox/earcut.hpp.git
        GIT_TAG        v2.2.4
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_GetProperties(earcut)
    if(NOT earcut_POPULATED)
        FetchContent_Populate(earcut)
    endif()

    set(TACHYON_EARCUT_INCLUDE_DIR "${earcut_SOURCE_DIR}/include")
else()
    set(TACHYON_EARCUT_INCLUDE_DIR "")
endif()

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

# tinyexr for EXR image output
if(TACHYON_ENABLE_OUTPUT)
    FetchContent_Declare(
        tinyexr
        GIT_REPOSITORY https://github.com/syoyo/tinyexr.git
        GIT_TAG        v1.0.8
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(tinyexr)
else()
    message(STATUS "[Tachyon] Output feature disabled; skipping tinyexr fetch")
endif()
# Version header generation
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/tachyon/core/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/tachyon/version.h
)

# Build config generation for C++ loader
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/tachyon/tachyon_build_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/tachyon/tachyon_build_config.h
)

# Binary dir include directory is added to TachyonCore target in src/CMakeLists.txt

# Subdirectories
add_subdirectory(src)

if(TACHYON_BUILD_TESTS)
    add_subdirectory(tests)
endif()

if(TACHYON_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
