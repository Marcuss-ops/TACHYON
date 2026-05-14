# TachyonScene sources and targets
# This file is consolidated from former fragmented .cmake files

set(TachyonSceneAuthoringSources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/expr_helpers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/anim_helpers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/transition_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/text_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/effect_builder.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/layer_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/composition_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builders/scene_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/data_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/dependency_node.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/state/evaluated_camera2d_state.cpp

    # Canonical authoring path
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/cpp_scene_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/authoring_service.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/core/analysis/motion_map.cpp
)

set(TachyonSceneSchemaSources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/scene_spec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/scene_spec_core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/migration/scene_migration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/objects/background_spec.cpp
)

set(TachyonSceneValidationSources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/layer_spec_normalizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_normalizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_composition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_layer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_schema.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_utils.cpp
)


set(TachyonSceneShapeSources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/shapes/shape_spec_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/shapes/shape_factory.cpp
)

set(TachyonSceneSources
    ${TachyonSceneAuthoringSources}
    ${TachyonSceneSchemaSources}
    ${TachyonSceneValidationSources}
    ${TachyonSceneShapeSources}
)

set(TachyonSceneAllSources
    ${TachyonSceneSources}
    ${TachyonSceneEvalSources}
)

# --- Targets ---
add_library(TachyonSceneEval STATIC ${TachyonSceneEvalSources})
tachyon_configure_common(TachyonSceneEval)
target_link_libraries(TachyonSceneEval
    PUBLIC
        TachyonTimeline
    PRIVATE
        TachyonCore
        TachyonColor
    )


    if(TACHYON_ENABLE_TEXT)
        target_link_libraries(TachyonSceneEval PRIVATE TachyonText)
    endif()


tachyon_link_text_deps(TachyonSceneEval)
tachyon_link_media_deps(TachyonSceneEval)
tachyon_link_omp(TachyonSceneEval)

add_library(TachyonPresets STATIC ${TachyonPresetsSources})
tachyon_configure_common(TachyonPresets)
target_link_libraries(TachyonPresets
    PUBLIC
        TachyonCore
        TachyonContent
    )


    if(TACHYON_ENABLE_TEXT)
        target_link_libraries(TachyonPresets PUBLIC TachyonText)
    endif()


tachyon_link_text_deps(TachyonPresets)
tachyon_link_media_deps(TachyonPresets)
tachyon_link_omp(TachyonPresets)

add_library(TachyonScene STATIC ${TachyonSceneSources})
tachyon_configure_common(TachyonScene)

target_link_libraries(TachyonScene
    PUBLIC
        TachyonSceneEval
        TachyonTimeline
    PRIVATE
        TachyonCore
        TachyonColor
        TachyonPlatform
        TachyonPresets
        TachyonLibrary
)

tachyon_link_omp(TachyonScene)
