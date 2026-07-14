# CkSnapshot Rebuild+Hydrate — mission brief (PROMPT.md)

**You are the executor.** This package was planned by a Fable-class session with full research; your job is to
execute, not design. When reality diverges from a step: STOP, record the divergence in `PROGRESS.md` → Blockers,
end the session. Do not improvise architecture. Every design decision is already made here or in the spec.

## Mission

Replace CkSnapshot's registry-image save/load with **rebuild + hydrate**: save = per-entity recipe + minimal
hydration payloads; load = rebuild entities through the NORMAL spawn path with simulation processors gated, then
overlay saved values through the SAME Apply-handler pipeline replication uses. End state: a feature author writes
`Add` / `Setup` / (optionally) a persistence payload — and never thinks about save/load.

**The spec is the design authority — read it in full before Phase 0:**
[docs/specs/2026-07-10-CkSnapshot-rebuild-hydrate-design.md](../../specs/2026-07-10-CkSnapshot-rebuild-hydrate-design.md)
(GREEN-LIT: [CTO review + v2 addendum](../../reviews/2026-07-10-CkSnapshot-rebuild-hydrate-CTO-review.md) — read the
addendum's three non-blocking notes N1/N2/N3; they are threaded into Phases 0/2/3B/4A below.)

## Repo / branch state (entry reality — verify at every session start)

- Superproject: `D:\Repositories\CkRepos\BusterBlock` (BB). This campaign touches TWO repos:
  - `Plugins/CkFoundation` (submodule) — branch **`feature/save-load-improvements`**, tip at campaign start
    `bc484d645` (spec `ec9587e58`, CTO addendum `bc484d645`, four M2 repair commits `15434d8ef` `5eda3ac8a`
    `fa2b5ac9d` `860ab0f2a`). All implementation commits go here unless the file lives in CkTests.
  - `Plugins/CkTests` (submodule) — new/changed tests go here. **CkTests commits must never be merged/pushed ahead
    of the CkFoundation commits they test** (cross-repo discipline; a CkTests tip referencing missing framework
    symbols breaks everyone).
- `.gitignore:49` blanket-ignores `*.md` — campaign docs and specs are **force-added** (`git add -f`). Source files
  are unaffected.
- Do NOT push anything. Commit per phase on `feature/save-load-improvements` (CkFoundation) and the current CkTests
  branch. No `Co-Authored-By` lines (house rule).
- A sibling session may be working in this worktree family. Stage ONLY files you changed, by name — never
  `git add <dir>`.

## Skills to load, and when

| When | Skill |
|---|---|
| Session start, every session | `ck-change-control` (gating discipline), `ck-tests-authoring-and-running` (CkTests) |
| Before writing any fragment/tag/processor/trait | `ck-macros-and-codegen` |
| Before Phase 2 (scheduler work) | `ckecs-domain-reference` + `ckecs-architecture-contract` |
| Any build/UHT/linker/AS failure | `ck-debugging-playbook` |
| Before touching `.as` or verifying AS surface (Phase 4B) | `ck-angelscript-interop` + `Script/CLAUDE.md` |

House style (root `CLAUDE.md`) is mandatory: trailing returns (except UFUNCTION decls), `CK_ENSURE_IF_NOT` (never
stock ensure), `NOT` macro, `_Member`/`In*` naming, no anonymous namespaces (filename-derived named namespaces),
`MoveTemp`, fmt-style logging. Mimic `CkTimer` for any new quartet-shaped code.

## THE TEST GATE (applies to EVERY phase — this is the no-regression contract)

All builds and test runs go through **UnrealToolbox** from the BB root (never Build.bat / raw `-ExecCmds`):

```powershell
# Build (editor must be CLOSED — a PreToolUse hook blocks builds while UnrealEditor runs):
D:\Repositories\CkRepos\BusterBlock\CkAuto\UnrealToolbox.exe --build --target Editor --config Development --output CkAuto\logs\ck-build.log

# Full snapshot suite (composable: add --build to do both in one call):
D:\Repositories\CkRepos\BusterBlock\CkAuto\UnrealToolbox.exe --test --test-pattern "Ck.Snapshot" --output CkAuto\logs\ck-test-snapshot.log

# Net suites (Ck.*.Net tests ONLY run via an explicit pattern — they are excluded from default runs):
D:\Repositories\CkRepos\BusterBlock\CkAuto\UnrealToolbox.exe --test --test-pattern "Ck.Attribute.Net" --output CkAuto\logs\ck-test-attr-net.log
D:\Repositories\CkRepos\BusterBlock\CkAuto\UnrealToolbox.exe --test --test-pattern "Net" --output CkAuto\logs\ck-test-net.log
```

Rules (violating any of these invalidates a "gate green" claim):

1. **`--discover-fresh` after adding/removing any test** without an intervening `--build` — the toolbox caches its
   test list and silently skips unknown tests. A "green" run that never discovered your new test is not green.
2. **Baseline discipline:** Phase 0 records the exact pass/fail counts + failing test NAMES for each pattern into
   `PROGRESS.md`. Every later phase re-runs the same patterns and reports the DELTA against those names ("baseline
   2 failing {a,b} → still 2 {a,b}"). There are ~9 known pre-existing failures in the broader BB suite — they are
   only ignorable if they are IN the recorded baseline by name.
3. **Read the verdict from the log file**, not the process chatter: grep the `--output` log for the result summary
   and for `Error:` lines naming your files. A toolbox "completed" notification is a proxy, not ground truth.
4. Green gate = build clean + `Ck.Snapshot` pattern delta-zero (plus phase-specific patterns listed in each
   PHASE doc) + no NEW `Angelscript: Error` in the run log.
5. If you edited source after the last build, the previous test run is STALE — rebuild and re-run before claiming
   the gate. Never claim a gate from a run that predates your last edit.

Existing test inventory this campaign protects (all in `Plugins/CkTests/Source/CkTests/Private/CkSnapshot/`,
38 files): `Ck.Snapshot.Core.RoundTrip`, `Ck.Snapshot.Parity.*_MPReload` (11 features), `Ck.Snapshot.M2a/M2b*`
orchestration gates, `Ck.Snapshot.DynamicFragment.*` (10), `Ck.Snapshot.Audit.UnregisteredRoundTripFlagged`,
`Ck.Snapshot.LifecycleStrip`, `Ck.Snapshot.Meta.RepDataRestoreCoverage`, plus RoundTrip tests per feature. The
two-signal lifecycle pins live outside the snapshot pattern: `Ck.Attribute.Net.Values_AppliedBefore_OnReplicationComplete`,
`Float_InitialBakedValue_Replicates`, `Float_PreComposition_StashedValue_Applies`.

## Phase map (each = one executor session; do them in order)

| Phase | Doc | One-liner | CkFoundation modules touched |
|---|---|---|---|
| 0 | `PHASE_0.md` | Baseline + census + 3 dt==0 fixes + oracle Tier-1 + harness test | CkPhysics, CkSubstep, CkEcs (new oracle files) |
| 1 | `PHASE_1.md` | `Produce`/`SeedContainer` on the handler registry; ONE re-drive processor replaces 12; local hydration queue; oracle Tier-2 | CkEcs Net + the 12 feature modules (deletions) |
| 2 | `PHASE_2.md` | `ECk_ProcessorLoadPolicy` + scheduler gate; `FGroup_Hydration` late dispatch; ReplicationComplete fire-gating; retire `5eda3ac8a` guards | CkEcs Scheduler/Net, CkPhysics |
| 3A | `PHASE_3A.md` | Save side: provenance stamping + spawn-recipe retention + format v3 writer | CkEcs, CkSnapshot |
| 3B | `PHASE_3B.md` | Load side: rebuild orchestration + reconciliation + retire reconstitution suppression; e2e gates | CkSnapshot, CkEcs, CkEcsExt, CkCamera (revert adopt hacks: NO — Phase 5) |
| 4A | `PHASE_4A.md` | SM redrive-as-hydration + N1 closure (spawner control-state) | CkStateMachine |
| 4B | `PHASE_4B.md` | Params-mutator payloads (7 features), RenderTarget re-author, MontagePlayer rebind, AS smoke matrix | per-feature |
| 5 | `PHASE_5.md` + `VALIDATION.md` | Decommission Model A (gate-not-delete registrations); full acceptance | CkEcs, CkSnapshot, CkCamera, CkEcsExt, CkPhysics |

Phases are independently shippable. If the campaign stops after any phase, the repo must be green.

## Chosen approach (settled — do not revisit)

Spec §4: Produce/Apply persistence contract on `FCk_ReplicatedFragmentHandlerRegistry`; provenance taxonomy
EngineOwned / ConstructSpawned / RuntimeSpawned with subtractive reconciliation; `ECk_ProcessorLoadPolicy` trait
(default `GatedDuringLoad`) with a framework-only kernel; hydration dispatch in a new `FGroup_Hydration` group
(after `FGroup_PostTransform`, before `FGroup_Replication`); ReplicationComplete fire gated on pending-apply drain
aggregated over dependents; three-tier fidelity oracle with `CK_WITH_FIDELITY_ORACLE`-gated deep-diff.

## Rejected approaches (kill reasons — never resurrect)

- **Harden Model A only** — repair obligation documented-unbounded (`CkSnapshot_RestoreMarker.h:17-19`); its best
  hardening IS Phase 1 of this migration.
- **dt==0 world-tick as the load gate** — dt-independent semantics (SM transitions, sensing) still execute; refuted.
- **Generic "setup settled" predicate from `MarkedDirtyBy` harvesting** — refuted by counterexample (Goap has no
  marker; CrowdAgent inverted marker; Probe's marker inside an unregistered aggregate; chained setups). NEVER build one.
- **Hydration-via-deferred-requests for ordering** — refuted: `Request_OverrideVelocity` is an IMMEDIATE write
  (`CkVelocity_Utils.cpp:94`); ordering must be global (group placement), not per-call.
- **Per-child persistent IDs for unlabeled children** — CTO fork ruling 1: rejected (parallel identity system).
  Unlabeled Construct-children are save-transient, both directions.
- **Next-tick dispatch deferral WITHOUT fire-gating** — breaks `Values_AppliedBefore_OnReplicationComplete` on the
  common path (CTO blocker 2). Fire-gating is mandatory, deferral rides on top of it.
- **Group-level (instead of per-processor) load gating** — impossible: Timer's Setup/HandleRequests/Update share
  one group.

## Fences (traps found during research — each is a hard "do NOT")

1. **Do NOT call `T::StaticStruct()` in static-init registrars** — trips the FoundPackage assert. Use the existing
   lazy-resolver pattern (`RegisterLazy`, `CkReplicatedFragmentContainer.cpp:27-52`).
2. **Do NOT store capturing lambdas / `std::function` in static registries** that grow during static-init — the
   TArray relocates and corrupts them (`CkSnapshot_FragmentRegistry.h:32-46`). `TFunction` inside the handler
   registry is fine (registrations complete before use); new REGISTRY-ARRAY entries must follow the existing shape.
3. **Do NOT use anonymous namespaces or file-local `static` helpers** — unity builds collide them.
4. **Do NOT write `.as` files while a toolbox test run is in flight** (corrupts the run).
5. **Do NOT trust `view.empty()` on in_place storages** — tombstones; iterate to check (precedent:
   `CkSnapshot_Subsystem.cpp:557-563`).
6. **Do NOT add UFUNCTION overloads** — UHT rejects; use house suffixes.
7. **Do NOT let a hydration/oracle code path silently continue on failure** — `CK_ENSURE_IF_NOT` with a correct
   silent recovery block, or loud. Non-negotiable #3.
8. **Do NOT run `Set_*`-named UFUNCTIONs into AS-visible surfaces without checking the asset-block gotcha**
   (`reference_as_compile_gotchas`): a `Set_*` UFUNCTION breaks `asset` blocks repo-wide in AS.
9. **`AuthorityOnly` processor gates must test `Get_IsEntityNetMode_Host`**, never `Get_HasAuthority` (transient
   entity is locally owned everywhere — `CkProcessor_NetModePolicy.cpp:25-31`).
10. **`FTag_EntityJustCreated` is STICKY** (entity-lifetime marker) — it does NOT mean "created this frame". The
    ConstructedThisFrame mechanism in Phase 2 exists because of this; do not substitute.

## Glossary (terms this package uses that the repo doesn't define yet)

- **Hydration payload** — the `FInstancedStruct` a feature's `Produce` emits and `Apply` consumes; for replicated
  features this is its existing `FCk_RepData_*`.
- **Kernel** — the fixed set of framework processors marked `RunsDuringLoad` (exact list in PHASE_2).
- **Provenance** — per-saved-entity classification: `EngineOwned` / `ConstructSpawned` / `RuntimeSpawned` (spec §4.2).
- **Reconciliation** — the post-rebuild subtractive pass (spec §4.2): rebuilt-labeled-but-not-saved → destroy;
  saved-no-match → loud orphan report.
- **Fidelity oracle** — test-only registry differ (spec §5): Tier-1 structural, Tier-2 Produce-diff, Tier-3 gated
  deep-diff.
- **Fire-gating** — `OnReplicationComplete` broadcast additionally requires pending-apply queues drained across
  self + dependents (spec §4.4, CTO note N2).
- **Model A** — the current registry-image capture/restore, kept green until Phase 5.

## File inventory (why each matters — read before touching)

**CkEcs / Net (the pipeline being generalized):**
- `Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h/.cpp` — handler registry (`FHandler{Apply,Remove}`
  at `.h:43-56`); Phase 1 extends it.
- `Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer_Processor.h/.cpp` — the single dispatch site;
  Phase 2 moves its group; Phase 1 adds the local queue drain; NotReady timeout 5s/2s at `_Processor.cpp:16-20`.
- `Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h/.cpp` — recipe replication precedent
  (`OnRep_ReplicationData_EntityScript` `.cpp:218-307`); fire-tag timing `.cpp:290-295`; dependent counters
  `.cpp:377-408` (Phase 2 fire-gating); `SetFragmentData<T>` `.h:271-294`.
- `Net/CkNet_Utils.h` — `TryAddContainerFragment` `:447-480` (host-gated, `NotAdded` on missing driver).

**CkEcs / Scheduler (the gate):**
- `Scheduler/CkProcessorDescriptor.h` — `PumpPolicy` enum `:79-84` (the trait pattern to mirror); descriptor fields.
- `Scheduler/CkProcessorTraits.inl.h` — `BuildDescriptor` `:125-222`; new trait `if constexpr` slots at `~:216`.
- `Scheduler/CkProcessorGroups.h` — group DAG (chain in header comment `:11-28`); Phase 2 inserts `FGroup_Hydration`.
- `Scheduler/CkProcessorScheduler.cpp` — main pass `:104-136`, pump `:204-300`, precomputed orders ctor `:69-80`
  (Phase 2 adds `_LoadPassOrder`); `Get_IsTickInProgress` re-entrancy `:181-187`.
- `Subsystem/CkEcsWorld_Subsystem.h/.cpp` — `ECk_ReconstitutionPhase` (retired Phase 3B); `Request_PumpToQuiescence`
  `.cpp:235-262`; `ACk_EcsWorld_Actor_UE::Tick` `.cpp:35-50` (gate flag threads through here).

**CkEcs / EntityScript + Snapshot:**
- `EntityScript/CkEntityScript_Processor.h/.cpp` — the 7 kernel construction processors; FinishConstruction stamps
  construction-complete state (`.cpp:182` area) — Phase 2 ConstructedThisFrame + Phase 3A provenance hang here.
- `EntityScript/CkEntityScript_Utils.cpp` — spawn guard `DoIs_WorldReconstituting` `:42-71` (retired Phase 3B);
  spawn request enqueue `:302`.
- `Snapshot/CkSnapshot_FragmentRegistry.h` — `CK_REGISTER_SNAPSHOTABLE` + registry (Phase 5 gates it);
  `CK_WITH_FIDELITY_ORACLE` define lands here (Phase 0).
- `Snapshot/CkSnapshot_RestoreMarker.h` — `FTag_Snapshot_JustRestored` (Phase 1 re-drive keys on it; deleted Phase 5).

**CkSnapshot:**
- `Subsystem/CkSnapshot_Subsystem.h/.cpp` — load state machine (`ELoadPhase`, `DoTick_Load` `:568+`); Phase 3A/3B
  rework; frame caps `kLoad_*FrameCap=600`.
- `Snapshot/CkSnapshot_Capture.cpp` / `CkSnapshot_Restore.cpp` — Model A core (stays for oracle); v3 writer/loader
  are NEW files beside them.
- `SaveGame/CkSnapshot_Header.h` — FormatVersion (v3 bump, Phase 3A).

**Feature modules (phase-specific):** listed in each PHASE doc. The 12 `*_ReplicateOnRestore` deletion list is in
PHASE_1; the dt==0 fixes in PHASE_0; the 7 Params-mutator payloads in PHASE_4B.

**CkTests:** `Source/CkTests/Private/CkSnapshot/` — all 38 existing files; new tests land beside them, named
`Test_Snapshot_<Area>_<Scenario>.cpp` registering `"Ck.Snapshot.<Area>.<Scenario>"`, following the closest existing
file's shape (spec pattern reference: `Test_Snapshot_Core_RoundTrip.cpp` for registry-level, `*_Gate.spec.cpp` for
orchestration/MP).

## PROGRESS.md discipline

Update `PROGRESS.md` at session end, every session: phase status, gate numbers (with failing names), commits made
(hash + repo), blockers hit. The next session's entry criteria are checked against it. Blockers go in the Blockers
section — never improvised around.
