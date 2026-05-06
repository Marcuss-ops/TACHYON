set(TachyonTrackerTestSources
    unit/tracker/track_tests.cpp
    unit/tracker/track_binding_tests.cpp
    unit/tracker/planar_track_tests.cpp
    unit/tracker/camera_solver_tests.cpp
    unit/tracker/optical_flow_tests.cpp
)

# ---------------------------------------------------------
# 6. RENDER DOMAIN: TachyonRenderTests
# renderer2d/*, renderer3d/*, asset resolution
# ---------------------------------------------------------
if(TACHYON_ENABLE_3D)
    add_executable(TachyonRenderTests
        unit/test_main_render.cpp
        unit/media/asset_resolution_tests.cpp
        unit/media/image_manager_tests.cpp
        unit/media/image_decode_tests.cpp
        unit/renderer2d/composition/framebuffer_tests.cpp
        unit/renderer2d/pipeline/scene3d_bridge_tests.cpp
        unit/renderer2d/rasterization/rasterizer_tests.cpp
        unit/renderer2d/utilities/surface_tests.cpp
        unit/renderer2d/composition/draw_list_builder_tests.cpp
        unit/renderer2d/effects/blend_modes_tests.cpp
        unit/renderer2d/rasterization/path_rasterizer_tests.cpp
        unit/renderer2d/rasterization/path_rasterizer_aa_tests.cpp
        unit/renderer2d/utilities/tiling_tests.cpp
        unit/renderer2d/effects/effect_host_tests.cpp
        unit/renderer2d/composition/matte_resolver_tests.cpp
        unit/renderer2d/composition/evaluated_composition_renderer_tests.cpp
        unit/renderer3d/motion_blur_tests.cpp
        unit/renderer3d/modifiers/modifier_tests.cpp
        unit/runtime/native_render_tests.cpp
    )

    if(TACHYON_ENABLE_TRACKER)
        target_sources(TachyonRenderTests PRIVATE ${TachyonTrackerTestSources})
    endif()

    add_executable(TachyonRenderPipelineTests
        unit/test_main_render_pipeline.cpp
        integration/render_session_tests.cpp
        integration/parallax_cards_tests.cpp
        integration/png_3d_validation_tests.cpp
        unit/renderer3d/motion_blur_tests.cpp
        unit/renderer3d/temporal/time_remap_tests.cpp
        unit/renderer3d/temporal/frame_blend_tests.cpp
        unit/renderer3d/temporal/rolling_shutter_tests.cpp
        unit/renderer3d/modifiers/modifier_tests.cpp
    )

    target_compile_definitions(TachyonRenderTests
        PRIVATE
            TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
    )

    target_link_libraries(TachyonRenderTests
        PRIVATE
            TachyonTestUtils
            TachyonPresets
            TachyonRenderer2D
            TachyonRenderer3D
            TachyonRuntime
    )

    if(TACHYON_ENABLE_TRACKER)
        target_link_libraries(TachyonRenderTests PRIVATE TachyonTracker)
    endif()

    if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
        target_precompile_headers(TachyonRenderTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
    endif()

    if(MSVC)
        file(GLOB OIDN_RUNTIME_DLLS CONFIGURE_DEPENDS "${oidn_SOURCE_DIR}/bin/*.dll")
        if(OIDN_RUNTIME_DLLS)
            add_custom_command(TARGET TachyonRenderTests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${OIDN_RUNTIME_DLLS}
                        $<TARGET_FILE_DIR:TachyonRenderTests>
                COMMAND_EXPAND_LISTS
            )
        endif()
    endif()

    if(MSVC)
        target_compile_options(TachyonRenderTests PRIVATE /FS)
    endif()

    target_compile_definitions(TachyonRenderPipelineTests
        PRIVATE
            TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
    )

    target_link_libraries(TachyonRenderPipelineTests
        PRIVATE
            TachyonTestUtils
            TachyonRuntime
            TachyonRenderer2D
            TachyonRenderer3D
    )

    if(TACHYON_ENABLE_TRACKER)
        target_link_libraries(TachyonRenderPipelineTests PRIVATE TachyonTracker)
    endif()

    if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
        target_precompile_headers(TachyonRenderPipelineTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
    endif()

    if(MSVC)
        file(GLOB OIDN_RUNTIME_DLLS CONFIGURE_DEPENDS "${oidn_SOURCE_DIR}/bin/*.dll")
        if(OIDN_RUNTIME_DLLS)
            add_custom_command(TARGET TachyonRenderPipelineTests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${OIDN_RUNTIME_DLLS}
                        $<TARGET_FILE_DIR:TachyonRenderPipelineTests>
                COMMAND_EXPAND_LISTS
            )
        endif()
    endif()

    if(MSVC)
        target_compile_options(TachyonRenderPipelineTests PRIVATE /FS)
    endif()

    add_executable(TachyonScene3DSmokeTests
        unit/test_main_scene3d_smoke.cpp
        integration/scene3d_smoke_tests.cpp
    )

    target_compile_definitions(TachyonScene3DSmokeTests
        PRIVATE
            TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
    )

    target_link_libraries(TachyonScene3DSmokeTests
        PRIVATE
            TachyonTestUtils
            TachyonRuntime
            TachyonRenderer3D
    )

    if(TACHYON_ENABLE_TRACKER)
        target_link_libraries(TachyonScene3DSmokeTests PRIVATE TachyonTracker)
    endif()

    if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
        target_precompile_headers(TachyonScene3DSmokeTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
    endif()

    if(MSVC)
        file(GLOB OIDN_RUNTIME_DLLS CONFIGURE_DEPENDS "${oidn_SOURCE_DIR}/bin/*.dll")
        if(OIDN_RUNTIME_DLLS)
            add_custom_command(TARGET TachyonScene3DSmokeTests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${OIDN_RUNTIME_DLLS}
                        $<TARGET_FILE_DIR:TachyonScene3DSmokeTests>
                COMMAND_EXPAND_LISTS
            )
        endif()
    endif()

    if(MSVC)
        target_compile_options(TachyonScene3DSmokeTests PRIVATE /FS)
    endif()

    add_test(
        NAME TachyonRenderTests
        COMMAND TachyonRenderTests
    )
    tachyon_set_test_labels(TachyonRenderTests render)

    add_test(
        NAME TachyonRenderPipelineTests
        COMMAND TachyonRenderPipelineTests
    )
    tachyon_set_test_labels(TachyonRenderPipelineTests render deterministic)

    add_test(
        NAME TachyonScene3DSmokeTests
        COMMAND TachyonScene3DSmokeTests
    )
    tachyon_set_test_labels(TachyonScene3DSmokeTests render)
endif()
