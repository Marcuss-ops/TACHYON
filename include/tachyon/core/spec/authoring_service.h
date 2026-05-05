#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <optional>

namespace tachyon::core::spec {

class AuthoringService {
public:
    static AuthoringService& get_instance();

    struct CompileResult {
        bool success{false};
        std::string error;
        std::string command;
        std::filesystem::path output_path;
        double duration_seconds{0.0};
    };

    static bool is_compiler_available();
    
    static std::string get_compiler_command(
        const std::filesystem::path& cpp_path,
        const std::filesystem::path& dll_path
    );

    static CompileResult compile_to_shared_lib(
        const std::filesystem::path& cpp_path,
        const std::filesystem::path& output_dir
    );

private:
#ifdef _WIN32
    static bool msvc_environment_is_ready();
    static std::optional<std::filesystem::path> find_vswhere_exe();
    static std::optional<std::filesystem::path> find_vcvars64_bat();
#endif

    static std::vector<std::string> get_link_libs();
};

} // namespace tachyon::core::spec
