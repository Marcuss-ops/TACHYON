#include "cli/support/cli_internal.h"
#include "cli/commands/command.h"
#include <vector>

namespace tachyon {

std::vector<CommandDescriptor> make_all_commands() {
    return {
        make_render_command(),
        make_preview_command(),
        make_preview_frame_command(),
        make_validate_command(),
        make_inspect_command(),
        make_version_command(),
        make_motion_map_command(),
        make_watch_command(),
        make_metrics_command(),
        make_doctor_command(),
        make_output_presets_command(),
        make_thumb_command(),
        make_probe_command(),
        make_concat_command(),
#ifdef TACHYON_ENABLE_TEXT
        make_fetch_fonts_command(),
#endif
    };
}

} // namespace tachyon
