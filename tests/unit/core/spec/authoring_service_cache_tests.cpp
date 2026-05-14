#include "tachyon/core/spec/authoring_service.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace tachyon::core::spec;

int main() {
    std::cout << "Running Authoring Service Cache tests..." << std::endl;

    const std::filesystem::path temp_dir = "temp_jit_test";
    std::filesystem::create_directories(temp_dir);

    const std::filesystem::path cpp1 = temp_dir / "scene1.cpp";
    const std::filesystem::path cpp2 = temp_dir / "scene2.cpp";

    {
        std::ofstream ofs(cpp1);
        ofs << "int main() { return 0; } // Version A";
    }
    {
        std::ofstream ofs(cpp2);
        ofs << "int main() { return 0; } // Version B";
    }

    auto& service = AuthoringService::get_instance();

    // We can't easily run the compiler in a unit test without full environment,
    // but we can check the internal logic if we expose it or just test that 
    // compile_to_shared_lib fails but generates a command with the expected filename.
    
    // Actually, I can just check if the output_path in result is different.
    // Even if compilation fails, the service should have determined the path.
    
    auto res1 = service.compile_to_shared_lib(cpp1, temp_dir / "out");
    auto res2 = service.compile_to_shared_lib(cpp2, temp_dir / "out");

    std::cout << "Path 1: " << res1.output_path << std::endl;
    std::cout << "Path 2: " << res2.output_path << std::endl;

    if (res1.output_path == res2.output_path) {
        std::cerr << "FAILED: Different C++ files resulted in same DLL path." << std::endl;
        return 1;
    }

    // Test that same file results in same path
    auto res3 = service.compile_to_shared_lib(cpp1, temp_dir / "out");
    if (res1.output_path != res3.output_path) {
        std::cerr << "FAILED: Same C++ file resulted in different DLL paths." << std::endl;
        return 1;
    }

    std::filesystem::remove_all(temp_dir);
    std::cout << "Authoring Service Cache tests passed!" << std::endl;
    return 0;
}
