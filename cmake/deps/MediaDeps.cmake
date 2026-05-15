include_guard(GLOBAL)

if(TACHYON_ENABLE_MEDIA)
    if(TACHYON_FETCH_DEPS)


        if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/pugixml/CMakeLists.txt")
            add_subdirectory("${CMAKE_SOURCE_DIR}/third_party/pugixml" "${CMAKE_BINARY_DIR}/_deps/pugixml-build" EXCLUDE_FROM_ALL)
        else()
            FetchContent_Declare(
                pugixml
                GIT_REPOSITORY https://github.com/zeux/pugixml.git
                GIT_TAG        ${TACHYON_PUGIXML_GIT_TAG}
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            )
            FetchContent_MakeAvailable(pugixml)
        endif()
    else()
        find_package(pugixml REQUIRED)
    endif()
else()
    message(STATUS "[Tachyon] Media feature disabled; skipping pugixml fetches")
endif()

if(TACHYON_ENABLE_MEDIA)
    if(TACHYON_FETCH_DEPS)
        if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/earcut/include")
            set(TACHYON_EARCUT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/earcut/include")
        else()
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
    endif()
else()
    set(TACHYON_EARCUT_INCLUDE_DIR "")
endif()

function(tachyon_link_media_deps target)
    if(TACHYON_ENABLE_MEDIA)
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(FFMPEG QUIET IMPORTED_TARGET libavcodec libavformat libavutil libswscale libswresample)
            if(FFMPEG_FOUND)
                target_link_libraries(${target} PUBLIC PkgConfig::FFMPEG)
                target_compile_definitions(${target} PUBLIC TACHYON_ENABLE_MEDIA)
            else()
                message(FATAL_ERROR "[Tachyon] FFmpeg dev libraries (libavcodec, etc.) not found via pkg-config. Please install them or set TACHYON_ENABLE_MEDIA=OFF.")
            endif()
        else()
            if(WIN32)
                message(WARNING "[Tachyon] PkgConfig not found. On Windows, please ensure pkg-config is in PATH or provide FFmpeg paths manually.")
            endif()
            message(FATAL_ERROR "[Tachyon] PkgConfig required for FFmpeg discovery. Please install pkg-config.")
        endif()
    endif()
endfunction()
