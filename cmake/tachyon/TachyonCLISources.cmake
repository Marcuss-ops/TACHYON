# TachyonCLI core sources (minimal)
set(TachyonCLICoreSources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_thumb.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_render.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_metrics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_validate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_inspect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_preview_frame.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_watch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_scene_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_fetch_fonts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_transition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_doctor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_output_presets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_motion_map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/report.cpp
)

# TachyonCLI catalog-specific sources
set(TachyonCLICatalogSources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/cli/cli_demo.cpp
)
