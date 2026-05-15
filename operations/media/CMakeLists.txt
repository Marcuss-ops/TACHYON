set(TACHYON_MEDIA_OPS_SOURCES
    src/probe.cpp
    src/probe_cache.cpp
    src/video_concat.cpp
    src/clip_processor.cpp
    src/overlay_merger.cpp
    src/transition_filters.cpp
    src/pipeline_orchestrator.cpp
)

add_library(TachyonMediaOps STATIC ${TACHYON_MEDIA_OPS_SOURCES})
target_include_directories(TachyonMediaOps
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
)
target_link_libraries(TachyonMediaOps PUBLIC TachyonCore TachyonPlatform TachyonRenderer2D pugixml)

if(TACHYON_ENABLE_MEDIA)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(FFMPEG QUIET libavcodec libavformat libavutil libswscale libswresample)
        if(FFMPEG_FOUND)
            target_link_libraries(TachyonMediaOps PUBLIC ${FFMPEG_LIBRARIES})
            target_include_directories(TachyonMediaOps PUBLIC ${FFMPEG_INCLUDE_DIRS})
        endif()
    endif()
    target_compile_definitions(TachyonMediaOps PUBLIC TACHYON_ENABLE_MEDIA)
endif()

set_target_properties(TachyonMediaOps PROPERTIES POSITION_INDEPENDENT_CODE ON)
