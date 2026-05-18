include_guard(GLOBAL)

if(TACHYON_FETCH_DEPS)
    FetchContent_Declare(
        crc32c
        GIT_REPOSITORY https://github.com/google/crc32c.git
        GIT_TAG        ${TACHYON_CRC32C_GIT_TAG}
    )
    
    set(CRC32C_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(CRC32C_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
    set(CRC32C_USE_GLOG OFF CACHE BOOL "" FORCE)
    set(CRC32C_INSTALL OFF CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(crc32c)
endif()
