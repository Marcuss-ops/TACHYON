# Tachyon JIT Build Contract Generator
include_guard(GLOBAL)

set(TACHYON_JIT_ABI_VERSION 1 CACHE INTERNAL "Current JIT ABI version")

get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(IS_MULTI_CONFIG)
    set(IS_MULTI_CONFIG_JSON "true")
else()
    set(IS_MULTI_CONFIG_JSON "false")
endif()

# Define JIT link targets that must be visible to plugins
set(JIT_LINK_TARGETS TachyonScene TachyonCore TachyonRuntime)

# Build the JSON arrays using generator expressions where necessary
set(JIT_LINK_LIBRARIES_GENEX "")
foreach(tgt ${JIT_LINK_TARGETS})
    if(TARGET ${tgt})
        if(JIT_LINK_LIBRARIES_GENEX STREQUAL "")
            set(JIT_LINK_LIBRARIES_GENEX "\"$<TARGET_LINKER_FILE:${tgt}>\"")
        else()
            string(APPEND JIT_LINK_LIBRARIES_GENEX ",\n    \"$<TARGET_LINKER_FILE:${tgt}>\"")
        endif()
    endif()
endforeach()

set(JIT_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_BINARY_DIR}/include"
)
string(REPLACE ";" "\",\n    \"" JIT_INCLUDE_DIRS_JSON "${JIT_INCLUDE_DIRS}")

set(JIT_COMPILE_DEFS 
    "TACHYON_USE_DLL"
    "TACHYON_SHARED"
)
if(TACHYON_ENABLE_MEDIA)
    list(APPEND JIT_COMPILE_DEFS "TACHYON_ENABLE_MEDIA")
endif()
if(TACHYON_ENABLE_HIGHWAY)
    list(APPEND JIT_COMPILE_DEFS "TACHYON_ENABLE_HIGHWAY")
endif()
string(REPLACE ";" "\",\n    \"" JIT_COMPILE_DEFS_JSON "${JIT_COMPILE_DEFS}")

# Generate the config file at build-time generation
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/tachyon_jit_config_$<CONFIG>.json"
     CONTENT "{
  \"generator\": \"${CMAKE_GENERATOR}\",
  \"multi_config\": ${IS_MULTI_CONFIG_JSON},
  \"active_config\": \"$<CONFIG>\",
  \"compiler\": \"${CMAKE_CXX_COMPILER}\",
  \"cxx_standard\": \"${CMAKE_CXX_STANDARD}\",
  \"include_dirs\": [
    \"${JIT_INCLUDE_DIRS_JSON}\"
  ],
  \"link_libraries\": [
    ${JIT_LINK_LIBRARIES_GENEX}
  ],
  \"compile_definitions\": [
    \"${JIT_COMPILE_DEFS_JSON}\"
  ],
  \"vcvars64_bat\": \"${TACHYON_VCVARS64_BAT}\",
  \"platform\": \"${CMAKE_SYSTEM_NAME}\",
  \"module_suffix\": \"${CMAKE_SHARED_MODULE_SUFFIX}\",
  \"abi_version\": ${TACHYON_JIT_ABI_VERSION}
}
")

message(STATUS "[Tachyon] JIT build contract generator initialized: ${CMAKE_BINARY_DIR}/tachyon_jit_config.json")

