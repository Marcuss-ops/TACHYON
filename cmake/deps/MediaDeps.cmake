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
    if(NOT TACHYON_ENABLE_MEDIA)
        return()
    endif()

    if(NOT TARGET Tachyon::FFmpeg)
        add_library(Tachyon::FFmpeg INTERFACE IMPORTED)
        set(_ffmpeg_found FALSE)

        # Level 1: Explicit Root
        if(TACHYON_FFMPEG_ROOT OR (TACHYON_FFMPEG_INCLUDE_DIR AND TACHYON_FFMPEG_LIBRARY_DIR))
            message(STATUS "[Tachyon] FFmpeg: Using explicit paths")
            if(TACHYON_FFMPEG_ROOT)
                set(TACHYON_FFMPEG_INCLUDE_DIR "${TACHYON_FFMPEG_ROOT}/include" CACHE PATH "" FORCE)
                set(TACHYON_FFMPEG_LIBRARY_DIR "${TACHYON_FFMPEG_ROOT}/lib" CACHE PATH "" FORCE)
            endif()

            target_include_directories(Tachyon::FFmpeg INTERFACE ${TACHYON_FFMPEG_INCLUDE_DIR})
            
            set(_libs avcodec avformat avutil swscale swresample)
            foreach(_lib ${_libs})
                find_library(FFMPEG_${_lib}_LIB NAMES ${_lib} PATHS ${TACHYON_FFMPEG_LIBRARY_DIR} NO_DEFAULT_PATH)
                if(FFMPEG_${_lib}_LIB)
                    target_link_libraries(Tachyon::FFmpeg INTERFACE ${FFMPEG_${_lib}_LIB})
                else()
                    message(FATAL_ERROR "[Tachyon] FFmpeg: Could not find ${_lib} in ${TACHYON_FFMPEG_LIBRARY_DIR}")
                endif()
            endforeach()
            set(_ffmpeg_found TRUE)
            set(TACHYON_FFMPEG_MODE "explicit" CACHE INTERNAL "")
        endif()

        # Level 2: PkgConfig
        if(NOT _ffmpeg_found)
            find_package(PkgConfig QUIET)
            if(PKG_CONFIG_FOUND)
                pkg_check_modules(FFMPEG QUIET IMPORTED_TARGET libavcodec libavformat libavutil libswscale libswresample)
                if(FFMPEG_FOUND)
                    message(STATUS "[Tachyon] FFmpeg: Found via PkgConfig")
                    target_link_libraries(Tachyon::FFmpeg INTERFACE PkgConfig::FFMPEG)
                    set(_ffmpeg_found TRUE)
                    set(TACHYON_FFMPEG_MODE "pkg-config" CACHE INTERNAL "")
                endif()
            endif()
        endif()

        # Level 3: Config / vcpkg
        if(NOT _ffmpeg_found)
            find_package(FFMPEG QUIET)
            if(FFMPEG_FOUND)
                message(STATUS "[Tachyon] FFmpeg: Found via find_package")
                target_link_libraries(Tachyon::FFmpeg INTERFACE FFMPEG::FFMPEG)
                set(_ffmpeg_found TRUE)
                set(TACHYON_FFMPEG_MODE "config" CACHE INTERNAL "")
            endif()
        endif()

        if(NOT _ffmpeg_found)
            message(FATAL_ERROR "[Tachyon] FFmpeg discovery failed. Please set TACHYON_FFMPEG_ROOT or install via pkg-config.")
        endif()
    endif()

    target_link_libraries(${target} PRIVATE Tachyon::FFmpeg)
    target_compile_definitions(${target} PRIVATE TACHYON_ENABLE_MEDIA)
endfunction()
