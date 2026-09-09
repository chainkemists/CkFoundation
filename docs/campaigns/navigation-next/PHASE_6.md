# PHASE 6 — Debugger and gym

> **Freshness:** authored 2026-08-31 during the P4 planning package (documentation-only session,
> [NN-D3]). **Status of record: [PROGRESS.md](PROGRESS.md)** — never this file.
> **Authoritative upstream:** [PROMPT.md](PROMPT.md), [REPRESENTATION.md](REPRESENTATION.md)
> [NN-D7], [MIGRATION_SEAM.md](MIGRATION_SEAM.md) [NN-D8] (esp. [NN-D8f]),
> [FEATURE_MATRIX.md](FEATURE_MATRIX.md), [VALIDATION.md](VALIDATION.md).
> **Also mandatory before writing any debugger code:** `research/R4-ck-native-assets.md` **§6**
> (the three-tier data flow) and
> `Plugins/CkGameplayDebugger/Source/CkCrowdDebugger/CLAUDE.md` (viewport ownership, snapshot
> boundary, the known baseline failure). Load the `ck-gameplaydebugger-extension` and
> `ck-slate-tools` skills **before** the work, not after.
> **Line references** are a snapshot @ CkFoundation `a25ec9539` / CkGameplayDebugger `d85b77cbe` —
> re-verify before editing.

---

## 1. Goal and feature ids

Make the ground field **observable** — by a human in PIE, by a human in a packaged Development or
Test build, and by the crowd debugger — without any debugger surface ever holding a live reference
into the thing it is drawing.

| Sub-phase | Features | Goal |
|---|---|---|
| **6A** | **F2.9** (tier-1 half) | The **runtime value-only debug snapshot type**, owned by the `CkGroundNav` runtime feature module. |
| **6B** | **F2.9** (draw half) | The **in-world draw module**, Runtime tier, working in packaged Development **and** Test. |
| **6C** | **F2.10** | **CkCrowdDebugger adapter** integration (DeveloperTool tier) — neutral surface bounds + provider health, viewport fit, status panel. |
| **6D** | **F2.13** | Runtime **gameplay-debugger category** — per-agent provider/profile/path state. |
| **6E** | **F2.12** | The **robust gym**: registered, control panel, stations. |
| **6F** | **F2.11** | **PerfLab seeding** migration to the facade's point generators (`[ADAPTER]` site; produces the numbers PHASE_8's gates consume). |

Ordering is load-bearing: **6A before everything** (every other sub-phase consumes the snapshot
type), 6B before 6C (the two overlays must agree, and the draw module is the reference), 6E last of
the visual work (the gym exercises what 6B/6C render), 6F independent and schedulable anywhere after
6A.

VALIDATION gate served: **A8 — Debugger and gym (P6)**, plus VALIDATION §2.3 (Layer 3 gym
requirements) and §4.1/§4.2/§4.4 `[EDITOR-VERIFY]`.

---

## 2. The three-tier pattern this phase must follow (not negotiable)

From `R4 §6` and both debugger `CLAUDE.md`s — they agree, and this phase does not invent a fourth
shape:

1. **The runtime feature module owns the snapshot type** (`CkGroundNav_DebugSnapshot.h`), exactly as
   `CkVoxelNav_DebugSnapshot.h` does.
2. **The DeveloperTool debugger collects and adapts.** The feature adapter translates; reusable
   mechanics (components, materials, reconciliation, picking) belong in **CkDebugScene**, never
   copied into a feature adapter. The snapshot boundary **copies values** — never a gameplay
   `UWorld`, actor, ECS handle/registry, navmesh, or producer.
3. **The viewport is a thin facade** over Common's `SCkDebug_3dPreviewViewport`. Do **not** add a
   CkGroundNav-owned `FPreviewScene`, viewport client, camera implementation, or input router.

Three properties are asserted, not assumed: **value-only**, **whole-snapshot atomic replace**, and
**failure is a status, never an empty scene**.

---

## 3. Entry criteria — confirm all before writing a line

- [ ] **P5 closed** in PROGRESS.md with A7 evidence recorded (links, PathNetwork migration, editor
      snap), and PHASE_5's sub-phase gates 5E/5F each closed or explicitly struck.
