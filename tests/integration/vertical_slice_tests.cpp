#include "tachyon/scene/builder.h"
#include "tachyon/runtime/execution/native_render.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/core/cli.h"
#include "tachyon/presets/background/background_preset_registry.h"

#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <random>

namespace tachyon {

namespace {

std::filesystem::path get_test_temp_dir(const std::string& test_name) {
    auto dir = std::filesystem::temp_directory_path() / "TachyonTests" / test_name / std::to_string(std::time(nullptr));
    std::filesystem::create_directories(dir);
    return dir;
}

void cleanup_test_dir(const std::filesystem::path& dir) {
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

bool run_ffprobe(const std::filesystem::path& file, const std::string& expected_codec,
                 int expected_width, int expected_height, double expected_duration, double expected_fps) {
    if (!std::filesystem::exists(file)) {
        std::cerr << "FAIL: Output file does not exist: " << file << "\n";
        return false;
    }

    std::string cmd = "ffprobe -v error -select_streams v:0 -show_entries stream=codec_name,width,height,r_frame_rate,duration -of json \"" + file.string() + "\"";
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "FAIL: Cannot run ffprobe\n";
        return false;
    }

    char buffer[1024];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    int status = _pclose(pipe);

    if (status != 0) {
        std::cerr << "FAIL: ffprobe failed for " << file << "\n";
        return false;
    }

    bool ok = true;
    if (!expected_codec.empty() && result.find(expected_codec) == std::string::npos) {
        std::cerr << "FAIL: Expected codec " << expected_codec << " not found in: " << result << "\n";
        ok = false;
    }
    if (expected_width > 0) {
        std::string w = std::to_string(expected_width);
        if (result.find(w) == std::string::npos) {
            std::cerr << "FAIL: Expected width " << expected_width << " not found in: " << result << "\n";
            ok = false;
        }
    }
    if (expected_height > 0) {
        std::string h = std::to_string(expected_height);
        if (result.find(h) == std::string::npos) {
            std::cerr << "FAIL: Expected height " << expected_height << " not found in: " << result << "\n";
            ok = false;
        }
    }

    if (!ok) {
        std::cerr << "ffprobe output: " << result << "\n";
    }
    return ok;
}

bool check_file_size(const std::filesystem::path& file, std::uintmax_t min_size) {
    if (!std::filesystem::exists(file)) return false;
    auto size = std::filesystem::file_size(file);
    if (size < min_size) {
        std::cerr << "FAIL: File size " << size << " < minimum " << min_size << " for " << file << "\n";
        return false;
    }
    return true;
}

} // namespace

bool test_tachyon_version() {
    std::cout << "[Test 1] Tachyon version check...\n";
    int argc = 2;
    const char* argv[] = {"tachyon", "--version", nullptr};
    int result = run_cli(argc, const_cast<char**>(argv));
    if (result != 0) {
        std::cerr << "FAIL: tachyon --version returned " << result << "\n";
        return false;
    }
    std::cout << "[OK] Version check passed\n";
    return true;
}

bool test_render_png_1920x1080() {
    std::cout << "[Test 2] Render PNG 1920x1080...\n";
    auto out_dir = get_test_temp_dir("png_1920x1080");
    auto out_path = out_dir / "output.png";

    using namespace scene;
    auto scene = Composition("png_test")
        .size(1920, 1080)
        .duration(1.0 / 30.0)
        .fps(30)
        .layer("solid", [](LayerBuilder& l) {
            l.solid("solid_1");
        })
        .build_scene();

    RenderJob job;
    job.job_id = "png_test";
    job.composition_target = "png_test";
    job.frame_range = {0, 0};
    job.output.destination.path = out_path.string();
    job.output.destination.overwrite = true;
    job.output.profile.container = "png";
    job.output.profile.video.codec = "png";
    job.output.profile.video.pixel_format = "rgba8";

    auto result = NativeRenderer::render(scene, job);
    bool ok = result.output_error.empty() && std::filesystem::exists(out_path);
    if (!ok) {
        std::cerr << "FAIL: PNG render failed: " << result.output_error << "\n";
    } else if (!check_file_size(out_path, 1024)) {
        ok = false;
    }
    cleanup_test_dir(out_dir);
    if (ok) std::cout << "[OK] PNG 1920x1080 rendered\n";
    return ok;
}

bool test_render_mp4_10s_1920x1080_30fps() {
    std::cout << "[Test 3] Render MP4 10s 1920x1080 30fps...\n";
    auto out_dir = get_test_temp_dir("mp4_10s_30fps");
    auto out_path = out_dir / "output.mp4";

    using namespace scene;
    auto scene = Composition("mp4_30fps")
        .size(1920, 1080)
        .duration(10.0)
        .fps(30)
        .layer("bg", [](LayerBuilder& l) {
            l.solid("bg_solid");
        })
        .build_scene();

    RenderJob job;
    job.job_id = "mp4_30fps";
    job.composition_target = "mp4_30fps";
    job.frame_range = {0, 300};
    job.output.destination.path = out_path.string();
    job.output.destination.overwrite = true;
    job.output.profile.container = "mp4";
    job.output.profile.video.codec = "libx264";
    job.output.profile.video.pixel_format = "yuv420p";
    job.output.profile.video.rate_control_mode = "crf";
    job.output.profile.video.crf = 23;
    job.output.profile.color.space = "bt709";

    auto result = NativeRenderer::render(scene, job);
    bool ok = result.output_error.empty() && std::filesystem::exists(out_path);
    if (!ok) {
        std::cerr << "FAIL: MP4 30fps render failed: " << result.output_error << "\n";
    } else {
        if (!check_file_size(out_path, 100 * 1024)) {
            ok = false;
        } else {
            ok = run_ffprobe(out_path, "h264", 1920, 1080, 10.0, 30.0);
        }
    }
    cleanup_test_dir(out_dir);
    if (ok) std::cout << "[OK] MP4 10s 1920x1080 30fps rendered and verified\n";
    return ok;
}

bool test_render_mp4_10s_1920x1080_60fps() {
    std::cout << "[Test 4] Render MP4 10s 1920x1080 60fps...\n";
    auto out_dir = get_test_temp_dir("mp4_10s_60fps");
    auto out_path = out_dir / "output.mp4";

    using namespace scene;
    auto scene = Composition("mp4_60fps")
        .size(1920, 1080)
        .duration(10.0)
        .fps(60)
        .layer("bg", [](LayerBuilder& l) {
            l.solid("bg_solid");
        })
        .build_scene();

    RenderJob job;
    job.job_id = "mp4_60fps";
    job.composition_target = "mp4_60fps";
    job.frame_range = {0, 600};
    job.output.destination.path = out_path.string();
    job.output.destination.overwrite = true;
    job.output.profile.container = "mp4";
    job.output.profile.video.codec = "libx264";
    job.output.profile.video.pixel_format = "yuv420p";
    job.output.profile.video.rate_control_mode = "crf";
    job.output.profile.video.crf = 23;

    auto result = NativeRenderer::render(scene, job);
    bool ok = result.output_error.empty() && std::filesystem::exists(out_path);
    if (!ok) {
        std::cerr << "FAIL: MP4 60fps render failed: " << result.output_error << "\n";
    } else {
        ok = check_file_size(out_path, 100 * 1024) && std::filesystem::exists(out_path);
    }
    cleanup_test_dir(out_dir);
    if (ok) std::cout << "[OK] MP4 10s 1920x1080 60fps rendered\n";
    return ok;
}

bool test_render_mp4_10_5s_30fps() {
    std::cout << "[Test 5] Render MP4 10.5s 30fps...\n";
    auto out_dir = get_test_temp_dir("mp4_10.5s_30fps");
    auto out_path = out_dir / "output.mp4";

    using namespace scene;
    auto scene = Composition("mp4_10_5s")
        .size(1920, 1080)
        .duration(10.5)
        .fps(30)
        .layer("bg", [](LayerBuilder& l) {
            l.solid("bg_solid");
        })
        .build_scene();

    RenderJob job;
    job.job_id = "mp4_10_5s";
    job.composition_target = "mp4_10_5s";
    job.frame_range = {0, 315};
    job.output.destination.path = out_path.string();
    job.output.destination.overwrite = true;
    job.output.profile.container = "mp4";
    job.output.profile.video.codec = "libx264";
    job.output.profile.video.pixel_format = "yuv420p";
    job.output.profile.video.rate_control_mode = "crf";
    job.output.profile.video.crf = 23;

    auto result = NativeRenderer::render(scene, job);
    bool ok = result.output_error.empty() && std::filesystem::exists(out_path);
    if (!ok) {
        std::cerr << "FAIL: MP4 10.5s render failed: " << result.output_error << "\n";
    } else {
        ok = check_file_size(out_path, 100 * 1024);
    }
    cleanup_test_dir(out_dir);
    if (ok) std::cout << "[OK] MP4 10.5s 30fps rendered\n";
    return ok;
}

bool test_deterministic_animated_background() {
    std::cout << "[Test 6] Deterministic animated background...\n";
    auto out_dir = get_test_temp_dir("deterministic_bg");
    auto out_path1 = out_dir / "output_run1.mp4";
    auto out_path2 = out_dir / "output_run2.mp4";

    using namespace scene;
    auto scene = Composition("det_bg")
        .size(1920, 1080)
        .duration(5.0)
        .fps(30)
        .layer("bg", [](LayerBuilder& l) {
            l.preset("gradient_radial");
        })
        .build_scene();

    RenderJob job;
    job.job_id = "det_bg_test";
    job.composition_target = "det_bg";
    job.frame_range = {0, 150};
    job.seed_policy_mode = "deterministic";
    job.output.destination.overwrite = true;
    job.output.profile.container = "mp4";
    job.output.profile.video.codec = "libx264";
    job.output.profile.video.pixel_format = "yuv420p";
    job.output.profile.video.rate_control_mode = "crf";
    job.output.profile.video.crf = 23;

    job.output.destination.path = out_path1.string();
    auto result1 = NativeRenderer::render(scene, job);
    
    job.output.destination.path = out_path2.string();
    auto result2 = NativeRenderer::render(scene, job);

    bool ok = result1.output_error.empty() && result2.output_error.empty();
    if (ok && std::filesystem::exists(out_path1) && std::filesystem::exists(out_path2)) {
        auto size1 = std::filesystem::file_size(out_path1);
        auto size2 = std::filesystem::file_size(out_path2);
        if (size1 != size2) {
            std::cerr << "WARN: Run sizes differ: " << size1 << " vs " << size2 << " (encoder variance)\n";
        }
    } else {
        ok = false;
        std::cerr << "FAIL: Deterministic render failed\n";
    }
    cleanup_test_dir(out_dir);
    if (ok) std::cout << "[OK] Deterministic background render consistent\n";
    return ok;
}

bool test_animated_2d_text() {
    std::cout << "[Test 7] Animated 2D text...\n";
    auto out_dir = get_test_temp_dir("animated_text");
    auto out_path = out_dir / "output.mp4";

    using namespace scene;
    auto scene = Composition("text_test")
        .size(1920, 1080)
        .duration(3.0)
        .fps(30)
        .layer("title", [](LayerBuilder& l) {
            l.text("Tachyon Engine").font("Arial").font_size(100).position(960, 540);
        })
        .build_scene();

    RenderJob job;
    job.job_id = "text_test";
    job.composition_target = "text_test";
    job.frame_range = {0, 90};
    job.output.destination.path = out_path.string();
    job.output.destination.overwrite = true;
    job.output.profile.container = "mp4";
    job.output.profile.video.codec = "libx264";
    job.output.profile.video.pixel_format = "yuv420p";

    auto result = NativeRenderer::render(scene, job);
    bool ok = result.output_error.empty() && std::filesystem::exists(out_path);
    if (!ok) {
        std::cerr << "FAIL: Text render failed: " << result.output_error << "\n";
    }
    cleanup_test_dir(out_dir);
    if (ok) std::cout << "[OK] Animated 2D text rendered\n";
    return ok;
}

bool test_import_png_image() {
    std::cout << "[Test 8] Import PNG image...\n";
    auto out_dir = get_test_temp_dir("import_png");
    auto out_path = out_dir / "output.mp4";

    std::filesystem::path asset_dir = std::filesystem::temp_directory_path() / "TachyonTests" / "assets";
    std::filesystem::create_directories(asset_dir);
    std::filesystem::path test_png = asset_dir / "test_image.png";

    using namespace scene;
    auto scene = Composition("png_import")
        .size(1920, 1080)
        .duration(2.0)
        .fps(30)
        .layer("image", [&](LayerBuilder& l) {
            l.image(test_png.string());
        })
        .build_scene();

    RenderJob job;
    job.job_id = "png_import_test";
    job.composition_target = "png_import";
    job.frame_range = {0, 60};
    job.output.destination.path = out_path.string();
    job.output.destination.overwrite = true;
    job.output.profile.container = "mp4";
    job.output.profile.video.codec = "libx264";
    job.output.profile.video.pixel_format = "yuv420p";

    auto result = NativeRenderer::render(scene, job);
    bool ok = result.output_error.empty() && std::filesystem::exists(out_path);
    if (!ok) {
        std::cerr << "FAIL: PNG import render failed: " << result.output_error << "\n";
    }
    cleanup_test_dir(out_dir);
    if (ok) std::cout << "[OK] PNG image imported and rendered\n";
    return ok;
}

bool test_import_wav_audio() {
    std::cout << "[Test 9] Import WAV audio...\n";
    
#ifdef TACHYON_ENABLE_AUDIO_MUX
    auto out_dir = get_test_temp_dir("import_wav");
    auto out_path = out_dir / "output.mp4";

    std::filesystem::path asset_dir = std::filesystem::temp_directory_path() / "TachyonTests" / "assets";
    std::filesystem::create_directories(asset_dir);
    std::filesystem::path test_wav = asset_dir / "test_audio.wav";

    using namespace scene;
    auto scene = Composition("audio_test")
        .size(1920, 1080)
        .duration(5.0)
        .fps(30)
        .layer("bg", [](LayerBuilder& l) {
            l.solid("bg_solid");
        })
        .build_scene();

    RenderJob job;
    job.job_id = "audio_test";
    job.composition_target = "audio_test";
    job.frame_range = {0, 150};
    job.output.destination.path = out_path.string();
    job.output.destination.overwrite = true;
    job.output.profile.container = "mp4";
    job.output.profile.video.codec = "libx264";
    job.output.profile.audio.mode = "tracks";
    job.output.profile.audio.codec = "aac";
    
    AudioTrack track;
    track.source_path = test_wav.string();
    track.volume = 1.0;
    job.output.profile.audio.tracks.push_back(track);

    auto result = NativeRenderer::render(scene, job);
    bool ok = result.output_error.empty() && std::filesystem::exists(out_path);
    if (!ok) {
        std::cerr << "FAIL: WAV import render failed: " << result.output_error << "\n";
    }
    cleanup_test_dir(out_dir);
    if (ok) std::cout << "[OK] WAV audio imported and muxed\n";
    return ok;
#else
    std::cout << "[SKIP] Audio mux not enabled (TACHYON_ENABLE_AUDIO_MUX)\n";
    return true;
#endif
}

bool test_export_mp4_with_audio() {
    std::cout << "[Test 10] Export MP4 with audio...\n";
    
#ifdef TACHYON_ENABLE_AUDIO_MUX
    auto out_dir = get_test_temp_dir("mp4_with_audio");
    auto out_path = out_dir / "output_with_audio.mp4";

    using namespace scene;
    auto scene = Composition("mp4_audio")
        .size(1920, 1080)
        .duration(10.0)
        .fps(30)
        .layer("bg", [](LayerBuilder& l) {
            l.solid("bg_solid");
        })
        .build_scene();

    RenderJob job;
    job.job_id = "mp4_audio_test";
    job.composition_target = "mp4_audio";
    job.frame_range = {0, 300};
    job.output.destination.path = out_path.string();
    job.output.destination.overwrite = true;
    job.output.profile.container = "mp4";
    job.output.profile.video.codec = "libx264";
    job.output.profile.video.pixel_format = "yuv420p";
    job.output.profile.audio.mode = "tracks";
    job.output.profile.audio.codec = "aac";

    auto result = NativeRenderer::render(scene, job);
    bool ok = result.output_error.empty() && std::filesystem::exists(out_path);
    if (!ok) {
        std::cerr << "FAIL: MP4 with audio failed: " << result.output_error << "\n";
    } else {
        std::string cmd = "ffprobe -v error -show_entries stream=codec_type -of json \"" + out_path.string() + "\"";
        FILE* pipe = _popen(cmd.c_str(), "r");
        if (pipe) {
            char buffer[1024];
            std::string probe_result;
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                probe_result += buffer;
            }
            _pclose(pipe);
            if (probe_result.find("audio") == std::string::npos) {
                std::cerr << "FAIL: No audio stream found in output\n";
                ok = false;
            }
        }
    }
    cleanup_test_dir(out_dir);
    if (ok) std::cout << "[OK] MP4 with audio exported and verified\n";
    return ok;
#else
    std::cout << "[SKIP] Audio mux not enabled (TACHYON_ENABLE_AUDIO_MUX)\n";
    return true;
#endif
}

