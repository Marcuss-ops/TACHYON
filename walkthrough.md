# Tachyon Engine Architecture Maturation

This document summarizes the current state of the Tachyon engine's core subsystems after the recent modularization work. The codebase is significantly more structured, but several text and rigging algorithms remain heuristic or intentionally simplified.

## Core Accomplishments

### 1. Rigging: Structured Evaluation Pipeline
- **Phase-Separated Evaluation**: The rigging pipeline is split into three phases:
  1. **FK Pass**: Computes initial world matrices from local TRS values.
  2. **Constraint Pass**: Resolves Point, Orient, Parent, and LookAt constraints in graph order.
  3. **IK Pass**: Applies CCD-based solving for configured chains.
- **Structural Hierarchy**: `Pose` now keeps parent and child indices, which removes linear parent lookup during hierarchy traversal.
- **Propagation**: World matrices are propagated through the hierarchy with explicit parent-child updates.
- **Verification**: Covered by `rigging_maturation_tests.cpp`.

### 2. Text: Unicode-Oriented Editing Foundation
- **Cluster Segmentation**: `ClusterIterator` handles combining marks, ZWJ sequences, and regional indicator pairs with simplified UAX#29-style rules.
- **Word Navigation**: `TextEditor` includes word-left, word-right, and select-word behavior using a Unicode-oriented heuristic instead of byte-based offsets.
- **IME Preview**: `get_display_text()` exposes composition text for visual IME feedback without committing it to the document until `commit_composition()`.
- **Editor Integration**: Layout hit-testing and cluster-aware selection are wired into the editor path.
- **Verification**: Covered by `editor_tests.cpp`.

### 3. PBR: Explicit Shading Contract
- **Material Evaluation**: `MaterialSample` now precomputes `diffuse_color` and `specular_color`, making the shading contract clearer.
- **Disney Lobe Mapping**: Material parameters such as subsurface, sheen, clearcoat, and specular tint are routed into the renderer-facing sample structure.
- **Renderer Integration**: The 3D shading path consumes the evaluated material sample rather than reconstructing material state ad hoc.

## Remaining Work
- **Rigging Depth**: IK solving, cycle handling, and pose evaluation can still be strengthened for large production graphs.
- **Text Compliance**: BiDi, line breaking, and grapheme segmentation remain simplified and should be refined toward fuller Unicode compliance.
- **Motion Blur**: Subframe evaluation is still limited compared with a full scene re-evaluation per sample.
