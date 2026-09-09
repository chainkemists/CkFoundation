# PHASE 1 — CkGroundNav module and the hermetic bake core

> Freshness: authored 2026-08-31 in planning session 1. Status of record is
> [PROGRESS.md](PROGRESS.md); the designs of record are [REPRESENTATION.md](REPRESENTATION.md)
> ([NN-D7]) and [FEATURE_MATRIX.md](FEATURE_MATRIX.md) F1.1–F1.12 + F1.43. Where this file and a
> design doc disagree, the design doc wins and the disagreement is a **STOP**.
>
> **Every code block in this file is illustrative — the executor refines it mechanically, does not
> redesign it.**

---

## 1. Goal

Stand up the `CkGroundNav` module and build the entire bake pipeline as **hermetic, ECS-free math**
— geometry collection through a physics-backend seam, span rasterization, walkability filtering,
vertical layer extraction, per-cell clearance, merged-plate decomposition, portal extraction, and
tiled immutable publish with epochs — landing **F1.1–F1.12** and **F1.43**.

No queries, no search, no ECS driver beyond the minimum needed to own a field. Every assertion in
this phase is a **count**: probe counts, cell counts, plate counts, portal counts, exact clearance
values. Vibes are not evidence.

---

## 2. Entry criteria

- [ ] **PHASE_0 closed in PROGRESS.md** with its gate evidence and the R3 disposition checklist at
      zero unresolved rows.
- [ ] Repo clean, on a `feature/` branch, all three submodules confirmed:
      ```powershell
      git -C "D:\Repos\CkPlugins_3" status --short --branch
      git -C "D:\Repos\CkPlugins_3\Plugins\CkFoundation" status --short --branch
      git -C "D:\Repos\CkPlugins_3\Plugins\CkTests" status --short --branch
      ```
- [ ] **The in-house patterns this phase mirrors still exist** (they are the design, not decoration):
      ```powershell
      rg --no-ignore -n "GeometryBackend" Plugins/CkFoundation/Source/CkVoxelNav/Public
      rg --no-ignore -n "_AggregatedChunkEpochSum|_Epoch" Plugins/CkFoundation/Source/CkVoxelNav/Public/CkVoxelNav/Volume/CkVoxelNavVolume_Fragment.h
      ```
      Zero hits on either ⇒ the R4 inventory has drifted ⇒ **STOP**.
- [ ] **Phase-entry baseline captured this session** (full suite, totals + failing-test names +
      artifact identity in PROGRESS.md):
      ```powershell
      Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --test `
        --parallel 1 --no-nullrhi --output=Saved/Logs/BuildTest.log --project="D:\Repos\CkPlugins_3"
      ```
      Pre-flight: the editor must be closed for this project (the toolbox exits **77** if it is
      open) — see the `build-test` skill's pre-flight table.
- [ ] Skills loaded: `ck-macros-and-codegen`, `ck-tests-authoring-and-running`, `build-test`,
      `ck-change-control`. Read `CkVoxelNav/CLAUDE.md` and one full feature quartet (`CkTimer`)
      before writing a line — mimicry of adjacent code beats invention (non-negotiable #1).

---

## 3. Standing decision gate (applies to EVERY `-> verify:` line in this file)

- Observation matches the stated expectation → continue.
- Compile/UHT/link failure → fix mechanically and re-verify. **Two failed attempts at the same
  step → STOP.**
- A hermetic test fails on a *number* (probe count, plate count, clearance value): the number in
  this file or in FEATURE_MATRIX.md is the specification. If the implementation cannot meet it,
  **the implementation is wrong, not the number** — do not relax the assertion. Two failed attempts
  → STOP.
- A number is not stated anywhere and the test needs one → it is a `[MEASURE at phase entry]` value:
  measure it, record it in PROGRESS.md and VALIDATION.md as a tracked number, and say in the record
  that you measured it. Never estimate one.
- A pre-existing red test from the baseline is still red with the same name → not yours; continue.
- A previously green test is now red → **STOP**, revert the offending step, diagnose, re-sequence.
- **Anything else → STOP, record it in PROGRESS.md § Blockers with verbatim evidence, end session.**

---

## 4. Sub-phase 1A — Module, math-core boundary, geometry backend seam (F1.43, F1.1)

**1A entry:** §2 all green.
**1A exit:** a `CkGroundNav` module exists; a bake driven entirely by a hand-authored box list runs
in a headless C++ test with no `UWorld`, no registry, no physics.

### Steps

1. Scaffold `Source/CkGroundNav/` by copying the smallest complete feature quartet (`CkTimer`) and
   renaming; add the module row to `CkFoundation.uplugin` (Runtime tier, `Default` loading phase)
   and to `Source/CLAUDE.md`'s tier table. Dependencies mirror CkVoxelNav's minus what this phase
   does not use: `Core, Ecs, EcsExt, Jolt, Label, Log, Record, Settings, Shapes, ThirdParty`.
   → verify:
   ```powershell
   Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --generate `
     --output=Saved/Logs/P1-1A-Generate.log --project="D:\Repos\CkPlugins_3"
   ```
   succeeds
   (`--generate` is warranted exactly here — a new module changed the source layout — and nowhere
   else in this phase).

