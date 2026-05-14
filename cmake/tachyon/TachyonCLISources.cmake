# TachyonCLI core sources (minimal render-focused CLI)
set(TachyonCLICoreSources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_render.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_thumb.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_validate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_inspect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_preview_frame.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_watch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_scene_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_output_presets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_motion_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/command_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli.cpp
)
