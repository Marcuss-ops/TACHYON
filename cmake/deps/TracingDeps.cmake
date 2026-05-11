include_guard(GLOBAL)

if(TACHYON_ENABLE_PERFETTO)
    if(TACHYON_FETCH_DEPS)
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
    else()
        find_package(perfetto CONFIG REQUIRED)
    endif()
endif()

if(TACHYON_ENABLE_TRACY)
    if(TACHYON_FETCH_DEPS)
        FetchContent_Declare(
            tracy
            GIT_REPOSITORY https://github.com/wolfpld/tracy.git
            GIT_TAG        v0.11.1
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        
        # Tracy options
        set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
        set(TRACY_ON_DEMAND ON CACHE BOOL "" FORCE)
        
        FetchContent_MakeAvailable(tracy)
    else()
        find_package(Tracy CONFIG REQUIRED)
    endif()
endif()