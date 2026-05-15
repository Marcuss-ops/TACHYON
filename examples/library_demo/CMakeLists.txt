# Tachyon Library Demo Tool

add_executable(tachyon-library-demo main.cpp)
tachyon_configure_common(tachyon-library-demo)
target_link_libraries(tachyon-library-demo
    PRIVATE
        TachyonCLI
        TachyonCatalog
        TachyonPresets
        TachyonDiagnostics
)
tachyon_link_mimalloc(tachyon-library-demo)
