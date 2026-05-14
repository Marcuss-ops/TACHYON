#include "command_registry.h"
#include "tachyon/core/core.h"
#include <algorithm>
#include <iostream>

namespace tachyon {

// Forward declarations for core commands
bool run_validate_command(const CommandContext& context);
bool run_inspect_command(const CommandContext& context);
bool run_motion_map_command(const CommandContext& context);
bool run_render_command(const CommandContext& context);
bool run_output_presets_command(const CommandContext& context);
bool run_preview_command(const CommandContext& context);
bool run_preview_frame_command(const CommandContext& context);
bool run_watch_command(const CommandContext& context);
bool run_thumb_command(const CommandContext& context);

CommandRegistry& CommandRegistry::get_instance() {
    static CommandRegistry instance = create_default_command_registry();
    return instance;
}

void CommandRegistry::register_command(CommandDescriptor descriptor) {
    commands_.push_back(std::move(descriptor));
}

CommandDispatchResult CommandRegistry::dispatch(const CommandContext& context) const {
    for (const auto& cmd : commands_) {
        if (context.options.command != cmd.name) continue;
        if (cmd.validate && !cmd.validate(context)) return CommandDispatchResult::ValidationError;
        return cmd.run(context) ? CommandDispatchResult::Success : CommandDispatchResult::ExecutionError;
    }
    return CommandDispatchResult::NotFound;
}

void CommandRegistry::print_help(std::ostream& out) const {
    out << "TACHYON " << version_string() << "\n";
    out << "Usage:\n";
    out << "  tachyon version\n";
    for (const auto& cmd : commands_) {
        out << "  " << cmd.usage << "\n";
    }
}

CommandRegistry create_default_command_registry() {
    CommandRegistry registry;

    registry.register_command({
        "validate", "tachyon validate --cpp <scene.cpp> [--preset <id>]",
        [](const CommandContext& ctx) { return !ctx.options.cpp_path.empty() || ctx.options.preset_id.has_value(); },
        run_validate_command
    });

    registry.register_command({
        "render", "tachyon render --cpp <scene.cpp> --out <file> [--frames <s-e>]",
        [](const CommandContext& ctx) { return !ctx.options.cpp_path.empty() || ctx.options.preset_id.has_value(); },
        run_render_command
    });

    registry.register_command({
        "preview", "tachyon preview --cpp <scene.cpp> [--out <file.png>]",
        nullptr, run_preview_command
    });

    registry.register_command({
        "preview-frame", "tachyon preview-frame --cpp <scene.cpp> --frame <n> --out <file.png>",
        nullptr, run_preview_frame_command
    });

    registry.register_command({
        "watch", "tachyon watch --cpp <scene.cpp>",
        nullptr, run_watch_command
    });

    registry.register_command({
        "inspect", "tachyon inspect --cpp <scene.cpp> [--json]",
        nullptr, run_inspect_command
    });

    registry.register_command({
        "motion-map", "tachyon motion-map --cpp <scene.cpp>",
        nullptr, run_motion_map_command
    });

    registry.register_command({
        "thumb", "tachyon thumb --cpp <scene.cpp> --out <file.jpg>",
        nullptr, run_thumb_command
    });

    registry.register_command({
        "output-presets", "tachyon output-presets list/info",
        nullptr, run_output_presets_command
    });

    return registry;
}

} // namespace tachyon
