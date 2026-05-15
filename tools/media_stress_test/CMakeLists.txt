# Tachyon Media Stress Test Tool

add_executable(tachyon-media-stress main.cpp)
tachyon_configure_common(tachyon-media-stress)

target_link_libraries(tachyon-media-stress
    PRIVATE
        TachyonCore
        TachyonMediaOps
        TachyonPlatform
)
