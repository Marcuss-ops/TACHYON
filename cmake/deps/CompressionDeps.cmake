include_guard(GLOBAL)

if(TACHYON_FETCH_DEPS)
    FetchContent_Declare(
        zstd
        GIT_REPOSITORY https://github.com/facebook/zstd.git
        GIT_TAG        ${TACHYON_ZSTD_GIT_TAG}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SOURCE_SUBDIR  build/cmake
    )
    set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_CONTRIB  OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_SHARED   OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_STATIC   ON  CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(zstd)
else()
    find_package(zstd CONFIG REQUIRED)
endif()

function(tachyon_link_zstd target)
    if(TARGET zstd::libzstd_static)
        target_link_libraries(${target} PUBLIC zstd::libzstd_static)
    elseif(TARGET libzstd_static)
        target_link_libraries(${target} PUBLIC libzstd_static)
    elseif(TARGET zstd::zstd)
        target_link_libraries(${target} PUBLIC zstd::zstd)
    endif()
endfunction()
