include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRuntimeCoreSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRuntimeExecutionSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRuntimePlaybackSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRuntimeProfilingSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRuntimeTelemetrySources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonRuntimeCompilerSources.cmake)

set(TachyonRuntimeSources
    ${TachyonRuntimeCoreSources}
    ${TachyonRuntimeExecutionSources}
    ${TachyonRuntimePlaybackSources}
    ${TachyonRuntimeProfilingSources}
    ${TachyonRuntimeTelemetrySources}
    ${TachyonRuntimeCompilerSources}
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/cache/cache_key_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/cache/disk_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/cache/frame_cache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/resource/render_context.cpp
    
    # Policies
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/policy/worker_policy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/policy/surface_pool_policy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime/policy/telemetry_policy.cpp
)
