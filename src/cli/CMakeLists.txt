# Tachyon CLI Module
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonCLISources.cmake)

# Source assertions
tachyon_assert_no_duplicate_sources(TachyonCLICoreSources)
tachyon_assert_no_duplicate_sources(TachyonCLIToolSources)

# Core CLI is built as a reusable static library.
tachyon_add_module(
    NAME CLICore
    SOURCES ${TachyonCLICoreSources}
    PUBLIC_DEPS TachyonRuntimeEngine TachyonLibrary TachyonDiagnostics
)

# Optional/demo tools are built as auxiliary static library.
tachyon_add_module(
    NAME CLITools
    SOURCES ${TachyonCLIToolSources}
    PUBLIC_DEPS TachyonCLICore
)

# Interface target for easier linking
tachyon_deprecate_target(TachyonTools TachyonCLITools)
