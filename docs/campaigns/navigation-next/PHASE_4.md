# PHASE 4 — Dynamics: markup, markup-live ground truth, local repair, observability

> Freshness: authored 2026-08-31 in planning session 1. Status of record is
> [PROGRESS.md](PROGRESS.md); the designs of record are [FEATURE_MATRIX.md](FEATURE_MATRIX.md)
> F1.31–F1.37, [MIGRATION_SEAM.md](MIGRATION_SEAM.md) Step 2 ([NN-D8a], [NN-D8b], [NN-D8d]), and
> `research/R4-ck-native-assets.md` §1 (the in-house repair/epoch discipline this phase mirrors).
> Where this file and a design doc disagree, the design doc wins and the disagreement is a **STOP**.
>
> **Every code block in this file is illustrative — the executor refines it mechanically, does not
> redesign it.**

---

## 1. Goal

Make the field respond to a world that changes: neutral area markup natively implemented on
CkGroundNav, the markup-live ground-truth probe, consolidated surface revision and rebuild
observability, push-based path invalidation, local repair as **partial re-derivation**, multi-world
safety, and the deterministic test rebuild hook — landing **F1.31–F1.37**.

The phase's real product is a property, not a feature: **because the plate decomposition is derived
and never patched, corruption is unrepresentable**. Every test in §4–§7 exists to keep that true.

---

## 2. Entry criteria

- [ ] **PHASE_3 closed in PROGRESS.md**, including the first full-suite shadow report.
- [ ] Repo clean, on a `feature/` branch (root + CkFoundation + CkTests) — commands as PHASE_1 §2.
- [ ] **The neutral markup vocabulary from PHASE_0 is still the only obstacle vocabulary in tests**:
      ```powershell
      rg --no-ignore -n "UNavArea_Null|ARecastNavMesh" Plugins/CkTests/Script
      ```
      → **zero hits**. Non-zero ⇒ a fixture regressed to the Recast vocabulary ⇒ **STOP**.
- [ ] **The revision surface is already consolidated** ([NN-D8a], done in PHASE_0):
      ```powershell
      rg --no-ignore -n "OnNavigationGenerationFinished" Plugins/CkFoundation/Source
      ```
      → one implementation site.
