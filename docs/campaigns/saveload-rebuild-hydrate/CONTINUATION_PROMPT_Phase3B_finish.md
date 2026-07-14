# Continuation — CkSnapshot rebuild+hydrate, FINISH Phase 3B (then 4A → 4B → 5)

**One-line:** Phase 3B's load pipeline is **IMPLEMENTED, compile-clean, and PROVEN end-to-end** (`Ck.Snapshot.M2b.LevelReload`
PASSES through v3). It is **UNCOMMITTED on disk**. Your job: read the **gate-2** result, finalize the Phase-4 casualty
categorization, add one cheap test, **COMMIT 3B**, write the 4A continuation prompt, then run **4A → 4B → 5** to completion.
You are **Opus**; stay Opus for implementation; route design forks through a **Fable** agent and verify against code.

## 0. READ THESE FIRST (in order)
1. `docs/campaigns/saveload-rebuild-hydrate/PROGRESS.md` — canonical tracker. Read **§Phase-3B progress** (RESEARCH
   CHECKPOINT + IMPLEMENTATION STATE + **GATE-1 RESULT** + **DIAGNOSIS COMPLETE**), the **Unattended execution protocol**,
   and **§Decisions [P3B-F1..F4], [P3B-D1..D5]** (all Fable-verified against code).
2. `docs/campaigns/saveload-rebuild-hydrate/PHASE_3B.md` (the phase), then `PHASE_4A.md` / `PHASE_4B.md` / `PHASE_5.md`
   + `VALIDATION.md` at each phase start. `docs/specs/2026-07-10-CkSnapshot-rebuild-hydrate-design.md` §4.2–§4.5 is the authority.
3. `*.md` is gitignored — campaign docs are **force-added** (`git add -f`). Source unaffected.

## 1. Repo state (verify at start)
- CkFoundation `feature/save-load-improvements` HEAD = **`57fee2671`** (docs). **10 source files + PROGRESS modified,
  1 new file (oracle-allowlist-p3.txt) UNTRACKED — ALL UNCOMMITTED.** `git -C Plugins/CkFoundation status --short`:
  - CkEcs: `CkEntityScript_Utils.cpp`, `Snapshot/CkSnapshot_Context.{h,cpp}`, `Subsystem/CkEcsWorld_Subsystem.{h,cpp}`
  - CkEcsExt: `EntityScript/CkEntityScript_WithActor.cpp`
  - CkSnapshot: `SaveGame/CkSnapshot_Header.h`, `Snapshot/CkSnapshot_CaptureV3.cpp`, `Subsystem/CkSnapshot_Subsystem.{h,cpp}`
  - docs: `PROGRESS.md` (M), `oracle-allowlist-p3.txt` (??)
- CkTests `dev` HEAD = **`5bb9798`** — CLEAN (the new V3.InstancedStructDiskSmoke test is DRAFTED in the scratchpad, not applied:
  `<scratchpad>/v3_disksmoke_test_draft.cpp`).
- **Build is CLEAN** (p3b-compilecheck1.log: 595s, Result: Succeeded, 0 errors). Editor CLOSED. NOTHING pushed.

## 2. What 3B built (all UNCOMMITTED — verify the diff before committing)
**3B.1 — v3 rebuild+hydrate LOAD pipeline** (the load path now consumes v3; Model A kept compiled for oracle/registry tests):
- **Fork A** (`CkSnapshot_Context.{h,cpp}`): map-backed `FSnapshotContext(const TMap<uint32,FCk_Handle>*, FCk_RegistryHandle)`
  mode — on load, remaps each raw saved-id in a blob to its live handle (missing/sentinel → invalid). [P3B-F2], verified.
- **Save-side D3a** (`CkSnapshot_Header.h` + `CkSnapshot_CaptureV3.cpp`): new `FTransform _ActorSpawnTransform` on
  `FCk_Snapshot_V3_EntityEntry`, captured for bridged RuntimeSpawned so the loader spawns the actor at its saved
  position (WithActor::Construct seeds the entity Transform from the actor — NO Transform-Produce, which SyncFromActor
  would stomp). [P3B-F1a], verified.
- **Load SM** (`CkSnapshot_Subsystem.{h,cpp}`): `ELoadPhase{Idle,TearingDown,AwaitingWorld,Rebuilding,Hydrating,Settling}`.
  v3-only hard break (v2 → Failed_IncompatibleSave; corrupt → Failed_Corrupt). `DoRebuild_Tick`: EngineOwned SaveKey/player
  rendezvous · ConstructSpawned adopt-by-label via LifetimeDependents walk ([P3B-F3]) · RuntimeSpawned bridged = actor-first
  spawn + `TryGet_ActorEntityHandle` ([P3B-F1]) · non-bridged = `Request_SpawnEntity` under mapped owner, **or SKIP if
  transient/root-owned (boot-infra, [P3B-D5])**. **Hydrating is ATOMIC** (enqueue payloads + reconcile + gate-off in ONE
  callback → hydration drains only in post-gate FULL passes where Setup precedes it → no Setup-stomp). Reconcile =
  subtractive `Request_DestroyEntity` of stray labeled ConstructSpawned children ([P3B-F4]). **Progress-based early-exit**
  (proceed after 30 no-progress ticks — kills the 600-frame stall). `_PersistedIds`/`_SkippedIds` track boot-infra skips.
