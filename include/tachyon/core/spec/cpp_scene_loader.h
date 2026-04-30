#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <filesystem>
#include <string>
#include <optional>

namespace tachyon {

/**
 * @brief Logic for compiling and loading a C++ scene script dynamically.
 */
class CppSceneLoader {
public:
    struct Result {
        std::optional<SceneSpec> scene;
        std::string diagnostics;
        bool success{false};
    };

    /**
     * @brief Compiles the specified C++ file into a temporary DLL/SO, 
     * loads it, and calls the build_scene() entry point.
     * 
     * @param cpp_path Path to the .cpp scene script.
     * @param output_dir Directory for temporary build artifacts.
     * @return Result containing the built SceneSpec or diagnostics on failure.
     */
    static Result load_from_file(
        const std::filesystem::path& cpp_path,
        const std::filesystem::path& output_dir = "build/cpp_scenes"
    );

private:
    static std::string get_compiler_command(
        const std::filesystem::path& cpp_path,
        const std::filesystem::path& dll_path
    );
};

// Signature for the entry point in the C++ scene script
typedef void (*BuildSceneFunc)(SceneSpec&);

} // namespace tachyon
