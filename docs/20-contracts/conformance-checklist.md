# Team Conformance Checklist (TACHYON v1)

Any new feature, backend, or effect must pass this checklist to be merged into `main`.

## Coordinate System & Space
- [ ] Uses Right-Handed coordinates (Y-up).
- [ ] Camera forward is precisely -Z.
- [ ] 2D layers use Top-Left origin (Composition Space).
- [ ] Transforms are applied in the order: Anchor -> Scale -> Rotate (YXZ) -> Position.

## Time & Units
- [ ] Time is handled in double-precision seconds.
- [ ] No frame-snapping occurs in internal property evaluation.
- [ ] Rational frame rate is used for final output stepping.

## Alpha & Color
- [ ] Internal buffers use **Premultiplied Alpha**.
- [ ] Compositing math performs `SrcOver` correctly (Color_dest * (1-Alpha_src) + Color_src).
- [ ] All math occurs in **Linear-Light** space (Rec.709 primaries).
- [ ] FP32 precision is used in all intermediate steps.

## Determinism
- [ ] Every stochastic process (noise, sampling) uses the `JobSeed`.
- [ ] Seeds are combined via the standard hashing policy (`MurmurHash3`).
- [ ] No `rand()`, `std::mt19937` with time seed, or unseeded noise.

## Diagnostics
- [ ] Failures emit a stable Diagnostic Code (`TY-xxxx`).
- [ ] Logs follow the structured format (Subsystem, Severity, Code, Message).
- [ ] Validation errors point back to the JSON SourcePath.

## Handoff
- [ ] Passes are named correctly (`beauty`, `alpha`, `depth`, `normal`, `albedo`).
- [ ] 3D passes hand off AOVs compatible with the 2D compositor's linear-light contract.
- [ ] Metadata (Frame Index, Time) is preserved during handoff.

---

> [!CAUTION]
> Deviating from these contracts without an architectural override will lead to deterministic drift and "Group A" veto.
