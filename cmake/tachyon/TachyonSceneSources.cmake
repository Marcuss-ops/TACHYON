# TachyonScene sources
set(TachyonSceneSources
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/data_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/dependency_node.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/evaluator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/evaluator_composition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/evaluator_math.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/evaluator/camera2d_evaluator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/evaluator/hashing.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/evaluator/layer_evaluator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/evaluator/layer_transform_3d.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/evaluator/layer_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/evaluator/light_evaluator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/evaluator/mesh_animator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/evaluator/property_sampler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/evaluator/roi_calculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/evaluator/templates.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/rigging/constraint_solver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/rigging/ik_solver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/rigging/rig_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/scene/state/evaluated_camera2d_state.cpp
    
    # Spec - Core
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/scene_spec_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/scene_spec_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/cpp_scene_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/asset_resolver.cpp
    
    # Spec - Schema
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/scene_spec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/scene_spec_core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/migration/scene_migration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/objects/background_spec.cpp
    
    # Spec - Compilation
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/component_library.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/effect_compiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/layer_compiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/preset_compiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/property_compiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/scene_compiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/scene_compiler_build.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/scene_compiler_graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/scene_compiler_hashing.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/scene_compiler_property.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/scene_compiler_resolve.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/scene_hash_builder.cpp
    
    # Spec - Validation
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_composition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_layer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_schema.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_utils.cpp

    # Spec - Shapes
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/shapes/shape_spec_builder.cpp
    
    # Timeline
    ${CMAKE_CURRENT_SOURCE_DIR}/timeline/camera_cuts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timeline/frame_blend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timeline/frame_evaluator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timeline/time.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timeline/time_remap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timeline/time_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/analysis/motion_map.cpp

    # Importers
    ${CMAKE_CURRENT_SOURCE_DIR}/importer/alembic_importer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/importer/usd_importer.cpp

    # Presets
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/builders_common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/background/background_builders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/background/background_preset_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/image/image_builders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/sfx/sfx_builders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/shape/shape_builders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/text/text_builders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/transition/transition_builders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/transition/transition_preset_registry.cpp
)
