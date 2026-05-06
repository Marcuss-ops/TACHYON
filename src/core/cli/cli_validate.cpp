#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/report.h"
#include "tachyon/runtime/runtime_facade.h"
#include "cli_internal.h"
#include <iostream>

namespace tachyon {

bool run_validate_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    ValidateRequest request;
    request.cpp_path = options.cpp_path;
    request.preset_id = options.preset_id;

    auto result = RuntimeFacade::instance().validate(request);

    if (!result.valid) {
        err << "Validation FAILED\n";
        for (const auto& error : result.errors) {
            err << "[ERROR] " << error << "\n";
        }
        for (const auto& warn : result.warnings) {
            err << "[WARNING] " << warn << "\n";
        }
        return false;
    }

    out << "scene spec valid\n";
    if (!result.warnings.empty()) {
        out << "warnings: " << result.warnings.size() << "\n";
        for (const auto& warn : result.warnings) {
            out << "[WARNING] " << warn << "\n";
        }
    }
    return true;
}

} // namespace tachyon
