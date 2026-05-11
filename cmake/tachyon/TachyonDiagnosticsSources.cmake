# TachyonDiagnostics sources
set(TachyonDiagnosticsSources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/analysis/scene_inspector.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/diagnostics/trace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/diagnostics/trace_session.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/diagnostics/trace_backend_noop.cpp
)

if(TACHYON_ENABLE_JSON_TRACE)
    list(APPEND TachyonDiagnosticsSources
        ${CMAKE_CURRENT_SOURCE_DIR}/diagnostics/trace_backend_json.cpp
    )
endif()

if(TACHYON_ENABLE_PERFETTO)
    list(APPEND TachyonDiagnosticsSources
        ${CMAKE_CURRENT_SOURCE_DIR}/diagnostics/trace_backend_perfetto.cpp
    )
endif()