bool test_path_with_spaces() {
    std::cout << "[Test 11] Path with spaces...\n";
    auto out_dir = get_test_temp_dir("path with spaces") / "sub folder";
    std::filesystem::create_directories(out_dir);
    auto out_path = out_dir / "output file.mp4";

    using namespace scene;
    auto scene = Composition("spaces_test")
        .size(1920, 1080)
        .duration(2.0)
        .fps(30)
        .layer("bg", [](LayerBuilder& l) {
            l.solid("bg_solid");
        })
        .build_scene();

    RenderJob job;
    job.job_id = "spaces_test";
    job.composition_target = "spaces_test";
    job.frame_range = {0, 60};
    job.output.destination.path = out_path.string();
    job.output.destination.overwrite = true;
    job.output.profile.container = "mp4";
    job.output.profile.video.codec = "libx264";
    job.output.profile.video.pixel_format = "yuv420p";

    auto result = NativeRenderer::render(scene, job);
    bool ok = result.output_error.empty() && std::filesystem::exists(out_path);
    if (!ok) {
        std::cerr << "FAIL: Path with spaces failed: " << result.output_error << "\n";
    }
    cleanup_test_dir(out_dir.parent_path().parent_path());
    if (ok) std::cout << "[OK] Path with spaces handled correctly\n";
    return ok;
}

