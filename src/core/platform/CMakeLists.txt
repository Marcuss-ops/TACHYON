# Tachyon Platform Module

set(TachyonPlatformSources
    process.cpp
)

tachyon_add_module(
    NAME Platform
    SOURCES ${TachyonPlatformSources}
)
