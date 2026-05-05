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

option(TACHYON_FETCH_DEPS "Fetch dependencies via FetchContent" ON)
option(TACHYON_USE_SYSTEM_DEPS "Use system packages instead of FetchContent" OFF)

if(TACHYON_USE_SYSTEM_DEPS)
    set(TACHYON_FETCH_DEPS OFF CACHE BOOL "Sync with TACHYON_USE_SYSTEM_DEPS" FORCE)
endif()
if(NOT TACHYON_FETCH_DEPS)
    set(TACHYON_USE_SYSTEM_DEPS ON CACHE BOOL "Sync with TACHYON_FETCH_DEPS" FORCE)
endif()

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
option(TACHYON_ENABLE_SKIA "Enable Skia-backed text rasterization" OFF)
option(TACHYON_ENABLE_3D "Enable 3D rendering and Embree integration" ON)
option(TACHYON_ENABLE_MEDIA "Enable media import and decoding" ON)
option(TACHYON_ENABLE_OUTPUT "Enable output encoders and sinks" ON)
option(TACHYON_ENABLE_PREVIEW_WINDOW "Enable GLFW/OpenGL preview window" OFF)
option(TACHYON_ENABLE_PRORES "Enable ProRes output sink" OFF)
option(TACHYON_ENABLE_VULKAN "Enable Vulkan backend" OFF)
option(TACHYON_ENABLE_OIDN "Enable OpenImageDenoise integration" OFF)
option(TACHYON_BUILD_TESTS "Build Tachyon tests" ON)
option(TACHYON_BUILD_EXAMPLES "Build Tachyon examples" OFF)
option(TACHYON_ENABLE_TRACKER "Enable tracker module" OFF)
option(TACHYON_ENABLE_AUDIO_MUX "Enable audio muxing to MP4 (enabled by default, requires ffmpeg at runtime)" ON)
if(MSVC)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")
    add_compile_options(/FS)
endif()

if(MSVC AND TACHYON_ENABLE_SKIA)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC runtime library" FORCE)
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

# Skia configuration
add_library(TachyonSkiaBackend INTERFACE)

if(TACHYON_ENABLE_SKIA)
    set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
    set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL Release)
    
    # Try to find Skia via vcpkg
    set(TACHYON_SKIA_VCPKG_PREFIX "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows-static")
    if(EXISTS "${TACHYON_SKIA_VCPKG_PREFIX}/share/unofficial-skia/unofficial-skia-config.cmake")
        list(PREPEND CMAKE_PREFIX_PATH "${TACHYON_SKIA_VCPKG_PREFIX}")
    endif()

    find_package(unofficial-skia CONFIG QUIET)
    
    if(TARGET unofficial::skia::skia)
        target_link_libraries(TachyonSkiaBackend INTERFACE unofficial::skia::skia)
        target_compile_definitions(TachyonSkiaBackend INTERFACE TACHYON_ENABLE_SKIA=1)
        message(STATUS "[Tachyon] Skia enabled for text rasterization")
        set(TACHYON_HAS_SKIA ON)
    else()
        target_compile_definitions(TachyonSkiaBackend INTERFACE TACHYON_ENABLE_SKIA=0)
        message(STATUS "[Tachyon] Skia not found; using software text rasterization")
        set(TACHYON_HAS_SKIA OFF)
    endif()
else()
    target_compile_definitions(TachyonSkiaBackend INTERFACE TACHYON_ENABLE_SKIA=0)
    set(TACHYON_HAS_SKIA OFF)
endif()

