# Tachyon CLI Module
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonCLISources.cmake)

# Source assertions
tachyon_assert_no_duplicate_sources(TachyonCLICoreSources)
tachyon_assert_no_duplicate_sources(TachyonCLIToolSources)

# Core CLI is built as a reusable static library.
add_library(TachyonCLICore STATIC ${TachyonCLICoreSources})
tachyon_configure_common(TachyonCLICore)
target_link_libraries(TachyonCLICore PUBLIC TachyonRuntimeEngine TachyonLibrary TachyonDiagnostics)

# Optional/demo tools are built as auxiliary static library.
add_library(TachyonCLITools STATIC ${TachyonCLIToolSources})
tachyon_configure_common(TachyonCLITools)
target_link_libraries(TachyonCLITools PUBLIC TachyonCLICore)

# Interface target for easier linking
add_library(TachyonTools INTERFACE)
target_link_libraries(TachyonTools INTERFACE
    TachyonRuntimeEngine
    TachyonPresets
    TachyonDiagnostics
    TachyonCLICore
    TachyonCLITools
)
