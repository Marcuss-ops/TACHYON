# Tachyon CLI Module

# TachyonCLI sources
set(TachyonCLISources
    app/cli.cpp
    app/cli_app.cpp
    app/options.cpp
    
    commands/command_registry.cpp
    commands/command_manifest.cpp
    commands/render.cpp
    commands/validate.cpp
    commands/inspect.cpp
    commands/preview_frame.cpp
    commands/watch.cpp
    commands/motion_map.cpp
    commands/version.cpp
    commands/thumb.cpp
    commands/metrics.cpp
)

if(TACHYON_ENABLE_TEXT)
    list(APPEND TachyonCLISources commands/fetch_fonts.cpp)
endif()

list(APPEND TachyonCLISources
    commands/doctor.cpp
    commands/output_presets.cpp
    commands/probe.cpp
    commands/concat.cpp

    support/utils.cpp
    support/scene_loader.cpp
    
    parsing/parse_helpers.cpp
    parsing/parse_render.cpp
    parsing/parse_inspect.cpp
    parsing/parse_metrics.cpp
    parsing/parse_tool.cpp
)

# Source assertions
tachyon_assert_no_duplicate_sources(TachyonCLISources)

# Unified CLI library
tachyon_add_module(
    NAME CLI
    SOURCES ${TachyonCLISources}
    PUBLIC_DEPS TachyonRuntime TachyonCatalog TachyonDiagnostics TachyonOperations
)

target_include_directories(TachyonCLI PRIVATE 
    .
    app
    commands
    parsing
    support
)
