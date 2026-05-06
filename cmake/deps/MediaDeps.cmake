include_guard(GLOBAL)

if(TACHYON_ENABLE_MEDIA)
    if(TACHYON_FETCH_DEPS)
        FetchContent_Declare(
            tinygltf
            GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
            GIT_TAG        ${TACHYON_TINYGLTF_GIT_TAG}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(tinygltf)

        FetchContent_Declare(
            pugixml
            GIT_REPOSITORY https://github.com/zeux/pugixml.git
            GIT_TAG        ${TACHYON_PUGIXML_GIT_TAG}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(pugixml)
    else()
        find_package(pugixml REQUIRED)
    endif()
else()
    message(STATUS "[Tachyon] Media feature disabled; skipping tinygltf and pugixml fetches")
endif()

if(TACHYON_ENABLE_MEDIA)
    if(TACHYON_FETCH_DEPS)
        FetchContent_Declare(
            earcut
            GIT_REPOSITORY https://github.com/mapbox/earcut.hpp.git
            GIT_TAG        ${TACHYON_EARCUT_GIT_TAG}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )

        FetchContent_GetProperties(earcut)
        if(NOT earcut_POPULATED)
            FetchContent_Populate(earcut)
        endif()

        set(TACHYON_EARCUT_INCLUDE_DIR "${earcut_SOURCE_DIR}/include")
    endif()
else()
    set(TACHYON_EARCUT_INCLUDE_DIR "")
endif()
