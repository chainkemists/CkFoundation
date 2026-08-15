# Phase 2 — updatable heightfield shape core (no ECS, no world)

**Goal:** the shape-level mechanics of requirements 4–6: an updatable variant of
`CreateHeightFieldShape` that exposes the inner `JPH::HeightFieldShape` and takes a deformation
envelope, a **pure** region-plan function (UE-rect → Jolt-rect, row flip, block alignment), and
an apply helper that performs the expand-and-overlay `SetHeights`. Resolves the mechanics half of
decision D3 (alignment = expand-and-overlay; envelope validated by us, never left to Jolt's
silent clamp). Everything here is plain C++ in `ck::jolt::bake` — testable with
`Request_GlobalJoltInit` alone, exactly like the existing
`Ck.Jolt.Bake.HeightField.KnownHeightsZUp` test.

## Entry criteria

1. Phase 1 exit criteria hold (or Phase 1 was skipped-by-decision — this phase does not depend
   on it; record which).
2. Baseline re-run recorded (the numbers this phase's "no regressions" is measured against).
3. Symbol re-verification:
   - `rg -n "CreateHeightFieldShape" Source/CkJolt` → declaration in `CkJoltBakeExtraction.h`,
     definition + the row-flip/`RotatedTranslatedShape` wrap in the .cpp.
   - `rg -n "mMinHeightValue|mMaxHeightValue" Source/CkThirdParty -g HeightFieldShape.h` → both
     present on `HeightFieldShapeSettings`.
   - `rg -n "void\s+SetHeights|void\s+GetHeights" Source/CkThirdParty -g HeightFieldShape.h` →
     both present, block-aligned-rect contract in their doc comments.

## The grid mapping (derived once here — do not re-derive)

Creation (existing code, unchanged behavior): UE sample `(x, y)` with height `h` lands at world
`(x*sx, y*sy, h)` via: Jolt local grid `(x, jrow)` where **`jrow = N-1-y`** (row flip), local
Y-up, `+90°` about X wrap, local-Z offset `-(N-1)*sy`. `mScale.Y = 1.0`, heights in world units —
so Jolt's encodable range (`GetMinHeightValue/GetMaxHeightValue`) is directly in world height
units, and `SetHeights` consumes world heights directly.

Region update mapping, for a UE rect `[UeX, UeX+SizeX) x [UeY, UeY+SizeY)` on a logical `N x N`
grid (`B` = block size, `M` = the created shape's `GetSampleCount()` — Jolt rounds `N` up to a
block multiple, padding samples are no-collision):

- X passes through: Jolt X-range = `[UeX, UeX+SizeX)`.
- Row flip: Jolt Y-range = `[N-UeY-SizeY, N-UeY)`.
- Alignment (outward): `JoltX = RoundDown(UeX, B)`, right = `RoundUp(UeX+SizeX, B)`;
  `JoltY = RoundDown(N-UeY-SizeY, B)`, top = `RoundUp(N-UeY, B)`. All results are within
  `[0, M]` because `M` is the rounded-up block multiple of `N` — assert this in the
  implementation, never clamp silently.
- Overlay: `GetHeights` the aligned rect (returns current heights, holes as `cNoCollisionValue`),
  copy the caller's UE-row-major values into it with the row flip applied
  (`AlignedRow = (N-1-UeRow) - JoltY`), `SetHeights` the aligned rect.

## Step 1 — the executable spec (commit red first)

Create `Plugins/CkTests/Source/CkTests/Private/UnitTests/CkJolt/Test_JoltHeightField_RegionUpdate.cpp`
mimicking `Test_JoltBake_HeightField_KnownHeightsZUp.cpp` (same flags, same `Request_GlobalJoltInit`
+ `ON_SCOPE_EXIT` shutdown, same `CastDownAt` helper — copy it into the new file's namespace).
Four tests:

```cpp
// "Ck.Jolt.HeightField.RegionPlan.MappingAndAlignment"  (pure fn — no Jolt init needed)
//   Cases (N=8, B=2, M=8 unless stated):
//     - full surface (0,0,8,8)          -> jolt (0,0,8,8)
//     - already aligned (2,2,2,2)       -> jolt (2,4,2,2)   [flip: rows 2..3 -> 4..5]
//     - unaligned (1,1,3,3)             -> jolt rect that contains flipped rows [4,7) and
//                                          cols [1,4), rounded outward to B: (0,4,4,4)
//     - odd logical N=9 (M=10, B=2): rect touching UE row 0 -> aligned top edge 10 (padding row)
//     - out of bounds (7,7,3,3) on N=8  -> unset
//     - zero-size                        -> unset
//   EXPECT: exact struct values (compute by hand from the mapping above and hard-code).

// "Ck.Jolt.HeightField.Update.RegionEditRoundTrip"
//   - Updatable shape from the h(x,y) = 10x + 100y surface (SampleCount 8, ScaleXY 100),
//     envelope Explicit {-500, +1500}.
//   - Apply a 3x3 update at UE (2,2) writing flat height 900 (within envelope, above the
//     original surface) via ApplyHeightFieldRegionUpdate -> EXPECT Applied.
//   - CastDownAt inside the region (e.g. world 250,250) -> ~900 (tolerance 5, quantization).
//   - CastDownAt outside (600,600) -> unchanged expected value from the h() formula.
//   - Apply a dig to -300 (below original min, inside envelope) -> Applied, probe confirms.

// "Ck.Jolt.HeightField.Update.HolePunchAndSurvival"
//   - Same base shape; punch a hole (HeightFieldNoCollisionValue()) at UE (5,5) at CREATION.
//   - Apply an update whose ALIGNED rect covers (5,5) but whose caller rect does not
//     -> EXPECT Applied AND the hole still misses (pins the GetHeights overlay preserving
//     holes in border cells).
//   - Apply an update whose caller rect WRITES a hole at (2,2) -> ray at (2,2) misses, neighbor
//     still hits (holes are legal incoming values, not envelope violations).

// "Ck.Jolt.HeightField.Update.EnvelopeExceededRejected"
//   - Envelope FromSamples (no explicit widening): apply an update with a height above the
//     original max -> EXPECT OutOfEnvelope, and a probe confirms the shape is UNCHANGED.
//   - Out-of-bounds rect -> EXPECT OutOfBounds, shape unchanged.
```

TempAllocator for the tests: a local `JPH::TempAllocatorImpl TempAllocator{4 * 1024 * 1024};`
(mimic how vendored Jolt tests do it; global Jolt init is already up).

**Decision gate G1 (red):** expected — the file does not compile (the three new symbols do not
exist). Record. Then implement Step 2 and re-run; every test must go green with zero edits to the
test's expected values. If an expected value proves wrong, the MAPPING is wrong — stop and
re-derive against the existing KnownHeightsZUp test before touching expectations.

## Step 2 — implement (all in `CkJoltBakeExtraction.h/.cpp`)

Add to `ck::jolt::bake` (header additions below the existing `CreateHeightFieldShape`; keep
Jolt-type usage consistent with the header's existing `JPH::Ref<JPH::Shape>` precedent —
forward-include `HeightFieldShape.h` in the header is acceptable here because the header already
includes `Jolt/Physics/Collision/Shape/Shape.h`):

```cpp
/// The axis-corrected wrapper to put on a body, plus the INNER heightfield the region-update
/// path edits in place. The two reference the same storage — SetHeights on _HeightField is
/// visible through _Shape immediately.
struct CKJOLT_API FCk_Jolt_UpdatableHeightField
{
    JPH::Ref<JPH::Shape> _Shape;
    JPH::Ref<JPH::HeightFieldShape> _HeightField;
};

/// InDeformationEnvelope widens the encodable height range beyond the initial samples so later
/// region updates can dig/pile past them (maps to HeightFieldShapeSettings::mMin/MaxHeightValue;
/// Jolt ignores whichever bound the initial samples already exceed). Unset = FromSamples: the
/// range is exactly the initial samples' min/max, and updates outside it are rejected.
CKJOLT_API auto CreateHeightFieldShape_Updatable(
    const TArray<float>& InWorldHeights,
    int32 InSampleCount,
    const FVector2D& InScaleXY,
    const TOptional<FFloatInterval>& InDeformationEnvelope) -> FCk_Jolt_UpdatableHeightField;

/// Pure UE-rect -> Jolt-rect planning (row flip + outward block alignment). Unset = rect invalid
/// (out of bounds or zero-size) — the CALLER diagnoses; this function is a testable primitive.
struct CKJOLT_API FCk_Jolt_HeightFieldRegionPlan
{
    int32 _JoltX = 0;
    int32 _JoltY = 0;
    int32 _SizeX = 0;
    int32 _SizeY = 0;
};

CKJOLT_API auto ComputeHeightFieldRegionPlan(
    int32 InLogicalSampleCount,
    int32 InShapeSampleCount,
    int32 InBlockSize,
    int32 InUeX,
    int32 InUeY,
    int32 InUeSizeX,
    int32 InUeSizeY) -> TOptional<FCk_Jolt_HeightFieldRegionPlan>;

enum class ECk_Jolt_HeightFieldRegionUpdateResult : uint8
{
    Applied,
    OutOfBounds,
    OutOfEnvelope
};

/// Expand-and-overlay region edit: plans the aligned rect, GetHeights it, overlays the caller's
/// UE-row-major values (row flip applied; HeightFieldNoCollisionValue is a legal incoming value
/// and is exempt from envelope validation), validates every remaining value against the shape's
/// encodable range BEFORE SetHeights (Jolt clamps silently — this helper must never let it),
/// then SetHeights. Reports, never ensures: the public boundary (Phase 3) owns the loud
/// diagnosis, tests probe rejection without expected-error scaffolding.
CKJOLT_API auto ApplyHeightFieldRegionUpdate(
    JPH::HeightFieldShape& InOutHeightField,
    int32 InLogicalSampleCount,
    int32 InUeX,
    int32 InUeY,
    int32 InUeSizeX,
    int32 InUeSizeY,
    const TArray<float>& InWorldHeights,
    JPH::TempAllocator& InTempAllocator) -> ECk_Jolt_HeightFieldRegionUpdateResult;
```

Implementation notes (load-bearing):
- **Refactor, single source of truth:** the existing `CreateHeightFieldShape` becomes a thin
  wrapper over `CreateHeightFieldShape_Updatable` (unset envelope, discard `_HeightField`). The
  existing `Ck.Jolt.Bake.HeightField.KnownHeightsZUp` test is the behavioral pin — it must stay
  green untouched.
- The inner ref: `Create_ShapeFromSettings` returns `JPH::Ref<JPH::Shape>`; the heightfield
  settings' created shape IS a `HeightFieldShape` — hold it as
  `JPH::Ref<JPH::HeightFieldShape>` via `static_cast` on the known-type pointer (mirrors how the
  existing code passes `HeightFieldShape.GetPtr()` into the wrapper settings).
- Store nothing: these are free functions; state (encodable range, sample counts) is the Phase-3
  fragment's job. `ApplyHeightFieldRegionUpdate` reads the range from the shape itself
  (`GetMinHeightValue()/GetMaxHeightValue()`).
- Envelope in `Updatable`: heights are stored with `mScale.Y = 1.0`, so envelope values are world
  heights verbatim — set `Settings.mMinHeightValue/mMaxHeightValue` from the interval. Validate
  `Min < Max` with `CK_ENSURE_IF_NOT` (creation IS a public boundary; return `{}`).
- `SetHeights` stride: pass the aligned-rect row stride (`Plan._SizeX`); heights buffer is the
  overlaid aligned rect in Jolt row order.

## Step 3 — gate

Build; run `Automation RunTests Ck.Jolt` (full suite).
- **Expected:** baseline + Phase-1 greens unchanged; 4 new `Ck.Jolt.HeightField.*` greens;
  `Ck.Jolt.Bake.HeightField.KnownHeightsZUp` still green (the refactor pin).
- **If KnownHeightsZUp flips red** → the refactor changed creation behavior; diff
  `CreateHeightFieldShape`'s output path against git HEAD before anything else. STOP if not
  obvious.
- **If RegionEditRoundTrip is off by ~quantization** at exactly the block borders → expected
  precision loss documented by Jolt's SetHeights (border recompression); widen only THAT
  assertion's tolerance to 10 and note it in a comment referencing Jolt's doc line. Off by a
  cell/row → mapping bug, do not touch tolerances.

## Exit criteria

- `Ck.Jolt` suite: prior verdicts + 4 new greens, recorded as a delta line.
- `rg -n "CreateHeightFieldShape_Updatable|ComputeHeightFieldRegionPlan|ApplyHeightFieldRegionUpdate" Source/CkJolt`
  → header + cpp hits for all three; no other files.
- Existing `CreateHeightFieldShape` body is now a delegation (verify by reading it, not by test
  count alone).
- Diff touches ONLY `CkJoltBakeExtraction.h/.cpp` + the new test file. Comment audit done.

## Fences

- Do NOT change `CreateHeightFieldShape`'s signature or observable behavior — it is pinned.
- Do NOT put the row-flip math anywhere except `ComputeHeightFieldRegionPlan` + the overlay copy
  — two flips composed (or zero) reads mirrored terrain that LOOKS plausible on symmetric test
  data; the asymmetric h(x,y) surface in the tests exists precisely to catch this.
- Do NOT let `SetHeights` see values you did not validate (silent clamp — D3).
- Do NOT reject unaligned rects (alignment is expand-and-overlay by decision D3) and do NOT
  expand without the `GetHeights` overlay (writing zeros into border cells is silent corruption).
- Do NOT add ensures inside `ApplyHeightFieldRegionUpdate`/`ComputeHeightFieldRegionPlan` — the
  loud boundary is Phase 3's processor and bake call (mirrors the Cast/CastChecked split).
- Do NOT touch the ECS, the subsystem, or any reflected type in this phase.

## Risks + rollback

- **Risk:** vendored Jolt's sample-count rounding behaves differently than assumed for odd N
  (the plan derives aligned edges may land in padding rows). The RegionPlan odd-N test case
  exists to catch this at the pure-function level; if Jolt rejects the padded rect at
  `SetHeights` (assert), record actual behavior and clamp the plan to `M` — a one-function
  change, test expectations updated with a comment explaining the measured behavior.
- **Whole-phase rollback:** revert the commit. Pure additive C++ API + one delegation refactor;
  nothing else references the new symbols until Phase 3.