- [ ] **F1.8 immutable publish + epochs**, **F1.22 build-status query / provider health**, **F1.31–
      F1.35 markup and repair**, and **F1.40 shadow diagnostics** are landed — this phase renders
      them and builds none of them.
- [ ] **Session-start ritual** run verbatim (PROMPT.md §7) including two "Done"-claim spot-checks.
- [ ] **Phase-entry baseline captured this session** — full toolbox `--build --test`, `--parallel 1`,
      `--no-nullrhi`; totals **and failing-test names** and the artifact identity recorded, with the
      inherited `Nav.Filter.Customer` mapping failure recorded **by name**.
- [ ] **`[MEASURE]` budgets for A8 filled at entry** and recorded: snapshot production cost per
      frame at the reference agent count, draw-call/instance counts at the reference field size, and
      the current `Ck.CrowdDebugger.Viewport3d.*` pass/fail set. Measured on this machine, on the
      stated artifact.
- [ ] **Module rows planned, not improvised** — the `.uplugin` tier each new module will take is
      written down **before** the first module is added (see §9 fences).

**Verify commands** (the `build-test` skill owns the canonical shapes and the editor-closed
pre-flight table):

```powershell
Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --test --parallel 1 --no-nullrhi `
    --output=Saved/Logs/P6-Baseline.log --project="D:\Repos\CkPlugins_3"

# The crowd-debugger family this phase perturbs — capture its exact pass/fail set first
Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --test --parallel 1 --test-pattern CrowdDebugger `
    --output=Saved/Logs/P6-CrowdDebugger-Baseline.log --project="D:\Repos\CkPlugins_3"

# Tier check: what the uplugin says today
rg --no-ignore -n '"Name"|"Type"' Plugins/CkGameplayDebugger/CkDebugger.uplugin
```

---

## Standing decision gate (applies to EVERY `-> verify:` line in this file)

Read this once; it is the default branch set for every verification step below. Steps that need
extra branches carry their own **Gate** block.

- Observation matches the stated expectation → continue to the next step.
- Build fails to compile → fix mechanically (missing include, moved symbol, UHT complaint) and
  re-verify. **Two failed attempts at the same step → STOP.**
- A test that was red in the P6 phase-entry baseline is still red with the same name → not yours;
  continue, and carry it forward in the delta-zero comparison.
- A test that was green in the baseline is now red → **STOP**, restore the known-good state (revert
  your last step), then diagnose before re-applying.
- A test that was red in the baseline is now green → **STOP**. An unexplained improvement is as much
  a defect as a regression — explain it or record it (VALIDATION.md A1).
- **Any observation not enumerated above → STOP, record it in PROGRESS.md § Blockers with verbatim
  evidence, end session.**

---

## 4. Sub-phase 6A — the runtime value-only snapshot (F2.9, tier-1 half)

1. **Author `CkGroundNav_DebugSnapshot.h` in the `CkGroundNav` runtime module.** Contents: boxes,
   segments, counts, stable ids, epochs, and status — **nothing else**. Illustrative house shape —
   *executor refines mechanically, does not redesign*:

   ```cpp
   namespace ck
   {
       struct CKGROUNDNAV_API FCk_GroundNav_DebugSnapshot
       {
           CK_GENERATED_BODY(FCk_GroundNav_DebugSnapshot);

       private:
           ECk_GroundNav_DebugSnapshot_Status _Status = ECk_GroundNav_DebugSnapshot_Status::RuntimeOnly;
           ECk_GroundNav_DebugSnapshot_Source _Source = ECk_GroundNav_DebugSnapshot_Source::LivePie;

           uint64                             _CacheIdentity = 0;
           FCk_GroundNav_Epoch                _EpochSum;

           TArray<FCk_GroundNav_DebugPlate>   _Plates;
           TArray<FCk_GroundNav_DebugPortal>  _Portals;
           TArray<FCk_GroundNav_DebugSegment> _BoundarySegments;
           TArray<FCk_GroundNav_DebugLink>    _Links;

           int32                              _TileCount  = 0;
           int32                              _PlateCount = 0;
           int32                              _CellCount  = 0;

       public:
           CK_PROPERTY_GET(_Status);
           CK_PROPERTY_GET(_Source);
           CK_PROPERTY_GET(_CacheIdentity);
           CK_PROPERTY_GET(_EpochSum);
           CK_PROPERTY_GET(_Plates);
           CK_PROPERTY_GET(_Portals);
           CK_PROPERTY_GET(_BoundarySegments);
           CK_PROPERTY_GET(_Links);
           CK_PROPERTY_GET(_TileCount);
           CK_PROPERTY_GET(_PlateCount);
           CK_PROPERTY_GET(_CellCount);
       };
   }
   ```
   → *verify:* Layer 1 — a reflection- or compile-time assertion that the snapshot and every nested
   type contain **no** handle, `UObject` reference, `TObjectPtr`, `TWeakObjectPtr`, `UWorld`, actor,
   or `TSharedPtr` to a field structure; plus `rg` over the header as corroborating evidence.

