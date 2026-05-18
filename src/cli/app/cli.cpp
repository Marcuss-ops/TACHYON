#include "tachyon/core/cli.h"
#include "cli_app.h"

namespace tachyon {

struct CliRegister {
    CliRegister() {
        register_cli_entrypoint(&run_cli);
    }
};
static CliRegister g_cli_register;

int run_cli(int argc, char** argv) {
    CliApp app;
    return app.run(argc, argv);
}

} // namespace tachyon
