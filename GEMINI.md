# Tachyon Foundational Mandates

These mandates define the architectural integrity of the Tachyon engine. All future development and agent interactions must adhere strictly to these principles.

## 1. Zero Waste Architecture
Tachyon is designed for the absolute limit of hardware performance.
*   **Memory Integrity:** Prefer **SOA (Structure of Arrays)** over AOS for all pixel-intensive structures. Ensure all buffers are **32-byte aligned** for AVX2 safety.
*   **Computational Flow:** Eliminate all branching in per-pixel loops. Use **SIMD Masks** and branchless logic.
*   **Algorithmic Efficiency:** Use **Dependency-Based Hashing** to skip rendering of unchanged layers.
*   **Zero Allocations:** No memory allocations (`new`, `malloc`, `std::vector::resize`) are allowed within the hot render path. All temporary surfaces must come from a pre-allocated **SurfacePool**.
*   **Saturation:** Use a **Work-Stealing Tiling** architecture to ensure 100% core saturation.

## 2. 2D Motion Graphics / Procedural Compositing Engine
Tachyon is not a manual editor; it is a **procedural director**.
*   **Declarative Intent:** API should focus on *what* to achieve (e.g., "dramatic reveal"), not *how* to transform layers.
*   **Visibility Contracts:** Every critical asset must have a contract (e.g., "must be readable", "cannot be occluded"). The engine is responsible for enforcing these via validation.
*   **Procedural Motion:** Composition and movement must be programmatically derived from scene semantics.
*   **Integrated Validation:** Every render output must be accompanied by a validation report confirming architectural and visual integrity.

## 3. Tooling & Engineering Standards
*   **Hot-Path I/O:** Never perform synchronous disk I/O (like writing PNGs) in the render loop. Use RAM buffers or async telemetry.
*   **SIMD First:** When implementing pixel kernels, use **C++ Intrinsics** (AVX2/FMA) as the default choice over scalar C++.
*   **Empirical Verification:** Performance improvements must be measured via benchmark labs (e.g., `tests/perf/`) before merging.
