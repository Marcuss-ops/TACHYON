# Tachyon JIT Build Contract Generator
include_guard(GLOBAL)

set(TACHYON_JIT_ABI_VERSION 1 CACHE INTERNAL "Current JIT ABI version")

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/tachyon_jit_config.json.in"
    "${CMAKE_BINARY_DIR}/tachyon_jit_config.json"
    @ONLY
)

message(STATUS "[Tachyon] JIT build contract generated: ${CMAKE_BINARY_DIR}/tachyon_jit_config.json")