# FreeType/HarfBuzz for scalable font rendering and shaping.
set(TACHYON_FREETYPE_TARGET freetype)
set(TACHYON_HARFBUZZ_TARGET harfbuzz)
if(TACHYON_ENABLE_TEXT)
    # Use vcpkg provided dependencies if Skia is enabled (as they are already there)
    if(TACHYON_HAS_SKIA AND EXISTS "${TACHYON_SKIA_VCPKG_PREFIX}/share/freetype/freetype-config.cmake" AND EXISTS "${TACHYON_SKIA_VCPKG_PREFIX}/share/harfbuzz/harfbuzzConfig.cmake")
        list(PREPEND CMAKE_PREFIX_PATH "${TACHYON_SKIA_VCPKG_PREFIX}")
        find_package(ZLIB REQUIRED)
        find_package(freetype CONFIG REQUIRED)
        find_package(harfbuzz CONFIG REQUIRED)
        add_library(TachyonFreeType UNKNOWN IMPORTED)
        get_target_property(_tachyon_freetype_release freetype IMPORTED_LOCATION_RELEASE)
        get_target_property(_tachyon_freetype_debug freetype IMPORTED_LOCATION_DEBUG)
        get_target_property(_tachyon_freetype_include freetype INTERFACE_INCLUDE_DIRECTORIES)
        if(_tachyon_freetype_release)
            set_target_properties(TachyonFreeType PROPERTIES IMPORTED_LOCATION_RELEASE "${_tachyon_freetype_release}")
        endif()
        if(_tachyon_freetype_debug)
            set_target_properties(TachyonFreeType PROPERTIES IMPORTED_LOCATION_DEBUG "${_tachyon_freetype_debug}")
        endif()
        if(_tachyon_freetype_include)
            set_target_properties(TachyonFreeType PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${_tachyon_freetype_include}")
        endif()
        set_target_properties(TachyonFreeType PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "C")

        add_library(TachyonHarfbuzz UNKNOWN IMPORTED)
        get_target_property(_tachyon_harfbuzz_release harfbuzz IMPORTED_LOCATION_RELEASE)
        get_target_property(_tachyon_harfbuzz_debug harfbuzz IMPORTED_LOCATION_DEBUG)
        get_target_property(_tachyon_harfbuzz_include harfbuzz INTERFACE_INCLUDE_DIRECTORIES)
        if(_tachyon_harfbuzz_release)
            set_target_properties(TachyonHarfbuzz PROPERTIES IMPORTED_LOCATION_RELEASE "${_tachyon_harfbuzz_release}")
        endif()
        if(_tachyon_harfbuzz_debug)
            set_target_properties(TachyonHarfbuzz PROPERTIES IMPORTED_LOCATION_DEBUG "${_tachyon_harfbuzz_debug}")
        endif()
        if(_tachyon_harfbuzz_include)
            set_target_properties(TachyonHarfbuzz PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${_tachyon_harfbuzz_include}")
        endif()
        set_target_properties(TachyonHarfbuzz PROPERTIES IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")

        set(TACHYON_FREETYPE_TARGET TachyonFreeType)
        set(TACHYON_HARFBUZZ_TARGET TachyonHarfbuzz)
    elseif(TACHYON_FETCH_DEPS)
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
        set(FT_WITH_BROTLI OFF CACHE BOOL "Disable Brotli for FreeType" FORCE)
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
        find_package(FreeType REQUIRED)
        # HarfBuzz might not have a standard FindHarfBuzz, but we expect it to be available if TACHYON_FETCH_DEPS is OFF
        # For now, we'll just skip the fetch and assume the user provides it or we'll error later if missing.
    endif()
else()
    message(STATUS "[Tachyon] Text feature disabled; skipping FreeType and HarfBuzz fetches")
endif()

# Intel Embree CPU Ray Tracing Kernels
if(TACHYON_ENABLE_3D)
    if(TACHYON_FETCH_DEPS)
        FetchContent_Declare(
            embree
            GIT_REPOSITORY https://github.com/embree/embree.git
            GIT_TAG        v3.13.5
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        set(EMBREE_ISPC_SUPPORT OFF CACHE BOOL "Disable ISPC for Embree" FORCE)
        set(EMBREE_TUTORIALS OFF CACHE BOOL "Disable Embree tutorials" FORCE)
        set(EMBREE_TASKING_SYSTEM INTERNAL CACHE STRING "Use internal tasking for Embree" FORCE)
        FetchContent_MakeAvailable(embree)
    else()
        find_package(embree 3.13 REQUIRED)
    endif()
    set(EMBREE_STATIC_LIB ON CACHE BOOL "Build Embree as static library" FORCE)
    set(EMBREE_RAY_MASK ON CACHE BOOL "Enable Embree ray masking" FORCE)
    set(EMBREE_FILTER_FUNCTION ON CACHE BOOL "Enable Embree filter functions" FORCE)
    set(EMBREE_TASKING_SYSTEM INTERNAL CACHE STRING "Use internal tasking system for Embree" FORCE)
    # Build only for the host architecture
    set(EMBREE_MAX_ISA "SSE4.2" CACHE STRING "Max ISA for Embree" FORCE)
else()
    message(STATUS "[Tachyon] 3D feature disabled; skipping Embree fetch")
endif()

# tinygltf + pugixml for media loading and SVG parsing
if(TACHYON_ENABLE_MEDIA)
    if(TACHYON_FETCH_DEPS)
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
        # For header-only tinygltf, user should provide tinygltf_SOURCE_DIR or similar if needed
        find_package(pugixml REQUIRED)
    endif()
else()
    message(STATUS "[Tachyon] Media feature disabled; skipping tinygltf and pugixml fetches")
endif()

# OpenImageDenoise (OIDN) — prebuilt binaries (avoids ISPC + TBB build requirements)
# Only enabled when TACHYON_ENABLE_OIDN is ON; mainly for Windows
if(TACHYON_ENABLE_OIDN AND WIN32)
    if(TACHYON_FETCH_DEPS)
        FetchContent_Declare(
            oidn
            URL https://github.com/RenderKit/oidn/releases/download/v2.3.0/oidn-2.3.0.x64.windows.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(oidn)
        find_package(OpenImageDenoise REQUIRED PATHS "${oidn_SOURCE_DIR}" NO_DEFAULT_PATH)
    else()
        find_package(OpenImageDenoise REQUIRED)
    endif()
else()
    message(STATUS "[Tachyon] OIDN disabled (not requested or not on Windows)")
endif()

# earcut.hpp for 3D polygon triangulation (Extrusion)
if(TACHYON_ENABLE_MEDIA)
    if(TACHYON_FETCH_DEPS)
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
    endif()
else()
    set(TACHYON_EARCUT_INCLUDE_DIR "")
endif()

# Clipper2 for boolean shape operations (Merge Paths)
if(TACHYON_FETCH_DEPS)
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
else()
    find_package(Clipper2 REQUIRED)
endif()

# tinyexr for EXR image output
if(TACHYON_ENABLE_OUTPUT)
    if(TACHYON_FETCH_DEPS)
        FetchContent_Declare(
            tinyexr
            GIT_REPOSITORY https://github.com/syoyo/tinyexr.git
            GIT_TAG        v1.0.8
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(tinyexr)
    endif()
else()
    message(STATUS "[Tachyon] Output feature disabled; skipping tinyexr fetch")
endif()
# Version header generation
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/tachyon/core/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/tachyon/version.h
)

# Build config generation for C++ loader
set(TACHYON_VCVARS64_BAT "")
if(WIN32)
    set(_tachyon_vs_roots
        "$ENV{ProgramFiles}/Microsoft Visual Studio"
        "C:/Program Files/Microsoft Visual Studio"
        "C:/Program Files (x86)/Microsoft Visual Studio"
    )

    foreach(_tachyon_vs_root IN LISTS _tachyon_vs_roots)
        if(EXISTS "${_tachyon_vs_root}")
            file(GLOB_RECURSE _tachyon_vcvars_candidates
                LIST_DIRECTORIES FALSE
                "${_tachyon_vs_root}/vcvars64.bat"
            )
            if(_tachyon_vcvars_candidates)
                list(GET _tachyon_vcvars_candidates 0 TACHYON_VCVARS64_BAT)
                file(TO_CMAKE_PATH "${TACHYON_VCVARS64_BAT}" TACHYON_VCVARS64_BAT)
                break()
            endif()
        endif()
    endforeach()
endif()

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

# Build configuration summary
message(STATUS "-------------------------------------------------------")
message(STATUS " Tachyon Engine Configuration Summary")
message(STATUS "-------------------------------------------------------")
message(STATUS "  Version:            ${PROJECT_VERSION}")
message(STATUS "  Build Type:         ${CMAKE_BUILD_TYPE}")
message(STATUS "  Fetch Dependencies: ${TACHYON_FETCH_DEPS}")
message(STATUS "  Use System Deps:    ${TACHYON_USE_SYSTEM_DEPS}")
message(STATUS "  Features:")
message(STATUS "    - 3D Engine:      ${TACHYON_ENABLE_3D}")
message(STATUS "    - Media:          ${TACHYON_ENABLE_MEDIA}")
message(STATUS "    - Audio:          ${TACHYON_ENABLE_AUDIO}")
message(STATUS "    - Output:         ${TACHYON_ENABLE_OUTPUT}")
message(STATUS "    - Tests:          ${TACHYON_BUILD_TESTS}")
message(STATUS "-------------------------------------------------------")
