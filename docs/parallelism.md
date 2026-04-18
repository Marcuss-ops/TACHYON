# Parallelism

## Purpose

This document defines the expected parallel execution strategy for TACHYON.
The engine should not assume that frames must always be rendered in sequential order.
If the scene model is deterministic and frame-local evaluation is well-defined, then frame and pass work can be distributed aggressively.

## Core principle

Rendering should scale with available compute resources.
For many workloads, frame generation should be parallelizable across time as long as the required inputs for each sampled time are explicitly known.

## Temporal parallelism

The engine should support the idea that frame 1, frame 10, frame 20, and frame 30 may be evaluated and rendered independently when no hidden state couples them.
This is one of the strongest advantages of a deterministic programmatic renderer.

## Levels of parallel work

### 1. Frame-level parallelism
Multiple frames may be evaluated and rendered at the same time.

### 2. Pass-level parallelism
Independent render passes or cacheable subgraphs may be processed concurrently when dependencies allow it.

### 3. Subsystem-level parallelism
Asset preparation, property evaluation, effect work, and compositing preparation may be overlapped where safe.

## Architectural requirements

To support parallel execution correctly, the engine must preserve:

- explicit dependencies
- deterministic evaluation
- no hidden global state in render work
- stable cache keys
- allocator and buffer strategies that do not rely on unsafe implicit sharing

## Design rules

1. Sequential evaluation must not be assumed unless a subsystem explicitly requires it.
2. Parallelism must not change rendered output.
3. Work scheduling must respect graph dependencies, not authoring order alone.
4. The engine should favor explicit work graphs over implicit execution chains.

## Guiding principle

TACHYON should be architected so that render time trends toward available compute throughput, not toward video duration alone.
