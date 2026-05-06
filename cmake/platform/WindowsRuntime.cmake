# Tachyon Windows Runtime Configuration

if(MSVC AND TACHYON_ENABLE_SKIA)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC runtime library" FORCE)
endif()
