#include "tachyon/core/c_api.h"
#include "tachyon/core/cli.h"
#include "tachyon/core/diag/log.h"
#include "tachyon/version.h"

#include <cstring>
#include <algorithm>
#include <mutex>
#include <string>
#include <vector>
#include <stdexcept>

namespace {

void write_error(char* buf, int size, const char* msg) {
    if (!buf || size <= 0 || !msg) return;
    int n = std::min<int>(size - 1, static_cast<int>(std::strlen(msg)));
    std::memcpy(buf, msg, static_cast<std::size_t>(n));
    buf[n] = '\0';
}

bool g_initialized = false;
std::once_flag g_init_flag;

} // namespace

extern "C" {

TACHYON_API const char* tachyon_version(void) {
    return TACHYON_VERSION_STR;
}

TACHYON_API int tachyon_init(void) {
    std::call_once(g_init_flag, []() {
        tachyon::diag::init_logging();
        if (tachyon::has_backend_init()) {
            tachyon::g_init_backends_ptr();
        }
        g_initialized = true;
    });
    return g_initialized ? 0 : 1;
}

TACHYON_API int tachyon_run(int argc, const char** argv,
                             char* error_out, int error_size) {
    if (argc <= 0 || !argv) {
        write_error(error_out, error_size, "tachyon_run: argc/argv invalid");
        return 1;
    }

    if (!tachyon::has_cli_entrypoint()) {
        write_error(error_out, error_size,
            "tachyon_run: CLI entrypoint not registered. Ensure TachyonCLI is linked into tachyon_lib.");
        return 1;
    }

    // run_cli takes char** — make mutable copies on the stack
    std::vector<std::string> args(argv, argv + argc);
    std::vector<char*> argv_mut;
    argv_mut.reserve(static_cast<std::size_t>(argc));
    for (auto& s : args) argv_mut.push_back(s.data());

    try {
        return tachyon::g_run_cli_ptr(static_cast<int>(argv_mut.size()), argv_mut.data());
    } catch (const std::exception& e) {
        write_error(error_out, error_size, e.what());
        return 1;
    } catch (...) {
        write_error(error_out, error_size, "unknown exception in tachyon_run");
        return 1;
    }
}

TACHYON_API void tachyon_init_render_options(TachyonRenderOptions* options) {
    if (!options) return;
    std::memset(options, 0, sizeof(TachyonRenderOptions));
    options->quality = "draft";
    options->start_frame = -1;
    options->end_frame = -1;
}

TACHYON_API int tachyon_render(const TachyonRenderOptions* options,
                                char* error_out, int error_size) {
    if (!options) {
        write_error(error_out, error_size, "tachyon_render: options is null");
        return 1;
    }

    std::vector<const char*> args;
    args.push_back("tachyon");
    args.push_back("render");

    // Persistent storage for stringified numbers used by argc/argv proxy
    std::string s_frames, s_workers;

    if (options->preset_id) {
        args.push_back("--preset");
        args.push_back(options->preset_id);
    } else if (options->cpp_path) {
        args.push_back("--cpp");
        args.push_back(options->cpp_path);
    }

    if (options->output_path) {
        args.push_back("--out");
        args.push_back(options->output_path);
    }

    if (options->quality) {
        args.push_back("--quality");
        args.push_back(options->quality);
    }

    if (options->start_frame >= 0 || options->end_frame >= 0) {
        std::int64_t start = options->start_frame >= 0 ? options->start_frame : 0;
        std::int64_t end = options->end_frame >= 0 ? options->end_frame : start;
        s_frames = std::to_string(start) + "-" + std::to_string(end);
        args.push_back("--frames");
        args.push_back(s_frames.c_str());
    }

    if (options->worker_count > 0) {
        args.push_back("--workers");
        s_workers = std::to_string(options->worker_count);
        args.push_back(s_workers.c_str());
    }

    if (options->library_path) {
        args.push_back("--library");
        args.push_back(options->library_path);
    }

    return tachyon_run(static_cast<int>(args.size()), args.data(), error_out, error_size);
}

} // extern "C"
