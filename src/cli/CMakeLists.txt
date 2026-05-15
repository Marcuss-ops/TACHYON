# Tachyon CLI Module

# TachyonCLI sources
set(TachyonCLISources
    cli.cpp
    cli_app.cpp
    options.cpp
    command_registry.cpp
    command_manifest.cpp
    cli_utils.cpp
    cli_scene_loader.cpp
    cli_render.cpp
    cli_validate.cpp
    cli_inspect.cpp
    cli_preview_frame.cpp
    cli_watch.cpp
    cli_motion_map.cpp
    cli_version.cpp
    
    parsing/parse_helpers.cpp
    parsing/parse_render.cpp
    parsing/parse_inspect.cpp
    parsing/parse_metrics.cpp
    parsing/parse_tool.cpp

    # Secondary tools
    cli_thumb.cpp
    cli_metrics.cpp
    cli_fetch_fonts.cpp
    cli_doctor.cpp
    cli_output_presets.cpp
    cli_probe.cpp
    cli_concat.cpp
)

# Source assertions
tachyon_assert_no_duplicate_sources(TachyonCLISources)

# Unified CLI library
tachyon_add_module(
    NAME CLI
    SOURCES ${TachyonCLISources}
    PUBLIC_DEPS TachyonRuntime TachyonCatalog TachyonDiagnostics TachyonOperations
)
