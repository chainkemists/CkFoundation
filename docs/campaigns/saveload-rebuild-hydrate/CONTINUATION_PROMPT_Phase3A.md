# Continuation — CkSnapshot rebuild+hydrate campaign, resume at Phase 3A

**One-line:** Phases 0, 1, 2 are DONE + committed + green. Resume the UNATTENDED campaign at **Phase 3A** (save side) and run 3A → 3B → 4A → 4B → 5 to completion.

You are **Opus**; stay Opus for implementation. Work in the CkFoundation submodule
(`D:\Repositories\CkRepos\BusterBlock\Plugins\CkFoundation`, branch `feature/save-load-improvements`); new tests go in
`Plugins/CkTests` (branch `dev`).

---

## 0. READ THESE FIRST (in order) — they are authoritative, this file is just the session-bridge

1. **`docs/campaigns/saveload-rebuild-hydrate/PROGRESS.md`** — the canonical tracker. Read the **Status board**, the
   **Unattended execution protocol** section (OVERRIDES the default STOP-on-divergence rule), the **Phase-0 baseline
   table** (incl. the `[§1.6] Net env-red` + `[§1.6] Branch integration` notes), the **Decisions** sections
   ([P0-D1..D3], [P1-D1..D5], [P1-R1], [P2-D1..D3], and the **Fable-agent rulings** [BI-1], [P2-D2]).
2. **`docs/campaigns/saveload-rebuild-hydrate/PROMPT.md`** — mission, THE TEST GATE protocol, fences, file inventory.
3. **`docs/specs/2026-07-10-CkSnapshot-rebuild-hydrate-design.md`** — design authority. Read §4 (Produce/Apply,
   provenance EngineOwned/ConstructSpawned/RuntimeSpawned, reconciliation) and §5 (fidelity oracle) before 3A.
4. **`docs/campaigns/saveload-rebuild-hydrate/PHASE_3A.md`** — the phase you're about to execute. Then PHASE_3B/4A/4B/5
   each at the start of their phase.
5. The **CTO review** `docs/reviews/2026-07-10-CkSnapshot-rebuild-hydrate-CTO-review.md` — note **N1** (RuntimeSpawned
   subordinates / StoreDriver HFSM-task spawns double-spawn until spawner SM state is hydration-covered — a Phase-3→4
   coupling PHASE_3A §3A.4 must name) and **N3**.

`*.md` is gitignored repo-wide (`.gitignore:49`) — campaign docs are **force-added** (`git add -f`). Source files are unaffected.

---

## 1. Repo state (verify at session start)

- CkFoundation `feature/save-load-improvements` HEAD = **`99721e016`** (Phase-2 docs). `git -C Plugins/CkFoundation log --oneline -8` should show:
  `99721e016` docs Phase 2 DONE · `af9fad239` retire NeedsSetup guards · `8c4fbce7a` fire-gating · `91b96a177` load-gate core · `dcbd0eff0` Phase-2 checkpoint · `4fb178824`/`df06b8394` §1.6 docs · `5d262f52a` §1.6 oracle.
- CkTests `dev` HEAD = **`4522c8e`** (LoadGate test). Below it: `b9e7f86` (§1.6 ProduceDiffBaseline), `14d65ac` (§0 oracle harness).
- **Tree should be CLEAN** (Phase 2 fully committed). `git status --short` empty in both repos except any docs you add.
- **NOTHING is pushed.** Do NOT push. Commit per phase (stage files BY NAME, never `git add <dir>` blind). CkTests commits stay BEHIND the CkFoundation commits they test.
- **⚠️ Shared worktree:** a sibling session rebased this branch on 2026-07-11 16:55 to fold in the **object-pooling-core**
  campaign (+3758 lines, incl. CkEcs Scheduler/EntityScript). Commit hashes were rewritten (old `49cfdb038` → `951112723`);
  old hashes survive only in reflog. The branch carries TWO campaigns now. Stage only files you changed; never touch a
  sibling's dirty paths.

---

