# Core Engine Module

Central engine components for the Tachyon rendering system.

## Directory Structure

```
src/core/
├── camera/          # Camera state and shake simulation
├── cli/             # Command-line interface implementations
├── expressions/     # Expression engine and VM
├── math/            # Matrix, quaternion, transform math
├── properties/      # Property interpolation and keyframes
├── scene/           # Scene evaluation and rendering
│   ├── evaluator/   # Layer and property evaluation
│   ├── rigging/     # IK solvers and constraints
│   └── state/       # Evaluated state containers
└── spec/            # Scene specification and compilation
    ├── compilation/  # Scene and preset compilers
    ├── schema/       # Scene JSON schema definitions
    ├── shapes/       # Shape specifications
    └── validation/   # Scene validation logic
```

## Key Components

- **c_api.cpp**: Public C API for the engine
- **cli.cpp**: Main CLI entry point and command routing
- **options.cpp**: Global engine options and configuration
- **report.cpp**: Reporting and diagnostics

## Submodules

### CLI Commands (`cli/`)
- `cli_render.cpp`: Render command implementation
- `cli_demo.cpp`: Demo/preview command
- `cli_validate.cpp`: Scene validation command
- `cli_inspect.cpp`: Scene inspection
- `cli_watch.cpp`: File watching for hot-reload
- `cli_transition.cpp`: Transition effects command

### Scene Evaluation (`scene/`)
- `evaluator.cpp`: Main scene evaluation orchestrator
- `evaluator_composition.cpp`: Composition-level evaluation
- `evaluator_math.cpp`: Evaluation math helpers

### Expressions (`expressions/`)
- `expression_engine.cpp`: Expression parsing and evaluation
- `expression_vm.cpp`: Stack-based expression VM

### Specification (`spec/`)
- `scene_compiler.cpp`: Compiles scene specs to runtime format
- `scene_spec_serialize.cpp`: JSON serialization
- `scene_spec_parse.cpp`: JSON parsing
- `layer_parse_json.cpp`: Layer-specific JSON parsing
