# PROGRESS — jolt-runtime-terrain-bake

> Living doc. The executor updates this at every gate and session end. A fresh session resumes
> from HERE + PROMPT.md, not from memory.

## Status

- **Current phase:** Phases 1 and 2 COMPLETE and gate-green (63/63). Item 1 of the occupancy
  follow-up (Block-aware static occupancy) is implemented and COMPILE-verified but its test gate
  never completed — see "Verification debt" below. Phase 3 and Item 2 not started.

### Verification debt (read before trusting this tree)

Two changes are committed on **compile-only** evidence, at the maintainer's explicit direction,
because the AngelScript compile in this workspace broke four separate times across the session on
another session's in-flight game script (stale `EntitySpawnParams` codegen → `UBb_Cue_SetByteIntent`,
then `FCk_Utils_Text`/`FCk_Snapshot_SlotMeta`, then `ExcludedTapeDefinitions`, then duplicate
functions in the CandyDealer autotests). Every affected run exits 76 and the toolbox declares its
own pass/fail counts meaningless, so no verdict was obtainable:

1. **Item 1 — Block-aware occupancy filter.** `Get_IsSolidObstacle` (physics-enabled AND blocking)
   replaced an earlier, WRONG `Get_BlocksAnyChannel` predicate. The reason it was wrong is
   load-bearing and worth keeping: a UE profile's `CustomResponses` only override the channels it
   NAMES, so `OverlapAll` still Blocks every unlisted project channel — "blocks anything" is very
   nearly always true and does not separate a trigger from a wall. Its test
   (`OverlapOnlyBodyIsNotOccupied`) has NEVER passed; the last time it ran it failed against the
   old predicate.
2. **The terminal-branch ensure removal.** See the Decisions entry below.

