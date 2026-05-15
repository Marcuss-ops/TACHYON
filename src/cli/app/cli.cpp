#include "tachyon/core/cli.h"
#include "cli_app.h"

namespace tachyon {

int run_cli(int argc, char** argv) {
    CliApp app;
    return app.run(argc, argv);
}

} // namespace tachyon
