# PHASE 2 — Camera texel-snap, render margin, sub-texel remainder

> Entry: Phase 1 exit criteria met. The heart of the technique — this phase kills pixel creep.
> All math is specified in [RESEARCH_Technique.md](RESEARCH_Technique.md) §A; do not re-derive.
> Test scenes still use `r.Ortho.Debug.ForceAllCamerasToOrtho 1` until Phase 3.

## Executable spec

C++ test `Test_PixelArtRender_SnapMath.cpp` (CkTests, written FIRST, red):

- `UCk_Utils_PixelArtRender_UE::Get_SnappedViewOrigin(const FVector& InOrigin, const FMatrix& InViewRotationMatrix, float InTexelWorldSize, FVector2f& OutRemainderTexels)`
  — property tests over 1000 seeded random (origin, rotation, texel-size) tuples:
  1. Idempotence: snapping a snapped origin is identity, remainder (0,0).
  2. Bound: `|OutRemainderTexels| ≤ 0.5 + KINDA_SMALL_NUMBER` per axis.
  3. Reconstruction: `Snapped + (RemainderTexels.X * Texel) * CamRight + (RemainderTexels.Y * Texel) * CamUp == InOrigin` within 1e-3.
  4. Forward invariance: `dot(Snapped - InOrigin, CamForward) == 0` within 1e-3.
- `Get_OrthoWidthFromProjection(const FMatrix& InProjection)` — for a constructed
  `FReversedZOrthoMatrix`, returns the source width within 1e-3 (the `2 / M[0][0]` extraction).

Toolbox: `--test --test-pattern PixelArtRender --discover-fresh`, red → green.

## Steps

1. **Pure math first** — implement the two utils above in `CkPixelArtRender_SnapMath.{h,cpp}`
   (free functions in `ck::pixel_art`, thin BPFL wrappers on `UCk_Utils_PixelArtRender_UE`).
   Basis extraction: derive `CamRight`/`CamUp`/`CamForward` from `InViewRotationMatrix` — UE's
   view rotation matrix axis-swizzles (world→view with Z forward). **Do not guess the rows**:
   validate inside the test by comparing against
   `FRotationMatrix(Rot).GetScaledAxis(EAxis::Y/Z/X)` for a known rotation, and lock in
   whichever extraction passes. Get the spec tests green BEFORE touching the SVE.
2. **Snap in `SetupViewProjectionMatrix(FSceneViewProjectionData&)`** (game thread), when the
   world is enabled + snap on + the projection is orthographic
   (`InOutProjectionData.ProjectionMatrix.M[3][3] >= 1.0f` — the engine's own ortho test,
   `SceneView.cpp:532`):
   - `OrthoWidth = Get_OrthoWidthFromProjection(...)`;
     `TexelWorld = OrthoWidth / InternalW_inner` (inner = without margin — see step 3 note).
   - Snap `ViewOrigin`; write remainder into a per-world transient slot on the state registry
     (`FCk_PixelArt_FrameTransients { FVector2f _RemainderTexels; uint64 _FrameNumber; }`).
     Same-frame ordering is guaranteed: `GetProjectionData` runs inside
     `UGameViewportClient::Draw` before `BeginRenderingViewFamily`
     (`GameViewportClient.cpp:1971`) — assert `_FrameNumber == GFrameCounter` when consumed.
3. **Margin fold** (same hook): widen the projection so the render covers
   `Internal + 2·Margin` texels while composition stays the inner window:
   `Scale = InnerW / RenderW`; `M[0][0] *= Scale; M[1][1] *= Scale;` then
   `View rect` request (D8 mechanism) targets `Internal + 2·Margin`. The upscaler's
   `InnerRectMinTexels = (Margin, Margin)`, `InnerRectSizeTexels = Inner`. Texel world size is
   IDENTICAL for inner and render windows by construction — the spec test for that is arithmetic
   in `Test_PixelArtRender_SnapMath` (add case 5: margin fold preserves texel size).
4. **Remainder → upscaler**: `BeginRenderViewFamily` reads the transient slot, bakes
   `SubTexelOffsetTexels` into the `new`'d upscaler. Sign convention gate below.
5. **Debug knobs** (all under `ck.PixelArt.Debug.*`, cheat-flagged): `SnapOnly` (snap without
   compensation — must visibly stutter), `CompSign` (+1/-1 — flips the remainder sign),
   `FreezeSnap` (hold last snapped origin).
6. **Gate 6.G — sign + behavior verification** `[EDITOR-VERIFY, standalone]`, with a scripted
   slow diagonal camera pan (add a console command `Ck_PixelArt_DebugPan` that drifts the view
   target at 0.2 texel/frame):
   - Expected with snap ON + comp ON (`CompSign` correct): geometry pixels do not crawl; motion
     is smooth.
   - With comp OFF (`SnapOnly`): visible whole-texel stepping (proves snap active).
   - With snap OFF entirely: visible creep (proves the baseline problem exists in the scene).
   - If smooth-but-creeping under snap+comp → the sign is wrong: flip `CompSign`, confirm, then
     make that sign the code default and delete the wrong branch.
   - Anything else (jitter worse than baseline, margin edge artifacts) → STOP, blocker with
     capture.
7. **Zoom policy**: recompute TexelWorld from the live projection every frame (it already falls
   out of step 2 — verify by animating ortho width via the debug pan command; expected:
   transition shimmer during zoom, stability at rest — this matches the documented technique
   limit, not a bug).
8. Commit; scoped suite + record.

## Exit criteria

- All SnapMath spec tests green (incl. margin case).
- Gate 6.G outcomes recorded in PROGRESS.md with the screenshot/capture paths (creep OFF-state
  evidence, stutter mid-state evidence, smooth final-state evidence).
- Margin verified: `Ck_PixelArt_DebugPan` at max remainder shows NO edge smear/garbage at any
  screen border (the +2 texel margin absorbs it).
- Full suite delta-zero vs baseline.

## Fences

- The snap NEVER applies to perspective projections — early-out, no ensure (perspective is a
  legitimate state; the pixel-art look is simply not stabilized there).
- Do not snap in ECS/CkCamera and do not add a `UCameraModifier` — D3 locked render-side
  (engine camera modifiers run before this hook; ECS snapping would be perturbed by shakes).
- Do not read `GFrameCounter` on the render thread for the transient handshake — both ends of
  it are game-thread hooks.
- `SetupViewPoint` is the WRONG hook (runs pre-camera-manager blending); do not move the snap
  there.