- [ ] **The in-house repair pins still exist** — they transfer one-for-one and are this phase's
      template:
      ```powershell
      rg --no-ignore -n "RepairedOctreeMatchesAFullRebake|MovedObstacleFlipsOccupancyOnlyWhereItMoved|SlicedRepairMatchesOneShotRepair" `
         Plugins/CkTests/Source/CkTests/Private/UnitTests/CkVoxelNav
      ```
      Zero hits ⇒ R4 §1 has drifted ⇒ **STOP**.
- [ ] **A6 `[MEASURE at phase entry]` budgets filled in now**, measured on the **Recast** path on the
      **same fixtures**: markup-live latency, moved-obstacle repair latency. Recorded in PROGRESS.md
      with the artifact. These are the promotion gates B3; a budget you invented is not a budget.
- [ ] **Pre-flight:** the editor must be closed for this project (the toolbox exits **77** if it is
      open) — see the `build-test` skill's pre-flight table.
- [ ] **Phase-entry baseline captured this session** (full suite, `--parallel 1 --no-nullrhi`,
      totals + failing-test names + artifact identity).
- [ ] Skills loaded: `ck-macros-and-codegen`, `ckecs-architecture-contract`,
      `ck-tests-authoring-and-running`, `ck-performance-and-analysis`, `build-test`,
      `ck-change-control`.

---

## 3. Standing decision gate (applies to EVERY `-> verify:` line in this file)

- Observation matches the stated expectation → continue.
- Compile/UHT/link failure → fix mechanically and re-verify. **Two failed attempts → STOP.**
- A hermetic test fails → the implementation is wrong; the assertion is the specification. Do not
  loosen an equivalence assertion (`repaired == full rebake` is exact, not approximate). Two failed
  attempts → STOP.
- A latency budget is missed → **STOP** and record both numbers with the fixture. Widening a
  promotion budget is an orchestrator decision.
- A previously green test is now red → **STOP**, revert the offending step, diagnose, re-sequence.
- A crowd/queue test changes result on the **Recast** provider → **STOP**: this phase must not move
  the Recast path at all.
- **Anything else → STOP, record it in PROGRESS.md § Blockers with verbatim evidence, end session.**

---

## 4. Sub-phase 4A — Neutral area markup and the markup-live probe (F1.31, F1.32)

**4A entry:** §2 all green.
**4A exit:** disabling a markup restores prior policy **byte-identically** against a field baked
without it; a cost-only markup performs **zero** geometry probes.

### Steps

1. Implement markup as an **ECS-native record**: a request enqueued on the ground-nav volume entity,
   drained by a `HandleRequests` processor into a markup fragment holding **value-only** records —
   shape from `CkShapes` types, area tag, enabled state, stable id.

   **Markup shape domain.** Markup accepts **`FCk_AnyShape`** — box, sphere, capsule, or cylinder —
   and the shape is **rasterized to cells at markup-apply time**. There is no box-only path and no
   second shape vocabulary. Illustrative:

   ```cpp
   USTRUCT(BlueprintType)
   struct CKGROUNDNAV_API FCk_Request_GroundNav_AreaMarkup : public FCk_Request_Base
   {
       GENERATED_BODY()
       CK_GENERATED_BODY(FCk_Request_GroundNav_AreaMarkup);

   private:
       UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
       FCk_AnyShape _Shape;

       UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
       FGameplayTag _AreaTag;

       UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
       ECk_EnableDisable _Enabled = ECk_EnableDisable::Enable;

   public:
       CK_PROPERTY_GET(_Shape);
       CK_PROPERTY_GET(_AreaTag);
       CK_PROPERTY(_Enabled);

   public:
       CK_DEFINE_CONSTRUCTORS(FCk_Request_GroundNav_AreaMarkup, _Shape, _AreaTag);
   };
   CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_GroundNav_AreaMarkup);

   namespace ck
   {
       struct CKGROUNDNAV_API FFragment_GroundNav_MarkupRecords
       {
           CK_GENERATED_BODY(FFragment_GroundNav_MarkupRecords);

           friend class FProcessor_GroundNav_HandleRequests;

       private:
           TArray<FCk_GroundNav_MarkupRecord> _Records;

       public:
           CK_PROPERTY_GET(_Records);
       };
   }
   ```
   Overlap semantics are fixed, not chosen: **usage tags OR together, cost multipliers take the
   max**. The bake reads markup records as an **input**, so markup participates in the content hash
   (F1.11) and stamps plate policy during decomposition. An invalid shape is rejected at admission
   with `CK_ENSURE_IF_NOT`, leaving **no partial state** and firing `Failed_NotEnqueued` on the
   completion delegate before returning — never strand a caller. A markup whose bounds lie in an
   unbuilt region is **accepted and recorded**, and applied when that region builds — never dropped.
   → verify: Layer 1 — painting an impassable box makes exactly the covered plates impassable and no
   others; two overlapping cost regions yield the max; **disabling restores prior policy exactly**,
   byte-compared against a field baked without the markup.

2. Implement the **cost-versus-walkability split**, which is the difference between a free update and
   a bake: markup that changes only cost updates plate policy **without re-deriving geometry**;
   markup that changes walkability triggers a local repair (§6).
   → verify: Layer 1 — a cost-only markup performs **zero** geometry probes (assert on the probe
   counter).

   **Gate 4A-2.**
   - Cost-only markup → zero probes; walkability markup → repair triggered → continue.
   - A cost-only markup probes geometry → the split is not implemented; it will show up later as
     markup-thrash under a moving crowd. Fix. Two attempts → STOP.
   - A walkability markup updates policy *in place* without re-deriving → **STOP**. That is patching
     a published field, the one fence that makes corruption representable.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

3. Implement the markup-live probe (F1.32): each markup record carries the epoch at which it was
   applied; the query compares that against the currently published epoch for the tiles the markup's
   bounds intersect. Because epochs are **derived at the read boundary and never stored as staleness
   flags**, there is no cache to go wrong.
   → verify: Layer 1 — immediately after a markup request `IsMarkupLive` is false; after the covering
   tiles republish it is true; a markup spanning two tiles is live only when **both** have
   republished.

4. Pin the race the probe exists to remove. Layer 2: paint an obstacle, immediately repath, and
   assert the path does not cross it — an autotest that **fails on the pre-migration behaviour**.
   → verify: the autotest is green on CkGroundNav, and confirmed to be a real test by checking it
   fails when the live-gate is bypassed.

5. Gate 4A: `--test-pattern GroundNav` with `--discover-fresh`.

---

## 5. Sub-phase 4B — Revision, rebuild observability, path invalidation (F1.33, F1.34)

**4B entry:** 4A exit green.
**4B exit:** the bounds signal fires exactly once per publish; a consumer holding a stale epoch
re-plans **exactly once per drift, not once per frame**.

### Steps

1. Implement `Get_SurfaceRevision(world)` on the **aggregated tile-epoch sum** — the sum *is* the
   revision; there is no separate counter to drift ([NN-D8a] + F1.8). Emit
   `BindTo_OnSurfaceRebuilt(bounds)` from the publish step with the republished tiles' union bounds,
   defined via `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE` and bound through the generated
   `BindTo_*` / `UnbindFrom_*` UFUNCTIONs — never hand-rolled.
   → verify: Layer 1 — revision is strictly monotone across arbitrary interleavings of tile rebuilds;
   the bounds signal fires **exactly once per publish** with bounds containing every changed cell and
   no more than the union of republished tiles. Layer 2 — both former subsystems' consumers behave
   identically on the consolidated API.

2. Implement path invalidation (F1.34) as a **push**, not per-path polling: on `OnSurfaceRebuilt`, an
   ECS processor tests the changed bounds against each active path's cached corridor bounds and sets
   a repath-required tag; the next `PathRefresh` tick consumes the tag and runs **plan repair**
   (PHASE_3's F1.30) rather than a cold search. Bounds are values; **no path holds a reference to the
   field**.
   → verify: Layer 1 — a rebuild adjacent to but not intersecting a corridor does **not** flag it;
   one intersecting it does; the inflation margin is respected exactly. Layer 2 — an obstacle dropped
   onto a crowd's path causes a repath within a **bounded, measured** number of frames, recorded.

   **Gate 4B-2.**
   - Adjacent-but-not-intersecting does not flag; intersecting does; repath count bounded → continue.
   - The epoch bumps every frame, or agents re-plan every frame → repair is thrashing. **STOP** and
     record: this is one of the named `[EDITOR-VERIFY]` fail signatures and it will not survive the
     promotion gate.
   - A path holds a shared reference to the field to answer invalidation → replace with cached
     corridor **bounds**. Holding the field defeats the immutable-publish memory story.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

3. Gate 4B: `--test-pattern GroundNav` then `--test-pattern Crowd`.

---

## 6. Sub-phase 4C — Local repair (F1.35)

**4C entry:** 4B exit green.
**4C exit:** the three transferred pins are green — `RepairedFieldMatchesAFullRebake`,
`MovedObstacleChangesOnlyWhereItMoved`, `SlicedRepairMatchesOneShotRepair`.

### Steps

1. Implement repair with the in-house discipline verbatim: a **dirty-bounds fragment** plus
   `NeedsRepair` / `RepairInProgress` tags; re-probe **only** the columns whose inflated probe box
   intersects the dirty bounds; re-run filters, distance transform, plate merge, and portal
   extraction for the touched tiles; **assemble a new structure and swap**. Illustrative:

   ```cpp
   namespace ck
   {
       CK_DEFINE_ECS_TAG(FTag_GroundNavVolume_NeedsRepair);
       CK_DEFINE_ECS_TAG(FTag_GroundNavVolume_RepairInProgress);

       struct CKGROUNDNAV_API FFragment_GroundNavVolume_DirtyBounds
       {
           CK_GENERATED_BODY(FFragment_GroundNavVolume_DirtyBounds);

           friend class FProcessor_GroundNavVolume_Repair;

       private:
           TArray<FBox> _DirtyBounds;

       public:
           CK_PROPERTY_GET(_DirtyBounds);
       };

       struct FProcessor_GroundNavVolume_Repair : public TProcessor<
           FProcessor_GroundNavVolume_Repair,
           FCk_Handle_GroundNavVolume,
           FFragment_GroundNavVolume_BuiltField,
           FFragment_GroundNavVolume_DirtyBounds,
           FTag_GroundNavVolume_NeedsRepair,
           TExclude<FTag_GroundNavVolume_RepairInProgress>>
       {
           using TProcessor::TProcessor;
           using Group = FGroup_Transform;

           auto
               ForEachEntity(
                   TimeType InDeltaT,
                   HandleType InVolume,
                   FFragment_GroundNavVolume_BuiltField& InBuiltField,
                   FFragment_GroundNavVolume_DirtyBounds& InDirtyBounds) -> void;
       };
   }
   CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavVolume_Repair);
   ```
   → verify: Layer 1 — `RepairedFieldMatchesAFullRebake` is **byte-identical**;
   `MovedObstacleChangesOnlyWhereItMoved` (cells outside the union of old+new bounds are untouched);
   `SlicedRepairMatchesOneShotRepair`; probe count for a small repair is bounded well below a full
   bake (record the number as a tracked regression value).

   **Gate 4C-1.**
   - All three pins green, byte-identical where stated → continue.
   - `Repaired != FullRebake` by a small amount → **STOP**. Do not add a tolerance. "Byte-identical"
     is the property the representation was chosen for, and a near-match means some stage is reading
     stale state across the repair boundary.
   - Sliced ≠ one-shot → the slice checkpoint re-derives differently; same rule as PHASE_1's budget
     gate. Two attempts → STOP.
   - Repair mutates the published structure to save an allocation → **STOP**; revert. This is the
     single fence whose violation cannot be detected later by any test in this suite.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

2. Pin the immutability directly. Layer 1: hold a `TSharedPtr<const>` to the pre-repair field across
   a repair and assert it is **unchanged and still self-consistent**.
   → verify: the test is green and the held field's contents byte-compare to a pre-repair copy.

3. Measure moved-obstacle repair latency on the same fixture as the §2 Recast measurement.
   → verify: latency ≤ the recorded Recast baseline (promotion gate B3). Record both numbers.

4. Layer-2 pin: a door opening and closing in a gym scenario flips reachability in the expected
   number of frames.
   → verify: green under `--discover-fresh`.

5. Gate 4C: `--test-pattern GroundNav`.

---

## 7. Sub-phase 4D — Multi-world safety, test hook, and the obstacle-suite crossover (F1.36, F1.37)

**4D entry:** 4C exit green.
**4D exit:** the crowd obstacle AS tests are green **on the CkGroundNav provider** — this phase's
headline exit.

### Steps

1. Enforce multi-world safety (F1.36): fields live on ECS volume entities; the deferred queue is
   already per-world from PHASE_0; **nothing static holds a field, a handle, or a `UWorld`**.
   Teardown follows the house `EndPlay` / `Destructor` processor vocabulary, and pending requests
   complete `Failed_Cancelled` via `ck::request::FireCancelledForPending`.
   → verify:
   ```powershell
   rg --no-ignore -n "^\s*static\s.*(Field|Markup|Revision|Deferred)" Plugins/CkFoundation/Source/CkGroundNav
   ```
   → zero process-wide provider state. Layer 2 (multi-PIE) — two worlds with independent fields and
   markup show **zero** cross-talk; ending one PIE session leaves the other's queries correct; a
   fresh startup log shows zero new ensures and zero script errors after world teardown.

   **Gate 4D-1.**
   - Two worlds isolated; teardown ensure-free; fresh startup log clean → continue.
   - Painting in world A changes world B → shared/global field state survived. Find it. Two attempts
     → STOP.
   - An ensure fires on world death → **STOP**; a teardown ensure is a promotion blocker (B6) and it
     compounds across repeated PIE cycles.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

2. Implement `Request_SurfaceRebuild_ForTesting` natively on CkGroundNav ([NN-D8d]): drain the build
   scheduler to completion under the slice budget and republish, then expose the **named "surface
   settled" condition** tests wait on.
   → verify: Layer 2 — an autotest paints markup, calls the hook, waits on the **named condition**,
   and asserts markup-live, **with no `WaitOneFrame` hop counting anywhere in the file**.

3. **The headline:** re-run the F1.42 obstacle fixture suite — every AS test and gym that punches a
   navigation hole — **on the CkGroundNav provider**, through neutral markup. The enumerated sites
   are the ten-plus from R3 §CkTests, migrated in PHASE_0 and green on Recast since then.
   → verify:
   ```powershell
   Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --test `
     --test-pattern Crowd --parallel 1 --discover-fresh --output=Saved/Logs/BuildTest.log `
     --project="D:\Repos\CkPlugins_3"
   ```
   with the provider setting on CkGroundNav → every named test green, each listed with its result in
   PROGRESS.md (VALIDATION.md A6, `UNavArea_Null` bullet — **a promotion prerequisite, not a
   nice-to-have**).

   **Gate 4D-3.**
   - All enumerated fixture tests green on CkGroundNav → continue; this banks promotion condition B2.
   - A fixture test is green on Recast and red on CkGroundNav → **STOP** and record the test name and
     the failure verbatim. Do not adjust the fixture to suit the new provider: the fixture is the
     contract, and adjusting it converts a provider defect into a silently weaker test.
   - A fixture test is flaky only on CkGroundNav → check the settle first (is the predicate gated on
     `Get_IsMarkupLive` and does it name this test's own entities?), then STOP after two attempts.
   - Anything else → STOP, record in PROGRESS.md Blockers, end session.

4. Phase gate: full suite with the provider setting back at its **unchanged default** (Recast), on
   the phase's final artifact.
   ```powershell
   Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --test `
     --parallel 1 --no-nullrhi --output=Saved/Logs/BuildTest.log --project="D:\Repos\CkPlugins_3"
   ```
   → verify: delta-zero vs the §2 baseline plus the phase's new tests green. The default provider does
   **not** change in this phase.