2. Author the math-core boundary. The whole pipeline is free functions and value types over a
   geometry batch; the ECS layer is a thin shell above it, exactly as `CkPathNetwork_Build.h` is
   "pure math, runtime-callable, no ECS/world dependency". Illustrative:

   ```cpp
   namespace ck::groundnav
   {
       // Every bake stage is a free function over values. No UWorld, no registry, no UObject.
       auto
       DoRasterizeSpans(
           const FCk_GroundNav_GeometryBatch& InGeometry,
           const FCk_GroundNav_BakeConfig&    InConfig,
           FCk_GroundNav_SpanField&           OutSpans) -> FCk_GroundNav_BakeStageResult;
   }
   ```
   → verify: `rg --no-ignore -n "UWorld|FCk_Handle|UObject" Plugins/CkFoundation/Source/CkGroundNav/Public/CkGroundNav/Bake`
   → **zero hits**. A math file that needs a world is a design defect in the math core, not a test
   that needs a world (VALIDATION.md §2.1).

3. Author the JPH-free geometry backend seam, mirroring
   `Backend/CkVoxelNav_GeometryBackend.h` + `_Jolt.h` / `_Stub.h`: enumerate-static-bodies-in-bounds,
   fetch-shape-geometry, cheap bounds prefilter. **Static bodies only** — kinematic bodies are
   invisible to the bake and are served by markup in PHASE_4. Illustrative:

   ```cpp
   namespace ck::groundnav
   {
       class CKGROUNDNAV_API ICk_GroundNav_GeometryBackend
       {
       public:
           virtual ~ICk_GroundNav_GeometryBackend() = default;

           virtual auto
           Get_HasGeometryInBounds(const FBox& InBounds) const -> bool = 0;

           virtual auto
           Get_StaticBodiesInBounds(const FBox& InBounds, TArray<FCk_GroundNav_BodyRef>& OutBodies) const -> int32 = 0;

           virtual auto
           Get_ShapeGeometry(const FCk_GroundNav_BodyRef& InBody, FCk_GroundNav_GeometryBatch& OutBatch) const -> bool = 0;
       };
   }
   ```
   The `_Stub` implementation takes a hand-authored box list and is the substrate for every Layer-1
   test in this phase. The `_Jolt` implementation goes through CkJolt's JPH-free surface — **no
   `<Jolt/...>` include may appear in CkGroundNav**.
   → verify: `rg --no-ignore -l "<Jolt/" Plugins/CkFoundation/Source/CkGroundNav` → **zero**.

4. Content fingerprint (F1.11) is computed **here**, during collection, in canonical order — the hash
   input list is a frozen, documented enumeration.
   → verify: a Layer-1 test per hash input asserting that perturbing it forces a rebuild, and that
   changing geometry submission order does **not** change the hash.

   **Gate 1A-4.**
   - Every enumerated input perturbs the hash; order does not → continue.
   - An input you must add to the bake is not in the enumeration → add it to the enumeration **and**
     add its test in the same change. This is the exact bug class the feature invites; a bake input
     outside the hash is a permissive skip, which is the unacceptable direction.
   - A hash input cannot be made order-independent → **STOP**, record it.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

