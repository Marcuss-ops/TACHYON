include_guard(GLOBAL)

if(TACHYON_FETCH_DEPS)
    # Clipper2 for boolean shape operations (Merge Paths)
    FetchContent_Declare(
        clipper2
        GIT_REPOSITORY https://github.com/AngusJohnson/Clipper2.git
        GIT_TAG        ${TACHYON_CLIPPER2_GIT_TAG}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(CLIPPER2_UTILS OFF CACHE BOOL "Disable Clipper2 utils" FORCE)
    set(CLIPPER2_EXAMPLES OFF CACHE BOOL "Disable Clipper2 examples" FORCE)
    set(CLIPPER2_TESTS OFF CACHE BOOL "Disable Clipper2 tests" FORCE)
    FetchContent_MakeAvailable(clipper2)

    # GoogleTest for unit testing
    FetchContent_Declare(
        googletest
        URL ${TACHYON_GTEST_URL}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    # For Windows: prevent gtest from overriding our compiler/linker options
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()
