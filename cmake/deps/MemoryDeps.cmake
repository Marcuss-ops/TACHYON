include_guard(GLOBAL)

if(TACHYON_ENABLE_MIMALLOC)
    if(TACHYON_FETCH_DEPS)
        FetchContent_Declare(
            mimalloc
            GIT_REPOSITORY https://github.com/microsoft/mimalloc.git
            GIT_TAG ${TACHYON_MIMALLOC_GIT_TAG}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )

        set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(MI_BUILD_OBJECT OFF CACHE BOOL "" FORCE)
        set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
        set(MI_BUILD_STATIC ON CACHE BOOL "" FORCE)

        FetchContent_MakeAvailable(mimalloc)
    else()
        find_package(mimalloc CONFIG REQUIRED)
    endif()
endif()

function(tachyon_link_mimalloc target)
    if(TACHYON_ENABLE_MIMALLOC)
        if(TARGET mimalloc-static)
            target_link_libraries(${target} PRIVATE mimalloc-static)
        elseif(TARGET mimalloc)
            target_link_libraries(${target} PRIVATE mimalloc)
        elseif(TARGET mimalloc::mimalloc)
            target_link_libraries(${target} PRIVATE mimalloc::mimalloc)
        endif()

        target_compile_definitions(${target} PRIVATE TACHYON_ENABLE_MIMALLOC)

        if(TACHYON_MIMALLOC_OVERRIDE)
            target_compile_definitions(${target} PRIVATE TACHYON_MIMALLOC_OVERRIDE)
        endif()
    endif()
endfunction()
