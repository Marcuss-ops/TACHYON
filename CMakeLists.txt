cmake_minimum_required(VERSION 3.15)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
endif()
project(TACHYON VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)
include(CTest)
include(${CMAKE_SOURCE_DIR}/cmake/options/TachyonOptions.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/deps/DependencyVersions.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/platform/Windows.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/platform/Linux.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(TACHYON_HAS_OPENMP OFF)
if(TACHYON_ENABLE_OPENMP)
    find_package(OpenMP QUIET)
    if(OpenMP_CXX_FOUND)
        set(TACHYON_HAS_OPENMP ON)
    else()
        message(STATUS "[Tachyon] OpenMP not found; building without OpenMP")
    endif()
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

# Dependency groups
include(${CMAKE_SOURCE_DIR}/cmake/deps/TextDeps.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/deps/Renderer3DDeps.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/deps/MediaDeps.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/deps/OutputDeps.cmake)
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
message(STATUS "    - Audio Mux:      ${TACHYON_ENABLE_AUDIO_MUX}")
message(STATUS "    - Output:         ${TACHYON_ENABLE_OUTPUT}")
message(STATUS "    - Tracker:        ${TACHYON_ENABLE_TRACKER}")
message(STATUS "    - Catalog Tools:  ${TACHYON_BUILD_CATALOG_TOOLS}")
message(STATUS "    - Tests:          ${TACHYON_BUILD_TESTS}")
message(STATUS "-------------------------------------------------------")
