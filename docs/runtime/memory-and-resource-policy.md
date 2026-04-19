# Memory and Resource Policy

## Purpose

This document defines how TACHYON should behave under memory, storage, and general resource pressure.

This matters because TACHYON explicitly targets CPU-first and low-cost environments where RAM and disk behavior often constrain real throughput more than raw arithmetic does.

## Core rule

Resource use should be bounded by policy, not by accident.

The engine should prefer:
- explicit budgets
- bounded queues
- visible fallback behavior
- reproducible failure when limits are exceeded

## Resource classes

The engine should reason about at least these classes:

- RAM used by scene state and evaluation caches
- RAM used by render surfaces and accumulation buffers
- RAM used by encode queues and output buffering
- temporary disk storage
- long-lived cache storage
- CPU worker concurrency

## Budget model

Recommended first budget model:

- global process memory budget
- renderer working-set budget
- output buffering budget
- temporary storage budget

These may later be derived from machine inspection, but the engine should support explicit job-level or runtime-level overrides.

## Budget states

The runtime should expose at least three operational states:

### Normal
The engine is operating within configured budgets.

### Pressure
The engine is still operating, but some fallback or throttling behavior has been activated.

### Exhausted
The engine cannot continue safely and must fail cleanly.

## Pressure responses

When entering pressure state, the engine should prefer deterministic operational responses such as:

- slowing producer stages through backpressure
- shrinking active queue depth
- dropping optional cache retention
- switching to lower-cost quality tier only if the active job explicitly allows it
- delaying speculative precomputation

The engine should not silently change visible output quality unless the selected policy explicitly permits that.

## Memory priorities

When memory must be reclaimed, suggested priority order is:

1. speculative or optional caches
2. inactive intermediate buffers
3. non-essential profiling retention
4. bounded queue contraction where safe
5. fail rather than corrupt or silently redefine output

## Disk and temp storage policy

The engine should treat disk as a constrained resource.

At minimum, the runtime should support:

- explicit temp directory selection
- temp storage budget or warning threshold
- cleanup rules for job success and failure
- visible distinction between cache storage and temporary encode intermediates

## Queue policy

No major pipeline queue should be unbounded.

This applies especially to:
- render to encode frame queues
- decode preload queues
- asynchronous effect work queues later if introduced

Every queue should declare:
- item type
- maximum length
- producer behavior when full
- consumer behavior when starved

## Surface allocation policy

Large render surfaces are one of the easiest ways to blow memory unexpectedly.

The engine should document:
- when surfaces are reused
- when surfaces are pooled
- when accumulation buffers are retained across steps
- what quality modes multiply surface memory cost

## Cache policy link

Resource policy and cache policy must remain compatible.

That means:
- caches may be dropped under pressure
- cache eviction should be explicit and observable
- pressure-triggered eviction must not violate determinism
- cache retention should not be mistaken for a correctness requirement

## Output interaction

The output subsystem should respect resource policy.

Examples:
- bounded frame queue to the encoder
- no forced image-sequence dump by default
- clean failure on disk-full conditions
- configurable queue size for slow disks and weak machines

## Quality-policy interaction

Resource policy may interact with quality tiers, but only through explicit rules.

Good examples:
- draft tier uses lower sample counts and smaller buffering budgets
- cinematic tier reserves higher accumulation or encode costs

Bad example:
- silently reducing motion blur or codec quality without the job requesting a lower tier

## Failure contract

When resources are exhausted, the engine should fail clearly.

At minimum, the failure should identify:
- which budget was exceeded
- which subsystem was active
- whether output may be partial or invalid
- whether retrying with a different quality tier or queue budget is likely to help

## Observability requirements

Resource policy should integrate with diagnostics.

Useful signals include:
- peak RSS or process memory estimate
- queue high-water marks
- temp storage consumed
- cache bytes retained and evicted
- pressure-state transitions
- encode backpressure duration

## First implementation recommendation

The first implementation should support:

- global memory budget
- bounded render to encode queue
- temp directory override
- temp storage cleanup rules
- cache eviction under pressure
- clear disk-full and memory-exhausted failures

## Guiding principle

TACHYON should behave predictably on weak machines.
A serious headless engine does not only render well when resources are abundant.
It also degrades or fails in disciplined, diagnosable ways when they are not.
