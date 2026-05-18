# Tachyon Render Benchmark Tool

add_executable(tachyon-render-benchmark main.cpp)
tachyon_configure_common(tachyon-render-benchmark)

target_link_libraries(tachyon-render-benchmark
    PRIVATE
        TachyonCore
        TachyonOutput
        TachyonPlatform
)
