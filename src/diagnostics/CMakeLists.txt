# Tachyon Diagnostics Module

set(TachyonDiagnosticsSources
    ../core/analysis/scene_inspector.cpp
    trace.cpp
    trace_session.cpp
    trace_backend_noop.cpp
)

if(TACHYON_ENABLE_JSON_TRACE)
    list(APPEND TachyonDiagnosticsSources trace_backend_json.cpp)
endif()

if(TACHYON_ENABLE_PERFETTO)
    list(APPEND TachyonDiagnosticsSources trace_backend_perfetto.cpp)
endif()

tachyon_add_module(
    NAME Diagnostics
    SOURCES ${TachyonDiagnosticsSources}
    PUBLIC_DEPS TachyonCore
)

if(TACHYON_ENABLE_TRACE)
    target_compile_definitions(TachyonDiagnostics PUBLIC TACHYON_ENABLE_TRACE)
endif()

if(TACHYON_ENABLE_JSON_TRACE)
    target_compile_definitions(TachyonDiagnostics PUBLIC TACHYON_ENABLE_JSON_TRACE)
endif()

if(TACHYON_ENABLE_PERFETTO)
    target_compile_definitions(TachyonDiagnostics PUBLIC TACHYON_ENABLE_PERFETTO)
    target_link_libraries(TachyonDiagnostics PRIVATE perfetto)
endif()
