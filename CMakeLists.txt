cmake_minimum_required(VERSION 3.15)
project(TACHYON VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Determine platform and compiler details
if(MSVC)
    add_compile_options(/W4 /WX /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Subdirectories
add_subdirectory(src)
add_subdirectory(tests)
# add_subdirectory(examples)

# Version header generation
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/tachyon/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/tachyon/version.h
)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
