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