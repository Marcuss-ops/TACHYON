# TachyonCLI sources
set(TachyonCLISources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_render.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_validate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_inspect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_preview_frame.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_watch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor/autosave/autosave_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/editor/undo/undo_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_scene_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_fetch_fonts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_transition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_motion_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/report.cpp
)

if(TACHYON_ENABLE_PREVIEW_WINDOW)
    list(APPEND TachyonCLISources
        ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/preview_window.cpp
    )
endif()
