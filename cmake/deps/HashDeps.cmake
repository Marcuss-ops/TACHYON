include_guard(GLOBAL)

if(TACHYON_FETCH_DEPS)
    FetchContent_Declare(
        xxhash
        GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
        GIT_TAG        ${TACHYON_XXHASH_GIT_TAG}
    )
    # xxHash has its own CMakeLists; let it populate so we get the source dir.
    FetchContent_MakeAvailable(xxhash)
else()
    # system xxhash: just need the header
    find_path(XXHASH_INCLUDE_DIR xxhash.h REQUIRED)
    if(NOT xxhash_SOURCE_DIR)
        set(xxhash_SOURCE_DIR "${XXHASH_INCLUDE_DIR}")
    endif()
endif()

# INTERFACE target: header-only, fully inlined via XXH_INLINE_ALL.
if(NOT TARGET tachyon_xxhash)
    add_library(tachyon_xxhash INTERFACE)
    target_include_directories(tachyon_xxhash INTERFACE "${xxhash_SOURCE_DIR}")
    target_compile_definitions(tachyon_xxhash INTERFACE XXH_INLINE_ALL)
endif()

function(tachyon_link_hash target)
    target_link_libraries(${target} PUBLIC tachyon_xxhash)
endfunction()
