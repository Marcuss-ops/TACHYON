#include "tachyon/core/cli_options.h"
#include <iostream>
#include <vector>
#include <cassert>

namespace tachyon::test {

bool run_cli_parser_tests() {
    std::cout << "[TEST] Running CLI Parser Tests...\n";

    {
        // Test basic render command
        char* argv[] = {(char*)"tachyon", (char*)"render", (char*)"--cpp", (char*)"scene.cpp", (char*)"--out", (char*)"video.mp4"};
        auto result = parse_cli_options(6, argv);
        assert(result.diagnostics.ok());
        assert(result.value->command == "render");
        assert(result.value->cpp_path == "scene.cpp");
        assert(result.value->render.output_override == "video.mp4");
    }

    {
        // Test preview with frame
        char* argv[] = {(char*)"tachyon", (char*)"preview", (char*)"--cpp", (char*)"scene.cpp", (char*)"--frame", (char*)"120"};
        auto result = parse_cli_options(6, argv);
        assert(result.diagnostics.ok());
        assert(result.value->command == "preview");
        assert(result.value->render.preview_frame_number == 120);
    }

    {
        // Test concat options
        char* argv[] = {(char*)"tachyon", (char*)"concat", (char*)"--inputs", (char*)"a.mp4,b.mp4", (char*)"--out", (char*)"c.mp4"};
        auto result = parse_cli_options(6, argv);
        assert(result.diagnostics.ok());
        assert(result.value->concat.inputs.size() == 2);
        assert(result.value->concat.output == "c.mp4");
    }

    {
        // Test probe JSON
        char* argv[] = {(char*)"tachyon", (char*)"probe", (char*)"--input", (char*)"test.mp4", (char*)"--json"};
        auto result = parse_cli_options(5, argv);
        assert(result.diagnostics.ok());
        assert(result.value->probe.input == "test.mp4");
        assert(result.value->probe.json_output == true);
    }

    std::cout << "[OK] CLI Parser Tests Passed.\n";
    return true;
}

} // namespace tachyon::test