## 2. The mandate (UNATTENDED — from PROGRESS §"Unattended execution protocol")

Run the campaign UNATTENDED through Phases 3A → 3B → 4A → 4B → 5, in order. **Per-phase loop:**
(a) read `PHASE_N.md` (+ spec + `PHASE_1_RESEARCH.md` where relevant); (b) implement on Opus mimicking neighboring
CkFoundation code + house style; (c) gate via **UnrealToolbox** from BB root; (d) when GREEN + delta-zero vs baselines,
**commit that phase** (never push) + update PROGRESS.md, then proceed automatically. Do NOT stop between green phases.

**QUESTIONS / DIVERGENCES → delegate, don't halt.** When a step is ambiguous or reality diverges from the plan and the
answer is NOT in the PHASE docs / spec / research: launch a **Fable-class agent** (`Agent` tool, `model: "fable"`,
read-only general-purpose, run_in_background:false), give it the exact question + file paths, **VERIFY its ruling against
the cited code yourself** (agents over-report), record it in PROGRESS §Decisions, then implement on Opus. Never improvise
architecture on Opus. (Precedents this session: [BI-1] kiosk-red diagnosis, [P2-D2] fire-gating dependent-set.)

**TRUE STOP** (→ PROGRESS §Blockers, end the run) only for: (a) a red gate you can't fix against plan/research even after
a Fable consult; (b) an irreversible/outward action (push, force-push, cross-repo merge, deleting another session's
work); (c) a genuine human-only product/risk decision a Fable agent flags as needing Adam.

---

## 3. THE TEST GATE — how to run + read it (this bit up-front saves an hour)

All build/test via **UnrealToolbox from the BB root** (never Build.bat / raw `-ExecCmds`; editor must be CLOSED — a
PreToolUse hook blocks Build.bat while the editor runs). Run in the background; wait for real completion.

```powershell
# from D:\Repositories\CkRepos\BusterBlock  (Set-Location there; the shell cwd is the CkFoundation Source dir by default)
.\CkAuto\UnrealToolbox.exe --build --target Editor --config Development --output CkAuto\logs\<name>.log
.\CkAuto\UnrealToolbox.exe --test --test-pattern "Ck.Snapshot" --discover-fresh --output CkAuto\logs\<name>.log
.\CkAuto\UnrealToolbox.exe --test --test-pattern "Net" --discover-fresh --output CkAuto\logs\<name>.log
```

**Gate-reading gotchas learned the hard way this session:**
- **Premature `Result: Succeeded`.** The toolbox emits `Result: Succeeded / Total execution time: 1.65 seconds` for
  sub-steps (ShaderCompileWorker "Target is up to date") BEFORE the real editor build. The REAL editor build is done when
  you see `Total time in Unreal Build Accelerator local executor: NNN seconds` (build) or `**** TEST COMPLETE. EXIT CODE:
  N ****` + a `Total: / Passed: / Failed:` block (test). Wait for the background task's completion notification, then grep.
- **`--discover-fresh` is mandatory after adding/removing any test** — the toolbox caches its test list; a scoped
  `--discover-fresh` (e.g. for "Ck.Snapshot") leaves the cache PARTIAL, so a later "Net" run without `--discover-fresh`
  returns a nonsense subset. Always `--discover-fresh` per pattern when tests changed.
- **Compile-check BEFORE gating.** A `--build` only pass (no `--test`) is worth 10 min — this session it caught a dropped
  `namespace ck {` that would otherwise have burned a 20-min gate. Do it after big edits.
- **Read verdicts from the `--output` log, never the "completed" notification.** Grep for `Result={Fail`, `error C####`,
  `LNK####`, `Angelscript: Error` naming YOUR files. Pre-existing `FProInstanceInstance` AS errors + texture/Steam
  warnings at boot are engine noise, not yours.

**Baseline deltas (diff against these NAMES, from PROGRESS baseline table):**
- `Ck.Snapshot`: **49/49/0** currently (46 §0 baseline + Oracle.StructuralBaseline + Oracle.ProduceDiffBaseline +
  LoadGate.GatedSkipsKernelTicks). Any new red = investigate.
