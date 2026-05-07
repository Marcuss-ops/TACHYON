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
)
