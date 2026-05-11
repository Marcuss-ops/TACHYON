include_guard(GLOBAL)

if(TACHYON_ENABLE_SIMDJSON)
    if(TACHYON_FETCH_DEPS)
        FetchContent_Declare(
            simdjson
            GIT_REPOSITORY https://github.com/simdjson/simdjson.git
            GIT_TAG ${TACHYON_SIMDJSON_GIT_TAG}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(simdjson)
    else()
        find_package(simdjson CONFIG REQUIRED)
    endif()
endif()
