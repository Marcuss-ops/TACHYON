# Tachyon Backend Registry Module

set(TachyonBackendRegistrySources
    backend_registry.cpp
    backend_init.cpp
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

set_target_properties(TachyonBackendRegistry PROPERTIES POSITION_INDEPENDENT_CODE ON)
