include_guard(GLOBAL)

if(TACHYON_ENABLE_OIDN AND WIN32)
    if(TACHYON_FETCH_DEPS)
        FetchContent_Declare(
            oidn
            URL https://github.com/RenderKit/oidn/releases/download/v${TACHYON_OIDN_WINDOWS_VERSION}/oidn-${TACHYON_OIDN_WINDOWS_VERSION}.x64.windows.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(oidn)
        find_package(OpenImageDenoise REQUIRED PATHS "${oidn_SOURCE_DIR}" NO_DEFAULT_PATH)
    else()
        find_package(OpenImageDenoise REQUIRED)
    endif()
else()
    message(STATUS "[Tachyon] OIDN disabled (not requested or not on Windows)")
endif()

if(TACHYON_ENABLE_OUTPUT)
    if(TACHYON_FETCH_DEPS)
        FetchContent_Declare(
            tinyexr
            GIT_REPOSITORY https://github.com/syoyo/tinyexr.git
            GIT_TAG        ${TACHYON_TINYEXR_GIT_TAG}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(tinyexr)
    endif()
else()
    message(STATUS "[Tachyon] Output feature disabled; skipping tinyexr fetch")
endif()
