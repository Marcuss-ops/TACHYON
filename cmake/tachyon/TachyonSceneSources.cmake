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
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/template_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/json_report_utils.cpp
    
    # Spec - Schema
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/scene_spec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/scene_spec_core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/common/common_spec_serialize.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/migration/scene_migration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/objects/procedural_spec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/schema/objects/background_spec.cpp
    
    # Spec - Compilation
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/compilation/component_library.cpp
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
    
    # Spec - Serialization
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/serialization/layer_serialize.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/serialization/property_color_serialize.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/serialization/property_scalar_serialize.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/serialization/property_vector_serialize.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/text_animator_spec_serialize.cpp
    
    # Spec - Validation
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_composition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_layer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_schema.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/validation/scene_validator_utils.cpp
    
    # Spec - Parsing
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/parsing/layer_bindings_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/parsing/layer_effects_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/parsing/layer_shape_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/parsing/layer_spec_parse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/parsing/spec_value_parsers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/property_parse_json.cpp
    
    # Spec - Pipeline
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/pipeline/unified_scene_pipeline.cpp
    
    # Spec - Shapes
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/shapes/shape_spec_builder.cpp
    
    # Spec - Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/legacy/json_parse_helpers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/legacy/layer_parse_json.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/legacy/scene_spec_audio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/legacy/scene_spec_json.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/spec/scene_spec_json.cpp
    
    # Timeline
    ${CMAKE_CURRENT_SOURCE_DIR}/timeline/camera_cuts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timeline/frame_blend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timeline/frame_evaluator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timeline/time.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timeline/time_remap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/timeline/time_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_cut_contract.cpp

    # Importers
    ${CMAKE_CURRENT_SOURCE_DIR}/importer/alembic_importer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/importer/usd_importer.cpp

    # Presets
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/background/background_builders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/background/background_preset_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/image/image_builders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/sfx/sfx_builders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/shape/shape_builders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/text/text_builders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/presets/transition/transition_builders.cpp

)
