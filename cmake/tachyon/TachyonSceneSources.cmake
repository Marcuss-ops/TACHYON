# TachyonScene sources
# TODO: Technical debt - GLOB_RECURSE is risky for large engines, prefer explicit per-module source lists
file(GLOB_RECURSE TACHYON_ALL_CPP CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

set(TachyonSceneSources ${TACHYON_ALL_CPP})
list(FILTER TachyonSceneSources INCLUDE REGEX "/(core/(scene|spec|math|camera|expressions|model|platform|properties|shapes|timeline)|importer|presets|timeline)/|/(background_generator|camera_cut_contract)\\.cpp$")
list(FILTER TachyonSceneSources EXCLUDE REGEX "/core/spec/(shapes|schema/migration|template_registry|asset_resolver|pipeline/unified_scene_pipeline)/")
list(APPEND TachyonSceneSources ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/scene_hash_builder.cpp)

set(TachyonLegacyJsonSources)
if(TACHYON_ENABLE_LEGACY_JSON_SCENE)
    list(APPEND TachyonLegacyJsonSources
        ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/legacy/scene_spec_json.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/legacy/layer_parse_json.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/legacy/json_parse_helpers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/property_parse_json.cpp
    )
endif()

list(APPEND TachyonSceneSources ${TachyonLegacyJsonSources})
list(APPEND TachyonSceneSources ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/json_report_utils.cpp)
