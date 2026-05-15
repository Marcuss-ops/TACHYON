# Tachyon Content Module

set(TACHYON_CONTENT_SOURCES
    preset_catalog.cpp
)

add_library(TachyonContent STATIC ${TACHYON_CONTENT_SOURCES})
tachyon_configure_common(TachyonContent)
target_link_libraries(TachyonContent PUBLIC TachyonCore)