- `Net`: 102 tests. **Framework `Ck.*.Net` must be delta-zero.** The FOUR expected non-framework reds (all recorded, all
  ignorable): `Bb_AutoTest_RentnetKiosk_DamageToDestroy`, `..._DispensesLootOnDeath`, `RentnetKioskDriver_SpawnAndRelease`
  (BB kiosk-destruction env-flakes — they compose `DoesNotReplicate`, severed from the campaign) + `Ck.StateMachine.Net.
  OwningClientAuth_SubSm_AuthorityGatedTask` (known flake). A NEW `Ck.*.Net` red (non-kiosk, non-StateMachine) = real
  stop-condition. Filter: `grep Result=\{Fail ... | grep -viE "Rentnet|StateMachine.Net.OwningClientAuth"` → empty = good.
- The 3 two-signal pins that MUST stay green: `Ck.Attribute.Net.Values_AppliedBefore_OnReplicationComplete`,
  `Float_InitialBakedValue_Replicates`, `Float_PreComposition_StashedValue_Applies`.

---

## 4. What's already DONE (do not re-do)

- **Phase 0** — baselines, census (127 CK_REGISTER_SNAPSHOTABLE / 20 modules / 23 RepData), 3 dt==0 settle-guards,
  `CK_WITH_FIDELITY_ORACLE` define, oracle **Tier-1** (structural), harness test. [P0-D1..D3].
- **Phase 1** — `Produce`/`SeedContainer`/`Transport` + `RegisterLazyTyped<T>` on `FCk_ReplicatedFragmentHandlerRegistry`;
  ONE generic re-drive processor `FProcessor_Persistence_ReDriveOnRestore` replacing the per-feature `*_ReplicateOnRestore`
  family; **6 clean features migrated** (Velocity, Acceleration, Attribute×5, TagSet, MontagePlayer, AnimPlan) + their
  ReplicateOnRestore deleted; **6 DEFERRED** (Team, Player, Inventory Spatial+DataOnly, RenderTarget, 2dGridOccupancy —
  untouched, restore processors intact, per [P1-R1]); dormant local hydration queue (`FFragment_PendingHydration` +
  `FProcessor_Hydration_Dispatch`); oracle **Tier-2** (`Capture_Payloads`/`Diff_Payloads` + `Get_ProduceHandlerTypes`).
  Participation rule: `SeedContainer` present ⇒ re-drive; `Produce`-only ⇒ capture/oracle-only.
- **Phase 2** — `ECk_ProcessorLoadPolicy` trait + `ECk_SchedulerTickScope{Full,LoadKernel}` scope; 12-processor kernel
  marked RunsDuringLoad; `FGroup_Hydration` between PostTransform & Replication (both dispatchers moved in);
  `FTag_EntityScript_ConstructedThisFrame` defer ([P2-D1]); fire-gating `Get_HasUndrainedReplicatedFragments_IncludingDependents`
  recursing the **lifetime-dependents tree** ([P2-D2] Fable); retired the Velocity/Acceleration NeedsSetup apply-guards.

---

## 5. Phase 3A (your first task) — save side (read PHASE_3A.md for the real spec)

Save side per spec §4.2: provenance stamping (`EngineOwned`/`ConstructSpawned`/`RuntimeSpawned`) hung off the EntityScript
FinishConstruction path, spawn-recipe retention, and the **format v3 writer** (new files beside `CkSnapshot_Capture.cpp`;
`FormatVersion` bump in `SaveGame/CkSnapshot_Header.h`). Modules: CkEcs, CkSnapshot. **Thread CTO note N1** (name the
Phase-3→4 coupling: RuntimeSpawned subordinates double-spawn until the spawner SM state is hydration-covered at Phase 4)
into §3A.4. Phase 3A is additive (a NEW writer beside Model A, which stays for the oracle until Phase 5) — should be
lower-blast than Phase 2. Gate: `Ck.Snapshot` --discover-fresh + `Net`.

