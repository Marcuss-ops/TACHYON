include_guard(GLOBAL)

if(TACHYON_ENABLE_DRACO)
    if(TACHYON_FETCH_DEPS)
        set(DRACO_TESTS OFF CACHE BOOL "" FORCE)
        set(DRACO_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(DRACO_ANIMATION OFF CACHE BOOL "" FORCE)
        set(DRACO_JS_GLUE OFF CACHE BOOL "" FORCE)

        FetchContent_Declare(
            draco
            GIT_REPOSITORY https://github.com/google/draco.git
            GIT_TAG        ${TACHYON_DRACO_GIT_TAG}
        )
        FetchContent_MakeAvailable(draco)
    else()
        find_package(draco REQUIRED)
    endif()
endif()
