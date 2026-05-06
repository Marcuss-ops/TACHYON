# ---------------------------------------------------------
# 3. RUNTIME DOMAIN: TachyonRuntimeTests
# runtime/execution/*
# ---------------------------------------------------------
add_executable(TachyonRuntimeTests
    unit/test_main_runtime.cpp
    unit/core/spec/render_job_tests.cpp
    unit/runtime/core/frame_cache_tests.cpp
    unit/runtime/core/frame_cache_budget_tests.cpp
    unit/runtime/core/backbone_tests.cpp
    unit/runtime/core/expression_vm_tests.cpp
    unit/runtime/core/determinism_tests.cpp
    unit/runtime/io/frame_output_sink_tests.cpp
    unit/runtime/core/quality_tier_tests.cpp
    unit/runtime/core/runtime_policy_tests.cpp
    unit/runtime/execution/tiling_integration_tests.cpp
    unit/runtime/execution/frame_executor_tests.cpp
    unit/runtime/execution/frame_range_tests.cpp
    unit/runtime/execution/tile_scheduler_tests.cpp
    unit/runtime/frame_adapter_tests.cpp
    unit/runtime/telemetry_industrial_tests.cpp
)

target_compile_definitions(TachyonRuntimeTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonRuntimeTests
    PRIVATE
        TachyonTestUtils
        TachyonRuntime
        TachyonCore
        TachyonPlatform
        TachyonSceneEval
)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonRuntimeTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

if(MSVC)
    file(GLOB OIDN_RUNTIME_DLLS CONFIGURE_DEPENDS "${oidn_SOURCE_DIR}/bin/*.dll")
    if(OIDN_RUNTIME_DLLS)
        add_custom_command(TARGET TachyonRuntimeTests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${OIDN_RUNTIME_DLLS}
                    $<TARGET_FILE_DIR:TachyonRuntimeTests>
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()

if(MSVC)
    target_compile_options(TachyonRuntimeTests PRIVATE /FS)
endif()

add_executable(TachyonNativeRenderTests
    unit/test_main_native_render.cpp
    unit/runtime/native_render_tests.cpp
)

target_compile_definitions(TachyonNativeRenderTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonNativeRenderTests
    PRIVATE
        TachyonTestUtils
        TachyonPresets
        TachyonRuntime
)

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonNativeRenderTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

if(MSVC)
    file(GLOB OIDN_RUNTIME_DLLS CONFIGURE_DEPENDS "${oidn_SOURCE_DIR}/bin/*.dll")
    if(OIDN_RUNTIME_DLLS)
        add_custom_command(TARGET TachyonNativeRenderTests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${OIDN_RUNTIME_DLLS}
                    $<TARGET_FILE_DIR:TachyonNativeRenderTests>
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()

if(MSVC)
    target_compile_options(TachyonNativeRenderTests PRIVATE /FS)
endif()

add_executable(TachyonRenderProfilerTests
    unit/test_main_render_profiler.cpp
    unit/runtime/profiling/render_profiler_tests.cpp
)

target_compile_definitions(TachyonRenderProfilerTests
    PRIVATE
        TACHYON_TESTS_SOURCE_DIR="${TACHYON_TESTS_SOURCE_DIR}"
)

target_link_libraries(TachyonRenderProfilerTests
    PRIVATE
        TachyonTestUtils
        TachyonRuntime
)

if(TACHYON_ENABLE_TRACKER)
    target_link_libraries(TachyonRenderProfilerTests PRIVATE TachyonTracker)
endif()

if(TACHYON_ENABLE_PCH AND COMMAND target_precompile_headers)
    target_precompile_headers(TachyonRenderProfilerTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/tachyon/pch.h")
endif()

if(MSVC)
    file(GLOB OIDN_RUNTIME_DLLS CONFIGURE_DEPENDS "${oidn_SOURCE_DIR}/bin/*.dll")
    if(OIDN_RUNTIME_DLLS)
        add_custom_command(TARGET TachyonRenderProfilerTests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${OIDN_RUNTIME_DLLS}
                    $<TARGET_FILE_DIR:TachyonRenderProfilerTests>
            COMMAND_EXPAND_LISTS
        )
    endif()
endif()

if(MSVC)
    target_compile_options(TachyonRenderProfilerTests PRIVATE /FS)
endif()

add_test(
    NAME TachyonRuntimeTests
    COMMAND TachyonRuntimeTests
)
tachyon_set_test_labels(TachyonRuntimeTests core deterministic)

add_test(
    NAME TachyonNativeRenderTests
    COMMAND TachyonNativeRenderTests
)
tachyon_set_test_labels(TachyonNativeRenderTests render)

add_test(
    NAME TachyonRenderProfilerTests
    COMMAND TachyonRenderProfilerTests
)
tachyon_set_test_labels(TachyonRenderProfilerTests core deterministic)