5. Author F1.12 agent profiles. **Radius is deliberately NOT a profile parameter** — it is a
   query-time predicate against the clearance field. Illustrative:

   ```cpp
   USTRUCT(BlueprintType)
   struct CKGROUNDNAV_API FCk_GroundNav_AgentProfile
   {
       GENERATED_BODY()
       CK_GENERATED_BODY(FCk_GroundNav_AgentProfile);

   private:
       UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       FCk_AnyShape _StandingExtents;

       UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       float _MaxSlopeDegrees = 45.0f;

       UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       float _MaxSlopeChangeDegrees = 30.0f;

       UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       float _StepHeightUu = 40.0f;

       UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       float _LedgeSensitivity = 1.0f;

       UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       float _RoughPerchToleranceUu = 0.0f;

   public:
       CK_PROPERTY_GET(_StandingExtents);
       CK_PROPERTY_GET(_MaxSlopeDegrees);
       CK_PROPERTY_GET(_MaxSlopeChangeDegrees);
       CK_PROPERTY_GET(_StepHeightUu);
       CK_PROPERTY(_LedgeSensitivity);
       CK_PROPERTY(_RoughPerchToleranceUu);

   public:
       CK_DEFINE_CONSTRUCTORS(FCk_GroundNav_AgentProfile, _StandingExtents);
   };
   ```
   **On the four `float` defaults above:** bake tunables are **plain config floats in unreal units
   and degrees**, matching the `CkNav_ProjectSettings` precedent (which expresses projection extents
   as floats). The "never bare floats" doctrine governs **authored gameplay shape assets**, not bake
   config — so the authored *shape* (`_StandingExtents`) is a `CkShapes` type, and the scalar bake
   knobs beside it stay floats. Durations and budgets are still `FCk_Time`. An invalid profile (negative clearance, slope > 90°, step > clearance) is rejected at
   admission with `CK_ENSURE_IF_NOT` — **never silently clamped**, and the failure body terminates
   the bake with a status and publishes nothing.
   → verify: a Layer-1 test per invalid-profile case asserting rejection, **zero** partial state, no
   published field, no crash.

6. Gate 1A: `Ck.GroundNav.Bake.*` runs headless.
   ```powershell
   Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --test `
     --test-pattern GroundNav --parallel 1 --output=Saved/Logs/BuildTest.log `
     --project="D:\Repos\CkPlugins_3"
   ```
   (A new C++ automation test needs a **touch + relink** to appear; `--generate` alone does not
   create it. A green run whose Total did not rise has not run your new tests.)
   → verify: Total rose by the number of tests you added; all green.

---

## 5. Sub-phase 1B — Rasterization, walkability filtering, layer extraction (F1.2, F1.3, F1.4)

**1B entry:** 1A exit green.
**1B exit:** a two-storey fixture resolves into exactly two layers with disjoint per-column
occupancy, from a hand-authored box list, headless.

### Steps

1. Span rasterization (F1.2): per-column ordered non-overlapping spans carrying top height, walkable
   flag, and a quantized surface normal; adjacent spans within the climb threshold merge. Degenerate
   or NaN triangles are **dropped with a counted diagnostic**, never rasterized. A column count over
   the tile budget fails the tile **with a status**.
   → verify: Layer 1, the four fixtures named in FEATURE_MATRIX F1.2 — a 1000×1000×10 uu box at Z=0
   is one walkable span per covered column at the expected height (±half a cell height); a 45° ramp
   is monotone with a normal within tolerance of the analytic normal; two floors 1000 uu apart are
   exactly two spans per column; a 30 uu step merges under a 40 uu climb threshold and does not
   under a 20 uu one.

