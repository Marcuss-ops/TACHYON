# Tachyon Backend Registry Module

set(TachyonBackendRegistrySources
    backend_registry.cpp
    backend_init.cpp
    profile_resolver.cpp
)

add_library(TachyonBackendRegistry STATIC ${TachyonBackendRegistrySources})
tachyon_configure_common(TachyonBackendRegistry)

target_include_directories(TachyonBackendRegistry 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)

target_link_libraries(TachyonBackendRegistry
    PUBLIC
        TachyonCore
)

if(TACHYON_ENABLE_MEDIA)
    target_link_libraries(TachyonBackendRegistry PUBLIC TachyonBackendFfmpeg)
endif()

if(TACHYON_ENABLE_WHISPER)
    target_link_libraries(TachyonBackendRegistry PUBLIC TachyonBackendWhisper)
endif()

if(TACHYON_ENABLE_HIGHWAY OR TACHYON_ENABLE_MANUAL_AVX2)
    target_link_libraries(TachyonBackendRegistry PUBLIC TachyonBackendSimd)
endif()

set_target_properties(TachyonBackendRegistry PROPERTIES POSITION_INDEPENDENT_CODE ON)
