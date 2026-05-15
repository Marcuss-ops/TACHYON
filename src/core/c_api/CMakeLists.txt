# Tachyon BindingsC Module

set(TachyonBindingsCSources
    ../c_api.cpp
)

tachyon_add_module(
    NAME BindingsC
    SOURCES ${TachyonBindingsCSources}
    PUBLIC_DEPS TachyonCore
)
