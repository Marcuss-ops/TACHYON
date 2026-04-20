# Task: Milestone 5+ Implementation

## Phase 1: Quick Fixes & Specs
- [x] Implement `Emission` in `RayTracer::render`
- [x] Add `mesh_path` to `LayerSpec` (`scene_spec.h`)
- [x] Parse `mesh_path` in `scene_spec_json.cpp`
- [x] Add `mesh_asset` and `texture_rgba` to `EvaluatedLayerState` (`evaluated_state.h`)
- [x] Evaluate `mesh_asset` in `evaluator.cpp`

## Phase 2: glTF Loading
- [x] Add `tinygltf` to `CMakeLists.txt`
- [x] Create `MeshAsset` struct (`mesh_asset.h`)
- [x] Implement `MeshLoader` (`mesh_loader.cpp`)
- [x] Integrate `MeshLoader` into `AssetResolver`

## Phase 3: Textures & OIDN
- [x] Add `OIDN` to `CMakeLists.txt`
- [x] Implement UV sampling in `RayTracer`
- [x] Integrate `OIDN` filter in `frame_executor.cpp`

## Phase 4: Shading & Reflections
- [x] Add single-bounce recursive reflections in `RayTracer`
- [x] Implement basic Environment IBL (sky gradient fallback)

## Phase 5: Verification
- [ ] Render glTF test scene
- [ ] Verify emission & bloom-like behavior (Hdr -> ToneMap)
- [ ] Verify denoiser efficiency