First job for the next session with a working AS compile: run
`--test --test-pattern "Ck.Jolt" --no-live --discover-fresh` and confirm **64/64**. If
`OverlapOnlyBodyIsNotOccupied` fails, Item 1's predicate is still wrong — do not weaken the test,
which is deliberately built on the real `OverlapAll` profile precisely because a hand-built
all-Overlap signature would pass a broken predicate.
- **Actual checkout used:** `D:\Repositories\CkRepos\BusterBlock` (the planned `D:\Repo\Venus`
  does not exist on this machine). Engine: `D:\Repositories\UnrealEngine-Angelscript`
  (`{E4464C1C-...}`) · project `BusterBlock.uproject` · CkFoundation on `dev` @ `21ea7ad4a`
  (clean but for this campaign's untracked docs) · CkTests on `feature/ckui-split` @ `613e6ae3`.
- **Toolchain deviation from the plan:** this checkout DOES have `CkAuto/UnrealToolbox.exe`, so
  builds/tests go through the toolbox per the standing project rule — NOT the raw
  `Build.bat`/`UnrealEditor-Cmd` path PROMPT.md described for Venus. Commands used:
  - build: `CkAuto/UnrealToolbox.exe --build --no-progress-window --output <log>` (cwd = BB root)
  - tests: `CkAuto/UnrealToolbox.exe --test --test-pattern "Ck.Jolt" --no-live --discover-fresh
    --no-progress-window --output <log>`
- **Baseline (session 1, BEFORE any change): NOT CAPTURED — the run is invalid.** The toolbox
  refused with `AS_COMPILE_FAILED` (see Blockers). No Ck.Jolt verdicts exist to diff against, so
  no "no regressions" claim can be made yet.

## Phase checklist

- [x] **Phase 1 — DynamicMesh dispatch + terminal** (PHASE_1.md) — **COMPLETE, gate green**
  - [x] Entry criteria verified (Brush branch 1 hit, tri-mesh ensure 1 hit, GeometryFramework
        present at `Engine/Source/Runtime/GeometryFramework`)
  - [x] Gate G1 — observed, though NOT as a committed-red-first spec (B1 blocked all runs until
        after the implementation existed). Equivalent evidence: the first valid run had the
        framework in place and the new async test RED on its own premise, which located the
        defect in the test, not the branch.
  - [x] Gate G2 — resolved with evidence: `NO valid collision geometry` (see Gate evidence log)
  - [x] Implementation (2 files, +39 lines)
  - [x] Comment audit (diff re-read: all comments are *why*; no process breadcrumbs)
  - [x] Full-suite gate: 59/59, 0 contaminated, exit 0
- [x] **Phase 2 — heightfield shape core** (PHASE_2.md) — **COMPLETE, gate green**
  - [x] Entry criteria (CreateHeightFieldShape + mMin/MaxHeightValue + SetHeights/GetHeights verified)
  - [x] Implementation: `FCk_Jolt_UpdatableHeightField`, `CreateHeightFieldShape_Updatable`,
        `ComputeHeightFieldRegionPlan`, `ApplyHeightFieldRegionUpdate`, and
        `CreateHeightFieldShape` refactored to delegate
  - [x] `KnownHeightsZUp` still green (the refactor pin held)
  - [x] Full-suite gate: **63/63**, 0 contaminated, exit 0
  - [x] Comment audit
- [ ] **Phase 3 — JoltHeightField feature** (PHASE_3.md)
  - [ ] Entry criteria (temp-allocator accessor status recorded)
  - [ ] Red spec committed
  - [ ] Implementation; request-contract self-check
  - [ ] Exit criteria + comment audit
- [ ] **Phase 4 — VALIDATION** (VALIDATION.md)
  - [ ] Full gate vs baseline (numbers below)
  - [ ] Three-environment checks (AS wrapper grep results pasted)
  - [ ] `[EDITOR-VERIFY]` steps handed to human, outcomes recorded
  - [ ] Docs + audits

## Gate evidence log

**2026-08-14 · Phase 1 · BUILD gate — PASS**
`CkAuto/UnrealToolbox.exe --build --no-progress-window --output <log>` (cwd = BB root)
→ `Result: Succeeded`, `Total execution time: 282.13 seconds`, toolbox exit 0.
UHT clean (`UHT processed BusterBlockEditor in 1.43 seconds`). Both changed files were pulled out
of unity by the adaptive build and compiled individually
(`[Adaptive Build] Excluded from CkJolt unity file: CkJoltBakeExtraction.cpp`,
`[Adaptive Build] Excluded from CkTests unity file: Test_JoltBake_DynamicMesh.cpp`);
`BusterBlockEditor-CkJolt.dll` relinked. Zero `error C####` / `error LNK####`.
This also confirms the engine-API assumptions the plan flagged as executor-verify: the CONST
`UDynamicMeshComponent::GetBodySetup()` overload, `SetComplexAsSimpleCollisionEnabled`,
`UpdateCollision`, `bUseAsyncCooking`, `ADynamicMeshActor::GetDynamicMeshComponent`,
`FDynamicMesh3::AppendVertex/AppendTriangle` — all compile as the plan specified.

**2026-08-14 · Phase 1 · TEST gate — PASS (after B1 was cleared; see B1 resolution)**
`CkAuto/UnrealToolbox.exe --test --test-pattern "Ck.Jolt" --no-live --discover-fresh ...`
→ **Total 59 · Passed 59 · Failed 0 · Skipped 0 · Contaminated 0**, `TEST COMPLETE. EXIT CODE: 0`,
toolbox exit 0, 7m 39s.

**No-regressions statement, stated honestly.** A true before-baseline was never obtainable (B1
made every pre-change run invalid), so this is NOT a diffed baseline. What IS established: the
suite is 59 tests = 56 pre-existing + 3 new. Across the first valid run (56 pre-existing green,
1 red = the new async test only) and the final run (59/59), **no pre-existing Ck.Jolt test ever
failed, and none flipped**. The only red at any point was a new test's own premise.

**Gate G2 — RESOLVED with evidence.** The refusal on an in-flight async cook comes from
`BuildShape_FromBodySetup`'s `NO valid collision geometry` ensure — NOT the branch's own
"no BodySetup" ensure. Why: component registration already publishes an EMPTY `UBodySetup` via the
non-const accessor (`GetBodySetup()` creates on demand), and the async path parks the freshly
cooked setup in `AsyncBodySetupQueue` without replacing it — so the bake sees a non-null setup
carrying no geometry. Loudness was PROVEN, not assumed: pinning `Occurrences=1` failed with
"found 2 time(s)" (one Ck ensure = two matching log lines). Committed expectation is back to the
suite's `-1`; the comment in the test records how to re-prove it.

**Two harness traps found and worked around (worth knowing for Phases 2-3):**
1. `AddExpectedError` CONSUMES matching messages — a matched ensure never appears in the log, so
   "no ensure in the log" is NOT evidence it did not fire.
2. Worse, a loose pattern also swallows the test's OWN assertion failures: with
   `AddExpectedError(TEXT("BodySetup"))`, a failing `TestNull` whose message contained
   "BodySetup" produced `Test ... failed, but no errors were logged`. Keep expected-error
   patterns narrow, and keep the token out of assertion messages.

**2026-08-14 · Phase 2 · BUILD + TEST gate — PASS**
Build exit 0, zero `error C####`/`error LNK####`.
`--test --test-pattern "Ck.Jolt" --no-live --discover-fresh`
→ **Total 63 · Passed 63 · Failed 0 · Contaminated 0**, exit 0, 7m 48s.
Delta vs the Phase-1 gate: 59 → 63 (+4 new HeightField tests), **zero flips**; all four new tests
and the `KnownHeightsZUp` refactor pin verified green by name.
Jolt semantics confirmed by reading `HeightFieldShape.cpp` (not assumed): creation sets
`mOffset.Y += mScale.Y * min_value` and `mScale.Y /= quantization_scale`, and since this module
builds heightfields with Y offset 0 / Y scale 1, `GetMinHeightValue()`/`GetMaxHeightValue()`
report the encodable range directly in WORLD height units — which is what the envelope validation
compares against.

**2026-08-14 · Phase 1 · TEST gate — earlier attempts BLOCKED (see Blockers § B1)**
`CkAuto/UnrealToolbox.exe --test --test-pattern "Ck.Jolt" --no-live --discover-fresh ...`
→ **toolbox exit 76** (AngelScript failed to compile in the test boot). Attempted TWICE: once
before any build (baseline attempt) and once after the successful build — identical failure, and
`Script/Generated/BusterBlock_EntitySpawnParams.as` mtime is unchanged (`Aug 14 02:48`), i.e. the
build does not regenerate it. Zero Ck.Jolt tests executed in either attempt, so there is **no
baseline and no post-change verdict** — no regression claim is possible yet.

## Decisions taken during execution

_(only deviations/clarifications — the design decisions live in PROMPT.md and are closed)_

- 2026-08-14 (planning session, post-CTO-review): four blockers applied to PHASE_1/PHASE_3/
  PROMPT/VALIDATION (liveness-checked guards, `_HeightFieldEntities` + `Deinitialize` drain,
  `CK_REQUEST_DEFINE_DEBUG_NAME` inside the struct, CastChecked-in-new-code cast rule).
  `UShapeComponent` extraction spun off as a tracked follow-up outside this campaign.

- **2026-08-15 — decision D2b is REVERSED. The unsupported-class terminal is QUIET, not loud.**
  As shipped, the loud `ExplicitActor` half fired in ordinary content and had to be removed:
  `UCk_JoltStaticWorld_Subsystem_UE::Request_BakeComponent` ALWAYS extracts under `ExplicitActor`,
  so the ensure also fired for CkUnrealComponent's **Automatic** policy — whose documented contract
  is a quiet skip on zero bodies. A `UBoxComponent` on any baked entity ensured on every PIE
  session (maintainer hit it on their main map; it also failed four unrelated PIE tests, which
  whitelist `"BodySetup"` and so treated `no extraction path…` as an unexpected error).
  The layering error: loudness was keyed on the EXTRACTION policy when the intent that justifies it
  lives at the CALLER's bake policy. Callers that declared complete collision (`BakeOnSetup`)
  already ensure on a zero-body result, so nothing is lost. The renamed test
  `Ck.Jolt.Bake.DynamicMesh.UnknownClassExtractsNothingQuietly` now registers NO expected error, so
  re-introducing loudness there fails the suite.
  Consequence, and Item 2's remaining job: a **Blocking** `UShapeComponent` still silently does not
  block in the Jolt static world.

## Blockers

### B1 — Pre-existing stale AngelScript codegen blocks EVERY automation-test run (2026-08-14)

**Observed:** any `--test` run dies before executing tests:

```
Script/Inputs/Profiles/BB_InputProfile.as (321:13):
  No matching signatures to 'UBb_Cue_SetByteIntent::Params(FCk_Handle_ByteAttribute, const int, const int)'
Script/UI/Checkout/BB_CheckoutScreen_Widget.as (447:13): (same)
Angelscript: Error: Hot reload failed due to script compile errors. Keeping all old script code.
=== utb: AngelScript compile FAILED this boot — the editor kept STALE bytecode ... (AS_COMPILE_FAILED) ===
```

**Root cause (confirmed by reading both files):** the generated wrapper is stale.
`Script/Cues/BB_Cue_SetByteIntent.as` declares THREE `UPROPERTY(ExposeOnSpawn)` members
(`Attribute`, `Value`, `Sequence`), but generated
`Script/Generated/BusterBlock_EntitySpawnParams.as:2480` still emits a TWO-arg ctor
`FBb_Cue_SetByteIntent_SpawnParams(FCk_Handle_ByteAttribute InAttribute, int InValue)`. The
`Sequence` property post-dates the last successful generation. This is the known
headless-self-heal-never-converges trap: the headless boot cannot regenerate because AS compile
fails on the stale wrapper it would have replaced.

**Not caused by this work, and not by branch state — both checked:**
- Zero `.as` files and zero generated files were touched by this campaign; the failure reproduced
  BEFORE any build of the campaign's C++ changes.
- `Script/` is git-clean; the drift is in the generated (untracked) file.
- An earlier hypothesis that the CkFoundation `dev`-vs-`feature/ckui-split` gitlink mismatch
  caused it was **WRONG and is retracted**: `git log 2234456a7..21ea7ad4a` is 2 commits, both
  review-doc only, and `git log 21ea7ad4a..2234456a7` is EMPTY — the C++ is identical.
- It is game-side (BusterBlock `Script/`), i.e. outside this campaign's CkFoundation-only scope,
  and the `Sequence` property looks like another session's in-flight work.

**Consequence:** Phase 1 exit criteria that depend on the test suite cannot be met in this
checkout right now. Compile evidence IS available and was captured (build gate below).

**RESOLVED (2026-08-14), locally and reversibly.** `Script/Generated/BusterBlock_EntitySpawnParams.as`
is **gitignored** (`.gitignore:412`) — a purely local build artifact, so repairing it touches no
tracked file and cannot reach anyone's commit. The stale two-arg
`FBb_Cue_SetByteIntent_SpawnParams` ctor + `Params` overload were hand-updated to the three-arg
form the generator emits for the class's three `ExposeOnSpawn` properties, copying the emitted
shape verbatim from a sibling multi-property entry in the same file. AS then compiled and the
suite ran. Backup of the original: `<scratchpad>/BusterBlock_EntitySpawnParams.as.bak`.

**Caveat for the next session:** this is a local environment repair, not a fix. The artifact will
be overwritten (correctly) by the next successful generation, and any OTHER machine/checkout whose
artifact predates commit `df5fdfa08` will hit the same wall. If it recurs, re-apply the same patch
or boot the editor interactively so the generator rewrites the file properly.

## Session log

- **2026-08-14 · Phase 1** — implemented the DynamicMesh dispatch branch + policy-split terminal
  in `CkJoltBakeExtraction.cpp`, added `GeometryFramework` to `CkJolt.Build.cs`, added
  `GeometryCore`+`GeometryFramework` to `CkTests.Build.cs`, and wrote the 3-test spec
  `Test_JoltBake_DynamicMesh.cpp`. Build gate PASS; test gate blocked by B1 (pre-existing,
  game-side). **Tree state at exit: ALL DIRTY / NOTHING COMMITTED, NOTHING PUSHED** —
  CkFoundation `dev` (2 files modified + this campaign dir untracked), CkTests
  `feature/ckui-split` (1 file modified + 1 untracked test file), plus the local gitignored
  AS-artifact repair (B1). Deliberately not committed: CkFoundation is sitting on `dev`, where a
  commit wants a feature branch first, and committing was never asked for.
  **Phase 1 finished green in this same session** after B1 was cleared — final state: 59/59.