2. Walkability filtering (F1.3) as **three ordered pure-function passes** — low-clearance, ledge,
   step/climb consistency — plus the optional rough-perch dilation. The 4-/8-neighbour connection
   mask produced here is the **only** adjacency the distance transform and the plate merge may
   consult: "walkable" has exactly one definition in the codebase.
   → verify: Layer 1, F1.3's three fixtures — a 100 uu slot under a 150 uu ceiling is walkable for a
   140 uu profile and demoted for a 160 uu one; the top row along a 500 uu cliff edge is demoted and
   the row behind it is not; a 5 uu sawtooth stays one connected region under the rough-perch
   tolerance and shatters at tolerance zero.

   **Gate 1B-2.**
   - All three fixtures behave as specified → continue.
   - A filter's result depends on pass order in a way the spec does not state → **STOP**: pass
     ordering is a design ruling, not an executor choice.
   - A second definition of "walkable" appears (a filter consults raw spans instead of the mask) →
     fix it; the single-definition rule is load-bearing for F1.5 and F1.6.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

3. Vertical layer extraction (F1.4): flood-fill connected components over the connection mask, then
   assign components to layers by 2D-footprint-bitset overlap — a component joins the lowest-indexed
   layer whose footprint does not intersect it, otherwise it opens a new layer. An unassignable span
   **opens a new layer rather than being dropped**: a layer count is cheap, a lost floor is not.
   Layer index is part of the stable id triple (tile, layer, cell).
   → verify: Layer 1 — a two-storey box building yields exactly 2 layers with disjoint per-column
   occupancy; a spiral ramp passing over its own lower run yields ≥2 layers with the ramp still one
   connected component across the split; a flat plane yields exactly 1 layer.

4. Gate 1B: `--test-pattern GroundNav`.
   → verify: all 1A + 1B tests green, Total rose by the tests added.

---

## 6. Sub-phase 1C — Clearance, plate merge, portals (F1.5, F1.6, F1.7)

**1C entry:** 1B exit green.
**1C exit:** measured cell→plate collapse recorded as a number; portal minimum clearance pinned by
the pinch-point fixture.

### Steps

