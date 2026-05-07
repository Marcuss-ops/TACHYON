include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonSceneAuthoringSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonSceneSchemaSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonSceneValidationSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonSceneImportSources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/tachyon/TachyonSceneShapeSources.cmake)

set(TachyonSceneSources
    ${TachyonSceneAuthoringSources}
    ${TachyonSceneSchemaSources}
    ${TachyonSceneValidationSources}
    ${TachyonSceneImportSources}
    ${TachyonSceneShapeSources}
)
