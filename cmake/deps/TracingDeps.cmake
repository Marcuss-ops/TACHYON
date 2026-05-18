include_guard(GLOBAL)

if(TACHYON_ENABLE_PERFETTO)
    if(TACHYON_USE_SYSTEM_DEPS)
        find_package(perfetto CONFIG QUIET)
    endif()

    if(NOT perfetto_FOUND AND NOT TARGET perfetto)
        FetchContent_Declare(
            perfetto
            GIT_REPOSITORY https://github.com/google/perfetto.git
            GIT_TAG ${TACHYON_PERFETTO_GIT_TAG}
        )
        FetchContent_MakeAvailable(perfetto)

        if(NOT TARGET perfetto)
            add_library(perfetto STATIC ${perfetto_SOURCE_DIR}/sdk/perfetto.cc)
            target_include_directories(perfetto PUBLIC ${perfetto_SOURCE_DIR}/sdk)
        endif()
    endif()
endif()

if(TACHYON_ENABLE_TRACY)
    if(TACHYON_USE_SYSTEM_DEPS)
        find_package(Tracy CONFIG QUIET)
    endif()

    if(NOT Tracy_FOUND AND NOT TARGET tracy)
        FetchContent_Declare(
            tracy
            GIT_REPOSITORY https://github.com/wolfpld/tracy.git
            GIT_TAG        ${TACHYON_TRACY_GIT_TAG}
        )
        
        # Tracy options
        set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
        set(TRACY_ON_DEMAND ON CACHE BOOL "" FORCE)
        
        FetchContent_MakeAvailable(tracy)
    endif()
endif()