**3B.2 — reconstitution retirement** (`CkEcsWorld_Subsystem.{h,cpp}`, `CkEntityScript_Utils.cpp`, `CkEntityScript_WithActor.cpp`):
deleted `ECk_ReconstitutionPhase` + accessors + `_ReconstitutionPhase` + `DoIs_WorldReconstituting` + call sites + all
CkSnapshot stamp sites. **KEPT `Get_IsSnapshotRespawnable`** ([P3B-D1] — it is the ActorSpawnIntent opt-in, not only a
reconstitution gate; deleting it breaks the gate). `rg "Reconstitution" Source` → **0**. Retiring was a PREREQUISITE, not
cleanup ([P3B-D2]: a stamped phase suppresses the loader's actor-triggered entity spawns).

## 3. PROVEN + DIAGNOSED (the load pipeline WORKS)
- **`Ck.Snapshot.M2b.LevelReload` PASSES** (p3b-diag-m2b.log) — bridged actor-first rebuild + ConstructSpawned adoption
  (all attribute meta-fragment children mapped, **0 unresolved ConstructSpawned**) + position + hydration. Core is SOUND.
- **Root causes of the gate-1 reds, understood + partly fixed:**
  - *12/19 unmapped = world-boot infrastructure* (`Bb_DayNightLampDriver`, `Bb_MusicDirector`, generic WithActor — the
    CkTests gate runs in the full BB editor). Transient-owned; the fresh world's boot recreates them → loader must SKIP
    them. **FIXED** ([P3B-D5]). The boot-infra-vs-gameplay-top-level discriminator is the **CTO-N1 problem → Phase 4A**.
  - *600-frame stall → tests timed out at 240 frames.* **FIXED** (progress-based early-exit).
  - *Attribute/AnimPlan values not restored* = **empty-seed Produce** ([P1-D2], verified `CkAttribute_RestorePersistence.h`) —
    no value in v3. Value-emitting features (Velocity, Acceleration, TagSet) DO round-trip. Making attribute Produce
    value-emitting is non-trivial (per-owner upsert-merge) → **Phase 4B**.

## 4. YOUR IMMEDIATE STEPS (finish 3B, then commit)
**GATE-2 IS DONE (p3b-gate2.log): BUILD CLEAN; Ck.Snapshot 41 pass / 10 fail. ALL 10 reds diagnosed as legitimate Phase-4
casualties (see PROGRESS §GATE-2 RESULT) — NONE is a load-pipeline bug.** M2b/M2b2a now PASS. DIAG clean (rebuild 2 frames,
19 skipped boot-infra, 0 orphaned).
1. **The definitive categorization (from gate-2 — do NOT re-derive; re-run only if you edit the load path):**
   - GREEN (load pipeline proven): all Model-A registry tests, M2b/M2b2a/M2b2b, `Parity.Acceleration/Velocity/TeamPlayer`.
   - **→ 4A (3):** `Parity.StateMachine`, `Parity.StateMachineNoHistory` (no Produce); **`M2a.LoadOrchestration`** (its subject
     is a NON-bridged transient-owned gameplay entity → skipped by the boot-infra heuristic [P3B-D5] → the CTO-N1 discriminator).
   - **→ 4B (7):** `Parity.Attributes`, `Parity.AnimPlan` (empty-seed Produce); `Parity.TagSet`, `Parity.GridPlacements`,
     `Parity.InventoryDataOnly`, `Parity.InventorySpatial`, `Parity.RenderTarget` (client-shaped Apply — stamp-a-sync-fragment,
     not authority-applied under hydration; value-emitting Produce is necessary but NOT sufficient).
   - A `PendingReplicationRetry timed out ... Ck_CueRelay` warning is the client re-materializing skipped boot-infra relays —
     it does NOT fail a non-casualty test (confirmed).
2. **Decide: commit 3B now, or push some coverage into 3B first.** RECOMMEND committing 3B (the load pipeline is the deliverable;
   all 10 reds are legitimately 4A/4B) and letting 4A/4B green them. If you'd rather shrink the red set in 3B, the two cheapest
   are TagSet's authority-side Apply (4B feature work) and the M2a/N1 discriminator (4A) — but both are genuinely Phase-4 scope,
   so committing-and-deferring is cleaner. Record the 10 casualties in §Blockers as Phase-4A/4B work items.
3. **Add the cheap test** — apply `<scratchpad>/v3_disksmoke_test_draft.cpp` into
   `Plugins/CkTests/Source/CkTests/Private/CkSnapshot/Test_Snapshot_V3_Capture.cpp` (append; add `#include "Serialization/MemoryWriter.h"`).
   It validates Fork A (map-backed remap + dangling-ref semantics), registry-level (no travel). Re-gate with `--discover-fresh`.
   The heavier PIE tests (Rebuild.NoDuplicateGrants / LostGrantStaysLost / OrphanHydrationLoud / OracleParity) are
   DEFERRED — record them as 3B follow-ups in PROGRESS; the existing Parity/M2b gate is the primary proof and the
   ConstructSpawned adopt+hydrate path is already covered via the attribute meta-fragment children. `oracle-allowlist-p3.txt`
   is created (empty — no BB driver world in the framework gate).
4. **COMMIT 3B** (stage BY NAME, never `git add <dir>`; no push; no Co-Authored-By):
   - CkF `refactor(CkEcs,CkEcsExt): retire reconstitution suppression machinery` — `CkEcsWorld_Subsystem.{h,cpp}`,
     `CkEntityScript_Utils.cpp`, `CkEntityScript_WithActor.cpp`.
   - CkF `feat(CkSnapshot): v3 rebuild+hydrate load pipeline (rebuild/hydrate/reconcile/settle)` — `CkSnapshot_Context.{h,cpp}`,
     `CkSnapshot_Header.h`, `CkSnapshot_CaptureV3.cpp`, `CkSnapshot_Subsystem.{h,cpp}`.
   - CkF `docs(CkSnapshot): Phase 3B progress + oracle allowlist` — `PROGRESS.md`, `oracle-allowlist-p3.txt`, this file.
   - CkTests `test(CkSnapshot): v3 InstancedStruct disk smoke` — the one new test. (CkTests stays BEHIND the CkF commits.)
   - ⚠️ Shared worktree: stage only YOUR files; a sibling session may be active.
5. **Update PROGRESS status board (Phase 3B → DONE w/ the annotated Phase-4 casualty list) + write
   `CONTINUATION_PROMPT_Phase4A.md`**, then proceed to Phase 4A.

## 5. Phases 4A → 4B → 5 (the annotated casualties are their work)
- **4A** (`PHASE_4A.md`, CkStateMachine): SM redrive-as-hydration (give CkStateMachine a value-emitting Produce + Apply so
  `Parity.StateMachine*` restore SM state) + **N1 closure**: the boot-infra-vs-gameplay-top-level discriminator ([P3B-D5])
  so gameplay RuntimeSpawned top-level entities respawn while boot-infra is skipped. Route the discriminator design to Fable.
- **4B** (`PHASE_4B.md`, per-feature): value-emitting Produce for the empty-seed features (Attributes ×5 — mind the per-owner
  upsert-merge; AnimPlan), the deferred-six client-shaped Apply (Grid/Inventory×2/Team/Player/2dGrid), RenderTarget re-author,
  MontagePlayer rebind, the 8 params-mutators, AS smoke matrix.
- **5** (`PHASE_5.md` + `VALIDATION.md`): decommission Model A (gate-not-delete registrations under `CK_WITH_FIDELITY_ORACLE`),
  full acceptance.

## 6. Gate + gotchas
```
CkAuto\UnrealToolbox.exe --build --test --test-pattern "Ck.Snapshot" --discover-fresh --output CkAuto\logs\<name>.log   # editor CLOSED
CkAuto\UnrealToolbox.exe --test --test-pattern "Net" --discover-fresh --output CkAuto\logs\<name>-net.log
```
- Real verdict = `Total time in Unreal Build Accelerator ...` (build) + tail `Result: Succeeded/Failed`, and
  `**** TEST COMPLETE. EXIT CODE: N ****` (test). The toolbox prints a premature `Result: Succeeded` for a sub-step — ignore it.
- `--discover-fresh` after adding/removing a test. Read verdicts from the `--output` log, not the notification.
- Baselines: Model-A registry tests + value-emitting parity + orchestration MUST stay green. `Net` framework `Ck.*.Net`
  delta-zero + recorded kiosk env-red trio + StateMachine.Net flake (ignorable). 3 Attribute.Net two-signal pins MUST stay green.
- Fable consults: `Agent`, `model:"fable"`, read-only, exact file:line anchors, VERIFY the ruling against code yourself.

## 7. Ruled out / don't re-investigate
- Reconstitution retirement is REQUIRED (not cleanup) — [P3B-D2]. Keep `Get_IsSnapshotRespawnable` — [P3B-D1].
- Transform via recipe `_ActorSpawnTransform`, NOT Transform-Produce (SyncFromActor stomps) — [P3B-F1a], verified.
- Bridged rebuild = actor-first (loader spawns actor → bridge re-creates entity), `FProcessor_ActorRespawn` dormant — [P3B-F1].
- ConstructSpawned adopt via LifetimeDependents walk (NOT the compile-time template) — [P3B-F3].
- Empty-seed attribute Produce ⇒ values don't round-trip in v3 (Phase 4B) — verified `CkAttribute_RestorePersistence.h`.
- Model A stays compiled until Phase 5.
