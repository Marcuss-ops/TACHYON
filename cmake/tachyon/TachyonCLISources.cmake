# TachyonCLI core sources (essential)
set(TachyonCLICoreSources
    ${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_app.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/options.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/command_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_scene_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_render.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_validate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_inspect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_preview_frame.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_watch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_motion_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_version.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/parsing/parse_helpers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parsing/parse_render.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parsing/parse_inspect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parsing/parse_metrics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parsing/parse_tool.cpp
)

# TachyonCLI secondary tools (auxiliary)
set(TachyonCLIToolSources
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_thumb.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_metrics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_fetch_fonts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_doctor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_output_presets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_probe.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli_concat.cpp
)
