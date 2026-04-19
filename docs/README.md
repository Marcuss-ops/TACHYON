# Documentation Index

This index is the starting point for navigating TACHYON's architecture and system design documents.

The repository currently contains both older flat documents under `docs/` and a newer grouped structure under subfolders.
This mixed structure is intentional during the transition.

## How to read the docs

Recommended reading order for a new contributor:

1. `vision.md`
2. `non-goals.md`
3. `architecture.md`
4. `roadmap.md`
5. `mvp-v1.md`
6. grouped system contracts below

## Grouped documentation

### Contracts
- `contracts/core-contracts.md`

### Render and compositing
- `render/surface-and-aov-contract.md`

### Runtime and execution policy
- `runtime/memory-and-resource-policy.md`
- `runtime/quality-tiers.md`

### Output and delivery
- `output/output-profile-schema.md`
- `encoder-output.md`
- `decode-encode.md`
- `render-job.md`

### Interfaces
- `interfaces/cli-and-api-contract.md`

### Observability
- `observability/diagnostics-and-profiling.md`

### Testing
- `testing/canonical-scenes.md`
- `testing-and-compatibility.md`

## Migration note

During the reorganization phase:

- existing flat documents remain valid
- new contract-heavy documents should prefer the grouped structure
- future documents should be added to a subfolder unless they are broad top-level project docs

## Folder intent

### `docs/contracts/`
Cross-cutting rules the whole engine must obey.

### `docs/render/`
Render surfaces, pass semantics, compositing handoffs, and backend-facing rendering contracts.

### `docs/runtime/`
Execution, memory, quality, fallback, and operational behavior.

### `docs/output/`
Render-job output profiles, delivery targets, and encoding-adjacent schemas.

### `docs/interfaces/`
CLI and service-facing contracts.

### `docs/observability/`
Logs, metrics, profiling, debugging, and diagnostics.

### `docs/testing/`
Golden scenes, validation packs, and compatibility-oriented checks.
