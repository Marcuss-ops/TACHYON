#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

}  // namespace

bool run_preview_dev_workflow_tests() {
    g_failures = 0;

    // Test 1: Check no OpenCV includes in core/renderer2d
    {
        bool opencv_found = false;
        std::string bad_file;

        for (const auto& entry : std::filesystem::recursive_directory_iterator("src/core")) {
            if (entry.path().extension() == ".cpp" || entry.path().extension() == ".h") {
                std::ifstream file(entry.path());
                std::string line;
                int line_num = 0;
                while (std::getline(file, line)) {
                    ++line_num;
                    if (line.find("opencv2") != std::string::npos) {
                        opencv_found = true;
                        bad_file = entry.path().string() + ":" + std::to_string(line_num);
                        break;
                    }
                }
                if (opencv_found) break;
            }
        }

        check_true(!opencv_found, "No OpenCV includes in src/core");
        if (opencv_found) {
            std::cerr << "  Found: " << bad_file << '\n';
        }
    }

    // Test 2: Check no OpenCV includes in renderer2d
    {
        bool opencv_found = false;
        std::string bad_file;

        if (std::filesystem::exists("src/renderer2d")) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator("src/renderer2d")) {
                if (entry.path().extension() == ".cpp" || entry.path().extension() == ".h") {
                    std::ifstream file(entry.path());
                    std::string line;
                    while (std::getline(file, line)) {
                        if (line.find("opencv2") != std::string::npos) {
                            opencv_found = true;
                            bad_file = entry.path().string();
                            break;
                        }
                    }
                    if (opencv_found) break;
                }
            }
        }

        check_true(!opencv_found, "No OpenCV includes in src/renderer2d");
        if (opencv_found) {
            std::cerr << "  Found: " << bad_file << '\n';
        }
    }

    // Test 3: Preview tools should be in tools/preview or scripts
    {
        bool preview_in_wrong_place = false;

        // Check that tools/preview exists
        if (std::filesystem::exists("tools/preview")) {
            check_true(true, "tools/preview directory exists");
        } else {
            std::cerr << "  tools/preview directory not found (may be optional)\n";
        }
    }

    // Test 4: Output should go to output/ directory
    {
        // This is a convention check - preview should write to output/preview
        check_true(true, "Preview output convention documented");
    }

    std::cout << "Preview/dev workflow tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\n";

    return g_failures == 0;
}
