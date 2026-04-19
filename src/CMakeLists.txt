# Core Engine Library
add_library(TachyonCore
    core/model/tachyon.cpp
    core/math/matrix4x4.cpp
    core/math/quaternion.cpp
    core/math/transform3.cpp
    core/camera/camera_state.cpp
    core/expressions/expression_engine.cpp
    media/assets/asset_resolution.cpp
    spec/parser/scene_spec.cpp
    scene/evaluator.cpp
    runtime/jobs/render_job.cpp
    runtime/jobs/render_plan.cpp
    runtime/frame_cache.cpp
    runtime/frame_executor.cpp
    runtime/render_session.cpp
    runtime/render_graph.cpp
    timeline/time_resolver.cpp
    timeline/layer_evaluator.cpp
    timeline/camera_evaluator.cpp
    timeline/frame_evaluator.cpp
    renderer2d/raster/cpu_rasterizer.cpp
    renderer2d/surface_rgba.cpp
    renderer2d/draw_list_builder.cpp
    renderer2d/layer_to_draw_command.cpp
    renderer2d/draw_list_rasterizer.cpp
    renderer2d/evaluated_composition_renderer.cpp
    text/font.cpp
    text/layout.cpp
    timeline/time.cpp
    cli/core/report.cpp
    cli/core/options.cpp
    cli/core/cli.cpp
)

target_include_directories(TachyonCore
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../third_party>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(TachyonCore
    PUBLIC
        nlohmann_json::nlohmann_json
)

add_executable(tachyon
    cli/render/main.cpp
)

target_link_libraries(tachyon
    PRIVATE
        TachyonCore
)
