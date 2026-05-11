include_guard(GLOBAL)

if(TACHYON_ENABLE_HIGHWAY)
    if(TACHYON_FETCH_DEPS)
        FetchContent_Declare(
            highway
            GIT_REPOSITORY https://github.com/google/highway.git
            GIT_TAG        ${TACHYON_HIGHWAY_GIT_TAG}
        )

        set(HWY_ENABLE_EXAMPLES OFF CACHE BOOL "Disable Highway examples" FORCE)
        set(HWY_ENABLE_TESTS OFF CACHE BOOL "Disable Highway tests" FORCE)
        FetchContent_MakeAvailable(highway)
    else()
        find_package(hwy CONFIG REQUIRED)
    endif()
endif()