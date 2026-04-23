@echo off
setlocal EnableDelayedExpansion

set TEST_EXE=C:\Users\pater\Pyt\Tachyon\build\tests\RelWithDebInfo\TachyonTests.exe
set PASSED=0
set FAILED=0
set FAILED_TESTS=

echo Running all tests...
echo.

for %%T in (math property expression asset_resolution image_manager image_decode framebuffer rasterizer surface draw_list_builder blend_modes evaluated_composition_renderer path_rasterizer path_rasterizer_aa frame_cache frame_cache_budget tiling_integration runtime_backbone frame_executor frame_output_sink tile_scheduler render_contract scene_evaluator render_session render_batch parallax_cards timeline camera_cuts camera_shake bezier_interpolator track track_binding planar_track camera_solver matte_resolver glyph_cache text effect_host precomp_mask golden tiling scene_spec render_job expression_vm) do (
    echo Running %%T...
    cmd /c "set TACHYON_TEST_FILTER=%%T& %TEST_EXE%" 2>&1 | findstr /C:"FAIL" /C:"OK"
    echo.
)

echo.
echo Test run complete.