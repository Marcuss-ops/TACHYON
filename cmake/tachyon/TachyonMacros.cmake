# Tachyon Build System Macros

# --- Utility Functions ---

function(tachyon_assert_no_duplicate_sources var_name)
    set(_list "${${var_name}}")
    set(_seen)
    set(_duplicates)
    foreach(_source IN LISTS _list)
        list(FIND _seen "${_source}" _seen_index)
        if(_seen_index EQUAL -1)
            list(APPEND _seen "${_source}")
        else()
            list(FIND _duplicates "${_source}" _duplicate_index)
            if(_duplicate_index EQUAL -1)
                list(APPEND _duplicates "${_source}")
            endif()
        endif()
    endforeach()

    list(LENGTH _duplicates _duplicate_count)
    if(_duplicate_count GREATER 0)
        string(REPLACE ";" "\n  " _duplicate_lines "${_duplicates}")
        message(FATAL_ERROR "Duplicate sources found in ${var_name}:\n  ${_duplicate_lines}")
    endif()
endfunction()

function(tachyon_configure_common target)
    target_include_directories(${target}
        PRIVATE
            "${CMAKE_SOURCE_DIR}/src"
        PUBLIC
            "${CMAKE_SOURCE_DIR}/include"
            "${CMAKE_BINARY_DIR}/include"
            "${CMAKE_SOURCE_DIR}/third_party"
            "${CMAKE_SOURCE_DIR}/third_party/stb/include"
    )
    
    if(absl_SOURCE_DIR)
        target_include_directories(${target} PUBLIC ${absl_SOURCE_DIR})
    endif()

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8 /FS)
        if(TACHYON_TREAT_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
        if(TACHYON_TREAT_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()

    if(TACHYON_ENABLE_MEDIA)
        target_compile_definitions(${target} PUBLIC TACHYON_ENABLE_MEDIA)
    endif()

    set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)

endfunction()

function(tachyon_link_infra target)
    tachyon_link_spdlog(${target})
    tachyon_link_hash(${target})
    tachyon_link_zstd(${target})
endfunction()

function(tachyon_link_absl target)
    if(TARGET absl::strings)
        target_link_libraries(${target} PUBLIC 
            absl::flat_hash_map
            absl::strings
            absl::str_format
        )
    else()
        target_link_libraries(${target} PUBLIC 
            absl_flat_hash_map
            absl_strings
            absl_str_format
        )
    endif()
    if(absl_SOURCE_DIR)
        target_include_directories(${target} PUBLIC ${absl_SOURCE_DIR})
    endif()
endfunction()

# --- Macros ---

# Standardizes the creation of a Tachyon static module.
# 
# Usage:
# tachyon_add_module(
#     NAME Core
#     SOURCES ${TachyonCoreSources}
#     PUBLIC_DEPS absl::strings
#     PRIVATE_DEPS TachyonPlatform
# )
macro(tachyon_add_module)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES PUBLIC_DEPS PRIVATE_DEPS DEFINITIONS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(target_name "Tachyon${ARG_NAME}")
    
    add_library(${target_name} STATIC ${ARG_SOURCES})
    tachyon_configure_common(${target_name})
    
    if(ARG_PUBLIC_DEPS)
        target_link_libraries(${target_name} PUBLIC ${ARG_PUBLIC_DEPS})
    endif()
    
    if(ARG_PRIVATE_DEPS)
        target_link_libraries(${target_name} PRIVATE ${ARG_PRIVATE_DEPS})
    endif()

    if(ARG_DEFINITIONS)
        target_compile_definitions(${target_name} PRIVATE ${ARG_DEFINITIONS})
    endif()
endmacro()

# Marks a target as a deprecated compatibility interface.
macro(tachyon_deprecate_target legacy_name new_name)
    if(NOT TARGET ${legacy_name})
        add_library(${legacy_name} INTERFACE)
        target_link_libraries(${legacy_name} INTERFACE ${new_name})
    endif()
endmacro()