---

## 8. Exit criteria

- [ ] Markup paints: a path that previously crossed a region no longer does; overlap semantics are
      OR-for-tags, max-for-cost; disable restores prior policy **byte-identically**.
- [ ] Cost-only markup performs **zero** geometry probes (probe-counter assertion).
- [ ] Markup-live is ground truth: false immediately after the request, true at the exact publish,
      and true for a two-tile markup only when **both** tiles republished.
- [ ] **The `UNavArea_Null`-derived fixture vocabulary is fully served on CkGroundNav** — every
      enumerated site listed in PROGRESS.md with its migrated test name and result.
- [ ] Repair pins green and exact: `RepairedFieldMatchesAFullRebake`,
      `MovedObstacleChangesOnlyWhereItMoved`, `SlicedRepairMatchesOneShotRepair`.
- [ ] A pre-repair `TSharedPtr<const>` field is provably unchanged across a repair.
- [ ] Epoch semantics: one bump per completed rebuild; staleness derived at the read boundary;
      chunked fingerprint is the epoch sum; a stale-epoch consumer re-plans **once per drift**.
- [ ] Exactly one surface-revision observer; the bounds signal fires exactly once per publish.
- [ ] Multi-PIE isolation green; teardown ensure-free; no process-wide provider state (grep evidence).
- [ ] Markup-live latency and moved-obstacle repair latency **≤ the recorded Recast baselines on the
      same fixtures**, both numbers in PROGRESS.md with their artifact.
