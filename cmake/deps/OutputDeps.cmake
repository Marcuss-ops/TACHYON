# Tachyon Output Dependencies (Clipper2, tinyexr)

if(TACHYON_FETCH_DEPS)
    FetchContent_Declare(
        clipper2
        GIT_REPOSITORY https://github.com/AngusJohnson/Clipper2.git
        GIT_TAG        Clipper2_1.3.0
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(CLIPPER2_UTILS OFF CACHE BOOL "Disable Clipper2 utils" FORCE)
    set(CLIPPER2_EXAMPLES OFF CACHE BOOL "Disable Clipper2 examples" FORCE)
    set(CLIPPER2_TESTS OFF CACHE BOOL "Disable Clipper2 tests" FORCE)
    FetchContent_MakeAvailable(clipper2)
else()
    find_package(Clipper2 REQUIRED)
endif()

if(TACHYON_ENABLE_OUTPUT)
    if(TACHYON_FETCH_DEPS)
        FetchContent_Declare(
            tinyexr
            GIT_REPOSITORY https://github.com/syoyo/tinyexr.git
            GIT_TAG        v1.0.8
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(tinyexr)
    endif()
else()
    message(STATUS "[Tachyon] Output feature disabled; skipping tinyexr fetch")
endif()
