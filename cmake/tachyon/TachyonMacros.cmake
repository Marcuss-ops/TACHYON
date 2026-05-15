# Tachyon Build System Macros

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
