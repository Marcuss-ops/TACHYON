# TachyonRenderer2D evaluated-composition sources
set(TachyonRenderer2DEvaluatedCompositionSources
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/effects/effect_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/matte_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/pipeline/composition_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/pipeline/layer_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/pipeline/layer_to_draw_command.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/primitives/feathered_mask_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/primitives/layer_renderer_procedural.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/primitives/layer_renderer_simple.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/primitives/mask_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/primitives/text_mesh_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/utilities/composition_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/intent_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/rendering/pipeline/scene3d_bridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/renderer2d/evaluated_composition/utilities/raster.cpp
)