**Phase-2 line-refs in the PHASE docs are SHIFTED** by the object-pooling integration — re-locate insertion points by
PATTERN (grep for the symbol), never trust a bare line number in the older docs. Verified-current anchors are in PROGRESS.

---

## 6. Critical files / where things live

- **Tracker:** `docs/campaigns/saveload-rebuild-hydrate/PROGRESS.md` (canonical — trust it over memory).
- **Registry contract:** `Source/CkEcs/Public/CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h`
  (`FHandler{Apply,Remove,Produce,SeedContainer,Transport}`, `Get_ProduceHandlerTypes`/`Get_ReDriveHandlerTypes`).
- **Oracle:** `Source/CkEcs/Public/CkEcs/Snapshot/CkSnapshot_FidelityOracle.{h,cpp}` (Tier-1 + Tier-2, `#if CK_WITH_FIDELITY_ORACLE`).
- **Snapshot core (Model A + v3 writer target):** `Source/CkSnapshot/Public/CkSnapshot/Snapshot/CkSnapshot_Capture.cpp`
  / `CkSnapshot_Restore.cpp`; `SaveGame/CkSnapshot_Header.h` (FormatVersion); `Subsystem/CkSnapshot_Subsystem.cpp` (load SM).
- **Scheduler/load-gate (Phase 2):** `Source/CkEcs/Public/CkEcs/Scheduler/CkProcessor{Descriptor,Scheduler,Groups,Graph}.{h,cpp}`,
  `Subsystem/CkEcsWorld_Subsystem.{h,cpp}` (`Get_/Set_IsLoadGateActive`).
- **EntityScript construction (provenance hangs here):** `Source/CkEcs/Public/CkEcs/EntityScript/CkEntityScript_Processor.{h,cpp}`
  (FinishConstruction at `.cpp:~349`).

---

## 7. Ruled out / don't re-investigate

- The kiosk `Net` reds and the `StateMachine.Net` red are **NOT regressions** — recorded env-reds/flake ([BI-1], verified).
- The fire-gating dependent traversal question is **settled** ([P2-D2]) — recurse `Get_LifetimeDependents`, don't build a driver list.
- Test 2 (`Velocity_ApplyAfterLateSetup`) is **intentionally not written** ([P2-D3]) — covered by AccelerationParity_MPReload + pins.
- The branch hash-rewrite is **understood** (object-pooling integration) — not a corruption; don't "fix" it.

---

## 8. Recommended first moves

1. `git -C Plugins/CkFoundation log --oneline -8` and `git -C Plugins/CkTests log --oneline -3` — confirm HEADs `99721e016` / `4522c8e`, clean tree.
2. Read PROGRESS.md (status board + decisions + baseline notes), then PROMPT.md, then spec §4.2/§5, then PHASE_3A.md.
3. Confirm no editor is running (`tasklist | grep -i BusterBlockEditor`), then implement Phase 3A on Opus.
4. Compile-check (`--build` only) after the bulk of edits; then gate (`Ck.Snapshot` --discover-fresh + `Net`); read verdicts from the `--output` logs.
5. When green + delta-zero: commit (files by name; CkTests behind CkFoundation; no push), update PROGRESS.md, proceed to 3B.

---

## 9. Suggested first message to paste

> Continue the CkSnapshot rebuild+hydrate campaign, UNATTENDED, from Phase 3A. Phases 0–2 are done+committed+green.
> Read `Plugins/CkFoundation/docs/campaigns/saveload-rebuild-hydrate/PROGRESS.md` (canonical tracker), then PROMPT.md,
> the spec, and PHASE_3A.md. You're Opus; stay Opus for implementation; route design forks through a Fable agent and
> verify its ruling. Gate via UnrealToolbox from BB root (editor closed), read verdicts from the --output logs, commit
> per phase by name (no push), and run 3A → 3B → 4A → 4B → 5 to completion. Watch the gate gotchas in the continuation
> prompt (premature "Result: Succeeded", --discover-fresh, the recorded kiosk/StateMachine env-reds).