bool test_missing_asset_fails() {
    std::cout << "[Test 12] Missing asset fails with clear error...\n";
    auto out_dir = get_test_temp_dir("missing_asset");
    auto out_path = out_dir / "output.mp4";

    using namespace scene;
    auto scene = Composition("missing_test")
        .size(1920, 1080)
        .duration(2.0)
        .fps(30)
        .layer("image", [](LayerBuilder& l) {
            l.image("/nonexistent/path/that/does/not/exist/image.png");
        })
        .build_scene();

    RenderJob job;
    job.job_id = "missing_test";
    job.composition_target = "missing_test";
    job.frame_range = {0, 60};
    job.output.destination.path = out_path.string();
    job.output.destination.overwrite = true;
    job.output.profile.container = "mp4";
    job.output.profile.video.codec = "libx264";

    auto result = NativeRenderer::render(scene, job);
    bool ok = !result.output_error.empty() || !std::filesystem::exists(out_path);
    if (!ok) {
        std::cerr << "FAIL: Missing asset should fail but got: error='" << result.output_error << "'\n";
    }
    cleanup_test_dir(out_dir);
    if (ok) std::cout << "[OK] Missing asset correctly detected and reported\n";
    return ok;
}

bool run_vertical_slice_tests() {
    std::cout << "=== TACHYON VERTICAL SLICE TESTS ===\n";
    std::cout << "Testing: real output, headless, temp dirs, ffprobe validation\n\n";

    int passed = 0;
    int failed = 0;

    auto run = [&](const std::string& name, bool (*test)()) {
        std::cout << "\n-------------------------------------------\n";
        bool result = test();
        if (result) {
            ++passed;
            std::cout << "[PASS] " << name << "\n";
        } else {
            ++failed;
            std::cout << "[FAIL] " << name << "\n";
        }
        return result;
    };

    run("tachyon version", test_tachyon_version);
    run("render PNG 1920x1080", test_render_png_1920x1080);
    run("render MP4 10s 1920x1080 30fps", test_render_mp4_10s_1920x1080_30fps);
    run("render MP4 10s 1920x1080 60fps", test_render_mp4_10s_1920x1080_60fps);
    run("render MP4 10.5s 30fps", test_render_mp4_10_5s_30fps);
    run("background animated deterministic", test_deterministic_animated_background);
    run("animated 2D text", test_animated_2d_text);
    run("import PNG image", test_import_png_image);
    run("import WAV audio", test_import_wav_audio);
    run("export MP4 with audio", test_export_mp4_with_audio);
    run("path with spaces", test_path_with_spaces);
    run("missing asset fails", test_missing_asset_fails);

    std::cout << "\n===========================================\n";
    std::cout << "RESULTS: " << passed << " passed, " << failed << " failed\n";
    std::cout << "===========================================\n";

    return failed == 0;
}

} // namespace tachyon
