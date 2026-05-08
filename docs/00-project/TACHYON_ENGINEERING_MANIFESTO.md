---
Status: Canonical
Last reviewed: 2026-05-06
Owner: Core Team
---

# TACHYON ENGINEERING MANIFESTO

The best part is no part. The best process is no process.

## 1. THE ALGORITHM
1. Question every requirement. Each must have a person attached to it.
2. Delete the part or process. If you are not adding back at least 10% of what you deleted, you are not deleting enough.
3. Simplify or optimize. Never optimize something that should not exist.
4. Accelerate cycle time.
5. Automate.

## 2. ENGINEERING STANDARDS
- Memory Ownership: Use stack by default. Use unique_ptr for ownership. shared_ptr is a last resort.
- Hot Loops: Zero allocations (new/malloc) in the render loop. Use pre-allocated buffers.
- Data Layout: Keep related data contiguous (std::vector). Avoid deep inheritance and virtual functions in performance-critical paths.
- Determinism: Bit-exact rendering. Input A + State B must always produce Pixel C.
- Global State: Forbidden. Pass state explicitly via RenderContext.
- Singletons: Forbidden. They hide dependencies and break multithreading.
- Magic Numbers: Forbidden. All constants must be named and configurable.
- Error Handling: Fail fast. Never silence errors. A missing asset must trigger an immediate, loud failure or a deliberate fallback, never a silent black frame.
- Code Quality: If code requires documentation to be understood, the code is wrong. Rewrite for clarity.
- Modularization: Split files and functions by responsibility, not by arbitrary line counts. If a function does more than one thing, it is too long.
