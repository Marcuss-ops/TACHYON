# Tachyon BindingsC Module

set(TachyonBindingsCSources
    ../../core/c_api.cpp
)

tachyon_add_module(
    NAME BindingsC
    SOURCES ${TachyonBindingsCSources}
    PUBLIC_DEPS TachyonCore TachyonCLI TachyonBackends
)
