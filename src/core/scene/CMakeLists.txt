# Tachyon Scene Module

set(TachyonSceneEvalSources
    evaluator.cpp
    evaluator_composition.cpp
    evaluator_math.cpp
    evaluator/camera2d_evaluator.cpp
    evaluator/hashing.cpp
    evaluator/layer_evaluator.cpp
    evaluator/layer_utils.cpp
    evaluator/repeater_evaluator.cpp
    evaluator/precomp_evaluator.cpp
    evaluator/layer_dependency_resolver.cpp
    evaluator/effect_evaluator.cpp
    evaluator/roi_calculator.cpp
    evaluator/templates.cpp
    evaluator/property_sampler.cpp
    transform_resolver.cpp
    ../../render/intent_builder.cpp
)

set(TachyonSceneAuthoringSources
    builders/expr_helpers.cpp
    builders/anim_helpers.cpp
    builders/transition_builder.cpp
    builders/text_builder.cpp
    builders/effect_builder.cpp
    builders/layer_builder.cpp
    builders/composition_builder.cpp
    builders/scene_builder.cpp
    data_loader.cpp
    dependency_node.cpp
    state/evaluated_camera2d_state.cpp
    ../spec/cpp_scene_loader.cpp
    ../spec/authoring_service.cpp
    ../analysis/motion_map.cpp
)

set(TachyonSceneSchemaSources
    ../spec/schema/scene_spec.cpp
    ../spec/schema/scene_spec_core.cpp
    ../spec/schema/migration/scene_migration.cpp
    ../spec/schema/objects/background_spec.cpp
)

set(TachyonSceneValidationSources
    ../spec/validation/layer_spec_normalizer.cpp
    ../spec/validation/scene_normalizer.cpp
    ../spec/validation/scene_validator_composition.cpp
    ../spec/validation/scene_validator_layer.cpp
    ../spec/validation/scene_validator_schema.cpp
    ../spec/validation/scene_validator_utils.cpp
)

set(TachyonSceneShapeSources
    ../spec/shapes/shape_spec_builder.cpp
    ../spec/shapes/shape_factory.cpp
)

set(TachyonSceneSources
    ${TachyonSceneAuthoringSources}
    ${TachyonSceneSchemaSources}
    ${TachyonSceneValidationSources}
    ${TachyonSceneShapeSources}
)

# Targets
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
tachyon_link_omp(TachyonSceneEval)

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
        TachyonCatalog
)
tachyon_link_omp(TachyonScene)
