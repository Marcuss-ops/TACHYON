#pragma once

#include "tachyon/renderer2d/resource/surface.h"
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

namespace tachyon::test {

class GoldenTestHelper {
public:
    /**
     * @brief Simple pixel hashing for quick comparison.
     */
    static std::string hash_surface(const renderer2d::SurfaceRGBA& surface) {
        size_t h = 0;
        const auto* data = surface.data();
        size_t size = surface.width() * surface.height() * 4;
        for (size_t i = 0; i < size; ++i) {
            h ^= std::hash<uint8_t>{}(data[i]) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return std::to_string(h);
    }

    /**
     * @brief Compare surface against a saved golden hash file.
     * If file doesn't exist, it creates it (first run).
     */
    static bool compare_with_golden(const std::string& test_name, const renderer2d::SurfaceRGBA& surface) {
        std::string current_hash = hash_surface(surface);
        std::string golden_path = "tests/golden/" + test_name + ".hash";

        std::ifstream infile(golden_path);
        if (!infile.is_open()) {
            std::cout << "[GOLDEN] Creating new baseline for: " << test_name << " (" << current_hash << ")" << std::endl;
            std::ofstream outfile(golden_path);
            outfile << current_hash;
            return true;
        }

        std::string golden_hash;
        infile >> golden_hash;
        if (current_hash == golden_hash) {
            return true;
        }

        std::cerr << "[GOLDEN] MISMATCH in " << test_name << "!" << std::endl;
        std::cerr << "  Expected: " << golden_hash << std::endl;
        std::cerr << "  Actual:   " << current_hash << std::endl;
        return false;
    }
};

} // namespace tachyon::test