1. Per-cell clearance (F1.5): a two-pass chamfer distance transform over each layer's walkable mask,
   scaled to world units, stored as a flat per-cell array in the tile's finest tier. Per-cell arrays
   exist **only** for height and clearance; everything else lives at plate level
   (REPRESENTATION.md's memory mitigation).
   → verify: Layer 1 — on a 1000×1000 uu open square at 25 uu cells the centre cell's clearance is
   500 uu ± one cell diagonal; a 90 uu corridor yields max 45 uu ± one cell along its spine; and
   `clearance >= R` admits/rejects exactly the set a brute-force O(n²) reference computes on a 64×64
   fixture. **Include the cross-tile case** — VALIDATION.md A3 names it as the one that silently
   breaks — and a concave corner.

2. Merged-plate decomposition (F1.6): greedy rectangle decomposition per layer, seeded in a
   **deterministic scan order**. The merge criteria are exactly three, and they are the frozen set:
   **plane-fit tolerance**, **normal cone**, **policy equality** (same area-tag set, same walkable
   status). Illustrative:

   ```cpp
   USTRUCT(BlueprintType)
   struct CKGROUNDNAV_API FCk_GroundNav_MergeTunables
   {
       GENERATED_BODY()
       CK_GENERATED_BODY(FCk_GroundNav_MergeTunables);

   private:
       UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       float _PlaneFitToleranceUu = 0.0f;

       UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
       float _NormalConeDegrees = 0.0f;

   public:
       CK_PROPERTY_GET(_PlaneFitToleranceUu);
       CK_PROPERTY_GET(_NormalConeDegrees);
   };
   ```
   **Executors expose these two tunables and implement the three criteria. They do not add a fourth
   criterion, remove one, or redefine "mergeable"** ([NN-D7] fences). The *values* are
   `[MEASURE at phase entry]`: derive them from the staircase and ramp fixtures, record them in
   PROGRESS.md as a numbered decision with the fixture evidence.
   A region that cannot merge degenerates gracefully to one plate per cell — correct, just larger.
   → verify: Layer 1 — a flat 100×100 cell plane merges to exactly 1 plate; a plane with one hole
   merges to ≤5 plates; a 12-tread staircase merges to **between tread-count and 2 × tread-count
   plates for the 12-tread fixture** (i.e. 12–24; the exact pin is recorded at the first green run)
   with every plate's max height error within the plane-fit tolerance; the decomposition is identical
   across 100 runs and independent of geometry submission order.

   **Gate 1C-2 (the phase's real fork) — measure, then return.** The executor runs the sweep and
   records it; the executor does **not** pick the frozen values.

   **Sweep ranges** (against the named staircase and ramp fixtures):
   - plane-fit tolerance: **0.25–2.0 × the finest cell height**, in 0.25 steps.
   - normal cone: **5–30 degrees**, in 5-degree steps.

   For every (tolerance, cone) pair record: plate count, max per-plate plane-fit residual,
   shatter/over-merge verdict, and the collapse ratio. Record the whole grid in PROGRESS.md
   § Blockers, then **RETURN TO THE ORCHESTRATOR** for the numbered decision that freezes the two
   values. Do not proceed past 1C on a self-chosen value.

   Reference points for that decision, recorded alongside the grid:
   - the pairs at which the staircase neither shatters (one plate per tread) nor over-merges
     (a plate whose interior height error exceeds the tolerance);
   - whether shattering and over-merging have **no** overlapping window on the fixture — that would
     mean the merge criteria themselves need changing, which is an orchestrator decision;
   - the collapse ratio against the in-house volumetric precedent (91,752 → 359 on a 6400 uu scene);
     a poor ratio is a performance finding for PHASE_3's budget, not a correctness failure.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

3. Portal extraction (F1.7): for every adjacent plate pair — within a layer, across a layer boundary
   where a ramp changes layer, and across a tile boundary — emit the shared edge interval, its
   endpoints, the two plate ids, the **minimum clearance across the crossing**, and a traversal
   policy key. Portals are **derived on every rebuild, never patched**. A sub-cell-length portal is
   still emitted, carrying its true tiny clearance. A tile-boundary portal is emitted only once
   **both** tiles are built; until then the boundary is a hard edge and a path across it fails as
   `Unbuilt`, **never as `Blocked`**.
   → verify: Layer 1 — two abutting rooms joined by one door yield exactly one portal whose min
   clearance is the door width/2 ± one cell; a fully enclosed plate yields zero portals; a wall
   between two tiles produces zero cross-tile portals and a floor across the same boundary produces
   a portal whose interval matches the floor width; removing and re-adding a tile reproduces
   **byte-identical** portals.

4. The pinch-point pin — this is why the representation was chosen. A fixture where two wide plates
   join through a narrow doorway must reject a fat agent that per-plate clearance alone would admit.
   → verify: Layer 1 test asserting the per-portal minimum rejects and per-plate clearance would not
   (VALIDATION.md A3, portal bullet).

5. Gate 1C: `--test-pattern GroundNav`.
   → verify: all tests green; collapse ratio and merge tunables recorded in PROGRESS.md.

---

## 7. Sub-phase 1D — Tiling, immutable publish, epochs, reachability, budgeting (F1.8–F1.11)

**1D entry:** 1C exit green.
**1D exit:** a reader holding a published field across a rebuild observes the old contents exactly;
sliced bake equals one-shot bake.

### Steps

1. Tiling + immutable publish + epochs (F1.8). Illustrative — mirrors
   `FFragment_VoxelNavVolume_BuiltOctree`:

   ```cpp
   namespace ck
   {
       struct CKGROUNDNAV_API FFragment_GroundNavVolume_BuiltField
       {
           CK_GENERATED_BODY(FFragment_GroundNavVolume_BuiltField);

           friend class FProcessor_GroundNavVolume_Build;

       private:
           TSharedPtr<const FCk_GroundNav_Field> _Field;
           FCk_GroundNav_Epoch                   _Epoch;
           FCk_GroundNav_Epoch                   _AggregatedTileEpochSum;
           ECk_GroundNav_BuildStatus             _Status = ECk_GroundNav_BuildStatus::Unbuilt;

       public:
           CK_PROPERTY_GET(_Field);
           CK_PROPERTY_GET(_Epoch);
           CK_PROPERTY_GET(_AggregatedTileEpochSum);
           CK_PROPERTY_GET(_Status);
       };
   }
   ```
   Staleness is **derived at the read boundary, never stored as a flag**. The chunked field's
   fingerprint is the **sum** of tile epochs. A failed build leaves the previously published pointer
   untouched and records a status.
   → verify: Layer 1 — a reader holding the shared field across a full rebuild observes the old
   field's exact contents; the post-rebuild epoch is strictly greater; the aggregated sum is monotone
   across any interleaving of per-tile rebuilds; a failed build leaves the published pointer
   unchanged with a failure status.

2. Identity pin. **Stable integer ids only** — tile id, layer index, plate id, portal id. No raw
   pointers, no `TObjectPtr`/`TWeakObjectPtr`, no engine-object reference inside any field value
   type.
   → verify:
   ```powershell
   rg --no-ignore -n "TObjectPtr|TWeakObjectPtr|UObject\*|\bAActor\b|\*\s*_" `
      "Plugins\CkFoundation\Source\CkGroundNav\Public\CkGroundNav\Field"
   ```
   → **zero hits** (VALIDATION.md A3, identity bullet).

3. Reachability components (F1.9): union-find over portals during field assembly, stored as a flat
   per-plate `int32` array. **Document the honest limitation in the API contract**: components ignore
   per-agent clearance, so a *different* label proves unreachability while the *same* label does not
   prove reachability for a fat agent. Labels are valid only within one published epoch and are
   re-derived on rebuild, never carried across.
   → verify: Layer 1 — two rooms with no door carry different labels; opening the door (rebuild)
   merges them; assignment is deterministic and independent of tile build order.

4. Deterministic build budgeting (F1.10): **probe count is the primary budget**, wall-clock only a
   secondary guard expressed as `FCk_Time`. A slice consumes at most N probes, records where it
   stopped, and resumes with identical results. Exhausting a slice **never publishes a partial
   field**.
   → verify: Layer 1 — `SlicedBakeMatchesOneShotBake` over a fixture with ≥5 forced slices; probe
   count for a given fixture+config identical across runs **and across slice sizes**; no publish
   while a slice is outstanding.

   **Gate 1D-4.**
   - Sliced == one-shot, byte-identical, probe count stable across slice sizes → continue.
   - Probe count varies with slice size → the slice boundary is re-probing; that is a defect in the
     resume checkpoint, not an acceptable tolerance. Fix it. Two attempts → STOP.
   - The bake is deterministic per run but differs across runs → an iteration order depends on
     pointer/hash order somewhere. Find it. Two attempts → STOP.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

5. The ECS shell: a ground-nav volume entity, a `FFragment_GroundNavVolume_BuiltField` slot, a
   `Request_Build` (completion delegate **last**, `AutoCreateRefTerm`, no C++ default), and a
   budgeted build processor scheduled in the one window provably outside the async physics step —
   the same placement CkVoxelNav's builder uses. Illustrative:

   ```cpp
   namespace ck
   {
       struct FProcessor_GroundNavVolume_Build : public TProcessor<
           FProcessor_GroundNavVolume_Build,
           FCk_Handle_GroundNavVolume,
           FFragment_GroundNavVolume_BuiltField,
           TExclude<FTag_GroundNavVolume_BuildInProgress>>
       {
           using TProcessor::TProcessor;
           using Group = FGroup_Transform;

           auto
               ForEachEntity(
                   TimeType InDeltaT,
                   HandleType InVolume,
                   FFragment_GroundNavVolume_BuiltField& InBuiltField) -> void;
       };
   }
   CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_Build);
   ```
   → verify: the processor is registered (`rg -c "CK_REGISTER_PROCESSOR" Source/CkGroundNav` > 0);
   a Layer-1 or headless test drives a build to completion through the request.

6. Phase gate: full suite on the phase's **final** artifact.
   ```powershell
   Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --test `
     --parallel 1 --no-nullrhi --output=Saved/Logs/BuildTest.log --project="D:\Repos\CkPlugins_3"
   ```
   → verify: delta-zero vs the §2 baseline **plus** the phase's new `Ck.GroundNav.*` tests green.

---

## 8. Exit criteria

- [ ] The full `Ck.GroundNav.<Area>.<Scenario>` family runs **headless** with the `_Stub` geometry
      backend, zero engine subsystems, `IMPLEMENT_*AUTOMATION_TEST` only, zero `DEFINE_SPEC`;
      PIE-requiring tests (if any) isolated in `*Pie.cpp`.
- [ ] `<Area>` values used this phase ⊆ {`Bake`, `Layers`, `Clearance`, `Merge`, `Portals`, `Epoch`}.
- [ ] Determinism: a bake over a hand-authored box list is **bit-identical across 100 runs**.
- [ ] Named pins green: `SlicedBakeMatchesOneShotBake`; two-storey → exactly 2 layers; staircase
      plate count in the asserted range with plane-fit residual within the frozen tolerance;
      cross-tile clearance correct; portal min-clearance pinch-point rejection.
- [ ] `rg --no-ignore -l "<Jolt/" Plugins/CkFoundation/Source/CkGroundNav` → **0**.
- [ ] `rg --no-ignore -n "UWorld|FCk_Handle|UObject" .../CkGroundNav/Public/CkGroundNav/Bake` → **0**.
- [ ] No raw pointer or engine-object reference in any published field value type (grep evidence).
- [ ] Numbers recorded in PROGRESS.md **and** VALIDATION.md as tracked values: measured cell→plate
      collapse ratio, the frozen merge tunables with their fixture evidence, bake wall time and peak
      memory on the reference scene, probe count for the reference fixture.
- [ ] Full suite **delta-zero** vs the §2 baseline on the phase's final artifact; zero new ensures,
      zero new warnings; editor boots clean.
- [ ] Provenance cells complete for F1.1–F1.12 and F1.43.
- [ ] Comment audit run over the phase diff; PROGRESS.md updated; PHASE_2 entry criteria re-verified.

---

## 9. Fences

- **No queries, no search, no funnel.** Projection, containment, raycast, boundary segments,
  reachability *queries*, point generation → PHASE_2. A* and string-pulling → PHASE_3.
  Reachability *component labelling* (F1.9) is in scope because it is a bake product; the
  `Get_IsReachable` query is not.
- **No markup, no repair, no dynamics.** PHASE_4. Do not add a dirty-bounds fragment "while you're
  in there".
- **No coincident polygon mesh.** That re-opens the rejected hybrid candidate ([NN-D7] fences).
- **No per-agent-radius baked fields.** Clearance is per cell and per portal; one bake serves every
  radius. A second baked *profile layer* (slope/step/height, never radius) is a CTO decision.
- **Never patch a published field.** Repair — and any change at all — derives a new field or tile
  and swaps. Patching makes corruption representable, which is precisely the property this
  representation was chosen for.
- **Do not re-tune the definition of "mergeable"** beyond the two exposed tunables and the frozen
  three criteria.
- **Stable integer ids only** inside the field. No pointers, no engine-object references — this is
  also what makes PHASE_7's serialization possible at all.
- **`CK_REGISTER_PROCESSOR` on every processor.** Unregistered means unscheduled means a silent
  no-op.
- **Failure is a status, never an empty field.** A backend-unavailable tile bakes as `Unbuilt`, never
  as `Built` with zero plates over non-empty geometry.
- **Do not touch CkNavigation, CkCrowd, CkAStar, or CkVoxelNav behaviour** in this phase.

---

## 10. [P1] Done means

VALIDATION.md **A3** (bake core) is green with evidence in PROGRESS.md — every bullet, including the
two whose absence would be invisible until much later: the **cross-tile clearance** case and the
**portal minimum-clearance** pinch point — every VALIDATION.md §0 standing gate passes on the
phase's final artifact, and the Layer-1 coverage row for "rasterization / layers / clearance / merge
/ portals" in VALIDATION.md §2.4 is satisfied.