- [ ] Three environments for every new public API: C++, Blueprint, AngelScript.
- [ ] Full suite **delta-zero** vs the §2 baseline on the phase's final artifact, with the default
      provider unchanged; zero new ensures, zero new warnings; editor boots clean.
- [ ] Provenance cells complete for F1.31–F1.37.
- [ ] Comment audit run; PROGRESS.md updated; PHASE_5 entry criteria re-verified.

---

## 9. Fences

- **Never patch a published field.** Repair derives a new field or tile and swaps. This is the fence
  whose violation no later test can detect.
- **No tolerance on `repaired == full rebake`.** It is byte-identical or it is a defect.
- **No raw pointers, no `TObjectPtr`/`TWeakObjectPtr`, no engine-object references** inside the field
  or inside markup records — stable integer ids and value shapes only. This is also what keeps
  snapshots safe in PHASE_7.
- **Do not change the default provider.** Promotion is PHASE_8 with its own CTO sign-off; every phase
  leaves the Recast path selectable and green.
- **Do not adjust a fixture to suit the new provider.** The fixture is the contract.
- **Do not re-tune the merge criteria** while chasing a repair mismatch. The merge definition was
  frozen in PHASE_1; a mismatch is a repair defect, not a merge-tuning opportunity.
- **Settle on named conditions, never hop counts.** Markup and field state are global to the shared
  PIE world, so every predicate must name *this test's own* entities, and every paint-then-act wait
  gates on `Get_IsMarkupLive` — the value it exists to provide.
- **`CK_REGISTER_PROCESSOR` on every processor**; deferred `Request_*` APIs end with the completion
  delegate as the **last** parameter; a rejected request still completes (`Failed_NotEnqueued`) —
  never strand a caller.
- **Kinematic bodies stay invisible to the bake.** They are served by markup, which is exactly the
  boundary this phase makes real. Do not widen the geometry domain to "fix" a moving prop.
- **No links, no PathNetwork migration, no editor snap** (PHASE_5); **no debugger or gym** (PHASE_6);
  **no serialization or cook** (PHASE_7).

---

## 10. [P4] Done means

VALIDATION.md **A6** (dynamics, markup, and repair) is green with evidence in PROGRESS.md — including
its two promotion-critical bullets, the fully served `UNavArea_Null` fixture vocabulary (which banks
**B2**) and the moved-obstacle repair latency (which banks half of **B3**) — every VALIDATION.md §0
standing gate passes on the phase's final artifact, and the §2.4 coverage rows "markup + markup-live
probe" and "repair + epoch observability" have their Layer-1 and Layer-2 requirements satisfied.