2. **Adopt the proven status vocabulary verbatim**:
   `MissingCook / StaleCook / Building / Current / Failed / RuntimeOnly`. Adopt the source enum
   distinguishing `LivePie / RetainedSnapshot / EditorPreview`.
   → *verify:* Layer 1 — one test per status: each failure mode maps to **its** status and never to
   an empty snapshot.

3. **Layer bitmask with deterministic caps.** A snapshot never grows unbounded; truncation is
   deterministic (nearest-first or id-ordered, stated in the contract comment) and is **reported**,
   not silent.
   → *verify:* Layer 1 — the same field produces the same truncated snapshot across 100 runs; a
   truncated snapshot carries a truncation count.

4. **Construct-from-torn-down-producer test.** Build a snapshot, destroy the producing world/field,
   then enumerate the snapshot.
   → *verify:* Layer 1 — enumeration succeeds with no ensure and no crash (A8's stated evidence).

5. **Cache identity checked before enumeration; whole-snapshot atomic replace.** A consumer compares
   `_CacheIdentity` first and enumerates only on a change; a replace swaps the whole snapshot, never
   patches one in place.
   → *verify:* Layer 1 — a reader holding a snapshot across a replace observes the **old** snapshot's
   exact contents for as long as it holds it.

### Gate 6A → 6B

- All five steps green → **proceed**.
- Any needed datum cannot be expressed as a value (it seems to require a handle or a shared field
  reference) → **STOP**, record in PROGRESS.md § Blockers, end session. Do not widen the snapshot
  contract.
- Anything else → **STOP, record in PROGRESS.md § Blockers, end session.**

---

## 5. Sub-phase 6B — the in-world draw module (F2.9, draw half)

[NN-D8f] rules `CkNavmeshDebugDraw` **retired, not rewritten** — this module is its replacement, and
the Recast draw module's deletion belongs to **PHASE_8**, not here.

1. **Create the draw module at Runtime tier** and add its `.uplugin` row. It must work in a packaged
   **Development** and **Test** build; that is what "Runtime" buys and is why the tier is not a
   detail.
   → *verify:* the `.uplugin` module row (A8's stated evidence) **plus** a green packaged build —
   the build is the real proof, the row alone is a claim.

2. **Draw modes**: walkable cells, plates (by id / by area tag / by clearance), portals, boundary
   segments, layer separation, reachability components, links, and paths (**corridor vs
   string-pulled**, distinguishable).
   → *verify:* Layer 1 where the geometry is computable headlessly (counts and extents per mode);
   `[EDITOR-VERIFY]` §4.1 for the visual verdict.

3. **Draw through `CkPmg`'s retained tier** — `Create_DebugLineSet` + `Append_Debug*_World` for
   plate outlines, portals, boundary segments, corridors, and paths, **chunking indefinitely growing
   streams**. Do not hand-roll a second retained-geometry mechanism; `R4 §3` identifies this tier as
   right for exactly this, and CkCrowdDebugger already depends on CkPmg.
   → *verify:* Layer 1 — the retained set is rebuilt only on a cache-identity change (assert on a
   rebuild counter across N frames with a static field).

4. **Failure renders as a status.** Every failure mode draws its status; **none** draws an empty
   scene.
   → *verify:* Layer 1 — one assertion per status; `[EDITOR-VERIFY]` §4.2 step 6 (kill the field
   mid-session: *building*, then *current*, and **never** an empty scene at any point).

5. **A console toggle** for the field overlay in packaged Development builds.
   → *verify:* `[EDITOR-VERIFY]` §4.4 step 3.

### Gate 6B → 6C

- Steps green and the module's tier confirmed by a green packaged build → **proceed**.
- The draw module appears to need a dependency on a DeveloperTool- or Editor-tier module →
  **STOP**, record, end session. That inversion is the exact failure §9 fences against.
- Anything else → **STOP, record in PROGRESS.md § Blockers, end session.**

---

## 6. Sub-phase 6C — CkCrowdDebugger adapter integration (F2.10)

**Adapter changes only.** The collector reads `Get_SurfaceBounds` / `Get_ProviderHealth` /
`Get_SurfaceRevision` from the facade. Snapshot boundary rules are unchanged: copy values; never a
`UWorld`, actor, handle, field, or producer.

1. **Migrate `CkCrowdDebugger_DataCollector.cpp:169-482` and `Types.h:133`** (GetBounds viewport fit)
   and the NavmeshStatusPanel's "UNavigationSystemV1 OK" string to **neutral surface bounds +
   provider health**. Nav-adjacent views must keep working on **either** provider.
   → *verify:* `Ck.CrowdDebugger.Viewport3d.*` green **against the recorded entry pass/fail set**;
   the viewport fit is correct on both providers.

2. **Preserve the compatibility surface**: the `ck.CrowdDebugger.PathNetworkTrace` token and all
   specialized source/CVar/detail-panel controls survive the change; common controls stay
   capability-driven and are **not** recreated in the Crowd window.
   → *verify:* `rg --no-ignore -n 'CrowdDebugger.PathNetworkTrace' Plugins/CkGameplayDebugger` still
   resolves; the control set is unchanged in the diff.

3. **Render the shadow-parity surface** (VALIDATION §3.3): agreement counters, delta distributions,
   the list of diverging query identities, and a per-query overlay drawing **both** providers' paths
   in distinguishable colours, sourced from the value-only diagnostics fragment (F1.40) — not from
   live state.
   → *verify:* Layer 1 — the panel renders from a diagnostics fragment constructed in isolation;
   `[EDITOR-VERIFY]` §4.2 steps 3–5.

4. **The two overlays must agree.** The debugger-driven field overlay and the gym-driven one draw
   identical geometry.
   → *verify:* `[EDITOR-VERIFY]` §4.2 step 5 — an explicit human comparison, recorded.

### Gate 6C → 6D

- `Ck.CrowdDebugger.Viewport3d.*` matches the entry set **exactly** — same passes, and the same
  single known `Nav.Filter.Customer` failure and no other → **proceed**.
- A **second** failure appears in that family → **STOP**, record both failures by name, end session.
  Never fold a new failure into the known baseline one.
- The adapter appears to need a Crowd-owned preview scene, viewport client, camera, or input router
  → **STOP**, record, end session (CkCrowdDebugger `CLAUDE.md` forbids it).
- Anything else → **STOP, record in PROGRESS.md § Blockers, end session.**

---

## 7. Sub-phase 6D — runtime gameplay-debugger category (F2.13)

1. **Add the category in the CkGameplayDebugger runtime tier**, reading **value-only diagnostics
   fragments** (the same shape F1.40's shadow diagnostics use). Per selected agent: active provider,
   agent profile, current path status, waypoint index, last query time, current area tags, and
   whether the path is flagged for repath.
   → *verify:* Layer 1 — the category's data source is a value fragment (grep + the same no-handle
   assertion as 6A); `[EDITOR-VERIFY]` — toggles and renders in PIE **and** in a packaged Development
   build (§4.4).

2. **Teardown clean.** Closing the category (and closing the debugger window while PIE runs) leaves
   no ensure and no retained reference to a dead world.
   → *verify:* the teardown test of VALIDATION §5 plus `[EDITOR-VERIFY]` §4.2's fail-signature list.

### Gate 6D → 6E

- Green and clean → **proceed**. Any ensure on teardown or on window close → **STOP**, record, end
  session. Anything else → **STOP, record in PROGRESS.md § Blockers, end session.**

---

## 8. Sub-phase 6E — the robust gym (F2.12)

VALIDATION §2.3 is the checklist; it is reproduced here as steps, not summarized.

1. **Create the gym in `Script/CkGroundNav/` and register it** in
   `Script/Common/CkTests_GymRegistry.as` via `RegisterProjectGym`. One class per `.as` file; never
   rename a test/gym class casually (a rename orphans the placed wrapper actor in the map).
   → *verify:* the gym appears in the Tab list; `Ck_Gym_List` enumerates it.

2. **Steps are CkStateMachine graphs** — one `UCk_Gym_StepState` subclass per step, dwell gated by
   `UCk_Gym_Dwell`. The superseded `AutoStep % TotalSteps` if-else shape is **not** used, so the HUD
   highlight cannot drift from what is running.
   → *verify:* `Ck_Gym_GoTo` reaches every step; the HUD highlight matches the live state.

3. **Adopt the shared control panel** — `Get_ControlRows()` declares rows,
   `Request_ControlActivated()` acts on the index, rows are **rebuilt each frame** so every value
   column reads back **live state**. Mirror into a member only where there is genuinely no readback,
   and say so in a comment there.
   → *verify:* changing field state outside the panel updates the panel's value columns within a
   frame.

4. **Reserved keys Tab and H are not bound** by any row; disabled rows stay **visible and muted** so
   readiness changes never reorder activation indices.
   → *verify:* a row-index assertion across a readiness change.

5. **Minimum control rows** (all of them): provider select (Recast / CkGroundNav / shadow); field
   overlay toggle (plates, portals, clearance, layers, boundary segments); paint-markup + clear;
   rebake/repair; spawn-agents cycle; path-draw toggle; a status row showing **epoch, tile count,
   plate count, provider health**.
   → *verify:* each row present and functional in the `[EDITOR-VERIFY]` walkthrough.

6. **Stations**, at minimum: flat open ground; two-storey overlap; ramp; staircase; a narrow gap near
   the clearance threshold; a moved obstacle; a painted markup region; a multi-tile crossing; a
   no-route failure.
   → *verify:* every station reachable via `Ck_Gym_GoTo`.

7. **`[EDITOR-VERIFY]` VALIDATION §4.1** — the full 13-step walkthrough, its result and its **fail
   signatures** recorded in PROGRESS.md with the verifier and the artifact.
   → *verify:* the recorded human result. An agent cannot perform this and must not claim it.

### Gate 6E → 6F / phase exit

- Walkthrough recorded with no fail signature → **proceed**.
- Any fail signature from §4.1 step 13 (agents sinking/floating; paths hugging walls; a path crossing
  a portal narrower than the agent; the overlay going empty rather than showing a status; the epoch
  bumping every frame) → **STOP**, record the observation verbatim, end session. These are defects in
  earlier phases surfacing, not gym bugs to work around.
- Anything else → **STOP, record in PROGRESS.md § Blockers, end session.**

---

## 9. Sub-phase 6F — PerfLab seeding migration (F2.11)

1. Migrate `CkPerfLab_WorldSurvey_Builder.cpp:57-250` to F1.21's point generators **through the
   facade**, with a **caller-supplied RNG seed** so PerfLab runs stay reproducible.
   → *verify:* Layer 1 — identical seeded output across runs and across field epochs where the seed
   and epoch are both fixed; `[EDITOR-VERIFY]` — a PerfLab survey produces a comparable agent
   distribution on both providers.
2. Record the survey numbers this produces in PROGRESS.md — **PHASE_8's B4 budgets consume them**.
   → *verify:* the recorded numbers name their artifact and their date.

---

## 10. Exit criteria (measurable)

- [ ] **Targeted families green** with `--parallel 1`: `Ck.GroundNav.*` (snapshot, draw), and
      `Ck.CrowdDebugger.*` re-run as the **full serial `Crowd` family** after adapter changes
      (CkCrowdDebugger `CLAUDE.md` requires this), matching the entry pass/fail set exactly.
- [ ] **Full suite delta-zero** vs the phase-entry baseline, on the **final** artifact.
- [ ] **Zero new ensures, zero new warnings**; a fresh editor startup log inspected.
- [ ] **A8 evidence complete**: value-only snapshot contract asserted (grep + torn-down-producer
      test); every failure mode renders as a status, never an empty draw; whole-snapshot atomic
      replace with cache identity checked before enumeration; **draw module = Runtime tier, debugger
      window = DeveloperTool tier, editor half = Editor tier**, evidenced by the `.uplugin` rows and
      a green packaged build; gym registered and functional.
- [ ] **`[EDITOR-VERIFY]` §4.1, §4.2 and §4.4 recorded** with verifier and artifact. §4.4 runs
      **twice** — once against a packaged **Development** build, once against a packaged **Test**
      build — and both are build-machine runs (see §11).
- [ ] **Three environments** for every public API added.
- [ ] **Provenance cells** non-empty (this phase's features claim no algorithm — record "no
      algorithmic claim; our own three-tier debugger data-flow pattern" rather than leaving the cell
      empty; an empty cell blocks the gate).
- [ ] **Comment audit** run; PROGRESS.md updated; PHASE_7 re-verified at the boundary.

---

## 11. Fences (each with its reason)

- **Module tier is the highest-risk mistake in this phase.** Field + draw = **Runtime**; the debugger
  window = **DeveloperTool**; editor halves = **Editor**. Runtime code must never depend on an
  editor-tier module. Getting this wrong looks perfect in PIE and fails only in a packaged build —
  which is precisely why the evidence for the tier is a **green packaged build**, not the `.uplugin`
  row.
- **No raw pointers in snapshots — and no handles, `UObject`s, `UWorld`s, actors, registries, or
  shared field structures either.** A snapshot outlives its producer by design; anything live inside
  it is a use-after-free or a world-leak with a debugger holding the reference. Assert it, do not
  assume it.
- **Never hide a new failure behind the known inherited `Nav.Filter.Customer` baseline failure**
  (`CkCrowdDebugger/CLAUDE.md:29-30`). It is recorded by name at entry so a second red in the same
  family is unmistakable. Two reds is a STOP, not "the known one, plus".
- **Cook and packaging are build-machine-only — do not cook or package locally.** A local cook races
  the editor for `Saved/`/`Intermediate/` and produces an artifact no gate is defined against. The
  §4.4 packaged Development/Test verifications are requested from the build machine and their
  artifact identity is recorded.
- **Shipping-configuration builds require explicit maintainer approval** in chat before anyone
  requests one. This phase's gates need Development and Test, not Shipping.
- **Do not add a CkGroundNav- or Crowd-owned `FPreviewScene`, viewport client, camera, or input
  router.** The viewport is a thin facade over `SCkDebug_3dPreviewViewport`; reusable mechanics live
  in **CkDebugScene**. A second implementation is a maintenance fork with no owner.
- **Never substitute Engine `M_Simple*` materials** for the shared Unlit translucent CkDebugScene
  material — they lack the retained-ISM shader-usage contract.
- **`CkNavmeshDebugDraw` is not deleted in this phase.** It is retired in PHASE_8 under the
  retirement sign-off ([NN-D8f]); deleting it here removes the rollback path before promotion.
- **Never edit source, `Script/`, or config while a build or test run is in flight**; and new AS
  autotests need `--discover-fresh` — a green run whose Total matches the old count is stale-green.
- **Commits, pushes, submodule pointer bumps, and PRs only when the maintainer asks.** This phase
  spans two submodules (CkFoundation and CkGameplayDebugger); stage only the paths you authored, and
  report dirty paths you did not.

---

## 12. [P6] Done means

**[P6] is done when**, and only when:

1. `VALIDATION.md` **A8** is fully checked with evidence in PROGRESS.md: the value-only snapshot
   contract (grep + torn-down-producer test), failure-as-status for every failure mode,
   whole-snapshot atomic replace with cache identity checked before enumeration, the three module
   tiers evidenced by `.uplugin` rows **and** a green packaged build, and the gym registered and
   functional per §2.3.
2. `VALIDATION.md` **§2.3** is fully checked — every gym requirement, not a subset — and
   **`[EDITOR-VERIFY]` §4.1** is recorded with its verifier and artifact and carries **no** fail
   signature.
3. `VALIDATION.md` **§4.2** (debugger visual parity in PIE) and **§4.4** (packaged Development **and**
   Test) are recorded with verifier and artifact.
4. Every **§0 standing gate** is green for this phase: entry baseline captured, phase-exit full suite
   delta-zero on the final artifact, zero new ensures/warnings, clean editor boot with regenerated AS
   bindings, three environments, Provenance rows complete, no forbidden-source language.

A gym that looks right is a **precondition** for this phase's human checks and is **never** parity
evidence for any later gate — parity is PHASE_8's shadow report over the full suites.
