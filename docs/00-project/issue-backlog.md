# Pre-Scale Issue Backlog

> Status: Canonical working backlog
> Last reviewed: 2026-05-06

This file is the local source of truth for debt that must be tracked before scaling.
Use it to mirror GitHub issues until the tracker is the authoritative view again.

## P0

| Area | Current state | Next action |
|---|---|---|
| CI coverage | Smoke/core/render lanes exist, but new debt should be guarded automatically | Keep the disabled-test check in CI and expand coverage when new labels land |
| Disabled tests | Stale documentation exists for a few tests; source-of-truth is still split | Reconcile README, runner registration, and build graph on the next cleanup pass |
| Roadmap / MVP | Baseline now distinguishes production, experimental, stub, and out-of-scope work | Keep v1 scope frozen until a release baseline is cut |
| Dependency pins | FetchContent versions were scattered | Keep dependency pins centralized in `cmake/deps/DependencyVersions.cmake` |

## P1

| Area | Current state | Next action |
|---|---|---|
| Target graph | Large target graph remains coupled | Add enforcement docs and a target dependency policy check |
| Render session | Orchestration is still concentrated | Continue splitting context/sink/audio/cleanup responsibilities |
| Asset resolution | Resolver now gets better defaults, but still needs a full contract | Define allowed roots and fallback behavior per scene/job |
| Default camera | Hardcoded compatibility heuristics remain | Lift them into an explicit policy object and golden tests |
| Determinism | Good coverage exists, but not all contract edges are encoded | Add stable order and cache-key completeness checks |

