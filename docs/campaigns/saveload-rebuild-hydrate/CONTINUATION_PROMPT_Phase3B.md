# Continuation — CkSnapshot rebuild+hydrate campaign, resume at Phase 3B

**One-line:** Phases 0, 1, 2, **3A** are DONE + committed + green. Resume the UNATTENDED campaign at **Phase 3B** (load
side — the heart of the migration) and run 3B → 4A → 4B → 5 to completion.

You are **Opus**; stay Opus for implementation. Work in the CkFoundation submodule
(`D:\Repositories\CkRepos\BusterBlock\Plugins\CkFoundation`, branch `feature/save-load-improvements`); new tests go in
`Plugins/CkTests` (branch `dev`).

---

## 0. READ THESE FIRST (in order) — authoritative; this file is the session-bridge

1. **`docs/campaigns/saveload-rebuild-hydrate/PROGRESS.md`** — canonical tracker. Read the **Status board**, the
   **Unattended execution protocol** (OVERRIDES STOP-on-divergence: delegate design forks to a Fable agent, verify,
   record, implement on Opus — do NOT halt between green phases), the **Phase-0 baseline table** (+ the `[§1.6] Net
   env-red trio` + `[§1.6] Branch integration` notes), and the **Decisions** ([P0-*], [P1-*], [P2-*], **[P3A-D1/D2]**,
   Fable **[P3A-F1]** GC-holder / **[P3A-F2]** RemapHandles-lift, **[BI-1]** kiosk-red, **[P2-D2]** fire-gating).
2. **`docs/campaigns/saveload-rebuild-hydrate/PROMPT.md`** — mission, THE TEST GATE protocol, fences, file inventory.
3. **`docs/specs/2026-07-10-CkSnapshot-rebuild-hydrate-design.md`** — design authority. Re-read **§4.2** (provenance
   taxonomy + reconciliation), **§4.3** (load gate + kernel + settle), **§4.4** (hydration ordering + fire-gating),
   **§4.5** (what load keeps/deletes) before 3B.
4. **`docs/campaigns/saveload-rebuild-hydrate/PHASE_3B.md`** — the phase to execute (steps 3B.1–3B.3, exit, fences).
   Then PHASE_4A/4B/5 each at the start of their phase.
5. The **CTO review** `docs/reviews/2026-07-10-CkSnapshot-rebuild-hydrate-CTO-review.md` — **N1** (RuntimeSpawned
   subordinates / StoreDriver HFSM-task spawns double-spawn until spawner SM state is hydration-covered — that is
   Phase 4A's job; in 3B they go to the **oracle allowlist** with a `# Phase-4-pending:` comment, NOT "fixed" by
   suppressing the spawner — PHASE_3B fence). N2 fire-gating (done Phase 2). N3.

`*.md` is gitignored repo-wide (`.gitignore:49`) — campaign docs are **force-added** (`git add -f`). Source unaffected.

---

## 1. Repo state (verify at session start)

- CkFoundation `feature/save-load-improvements` HEAD = **`851ad572e`** (Phase-3A docs). `git -C Plugins/CkFoundation log --oneline -6`:
  `851ad572e` docs 3A DONE · `d0ce51877` registrar transport (3A.4) · `349947218` v3 writer · `81f7b6505` CkEcs framework ·
  `156333657` continuation-3A · `99721e016` Phase-2 DONE.
- CkTests `dev` HEAD = **`5bb9798`** (V3 tests). Below it: `4522c8e` (LoadGate).
- **Tree CLEAN** in both (3A fully committed). **NOTHING pushed.** Commit per phase (stage files BY NAME, never
  `git add <dir>`). CkTests commits stay BEHIND the CkFoundation commits they test.
- **⚠️ Shared worktree:** a sibling session may be active (D:/E: worktrees). Stage only files you changed; never touch a
  sibling's dirty paths. Phase 3A's file set was verified clean-of-foreign-paths; keep that discipline.

---

## 2. What Phase 3A built (the v3 SAVE format you will now LOAD)

Read the code, but the shape you consume in 3B:
- **Provenance stamps (CkEcs):** `ck::FTag_ConstructSpawned` (transient) stamped at create in
  `Request_SetupEntityWithLifetimeOwner` (`CkEntityLifetime_Utils.cpp`) when the owner is a not-yet-begun-play
  EntityScript. `ck::FFragment_SpawnRecipe` (+ GC-safe holder `UCk_EntityScript_SpawnRecipe_UE`) stamped at spawn in
  `FProcessor_EntityScript_SpawnEntity_HandleRequests::DoHandleRequest` — carries `Get_ScriptClass()` +
  `Get_SpawnParams()`. Files: `CkEcs/EntityScript/CkEntityScript_SpawnRecipe.h/.cpp`.
- **v3 format (`CkSnapshot/SaveGame/CkSnapshot_Header.h`):** `ECk_Snapshot_V3_Provenance{EngineOwned,ConstructSpawned,
  RuntimeSpawned}`; `FCk_Snapshot_V3_EntityEntry` (SavedId uint32, Provenance, LifetimeOwnerSavedId, SaveKey/PlayerId
  [EngineOwned], Label [ConstructSpawned], ScriptClassPath+SpawnParamsBytes+ContextOwnerSavedId+ActorClassPath
  [RuntimeSpawned]); `FCk_Snapshot_V3_PayloadEntry` (OwnerSavedId, TypePath, PayloadBytes); `FCk_Snapshot_V3_Tables`
  (`_Entities` + `_Payloads`, SerializeItem'd into the byte stream); `FCk_Snapshot_HeaderV3` (FormatVersion=3 + census
  counts). SaveGame now carries `_HeaderV3` + `_SnapshotBytesV3` beside Model-A `_Header`/`_SnapshotBytes`.
- **Writer (`CkSnapshot/Snapshot/CkSnapshot_CaptureV3.h/.cpp`):** `Run_CaptureV3(UWorld&, FArchive&, HeaderV3&)` +
  `Run_CaptureV3_Registry(registry, registryHandle, UWorld*, FArchive&, HeaderV3&)`. Entities written in
  lifetime-topology order (owners before dependents, depth-sorted). Blobs (params + payloads) serialized via a proxy
  archive (ArIsSaveGame=false, persistent) + `ck::snapshot::RemapHandles` (SAVE mode writes raw entity ids).
- **Shared handle-walk (`CkEcs/Snapshot/CkSnapshot_HandleWalk.h/.cpp`):** `ck::snapshot::RemapHandles(struct, memory,
  FArchive&, FSnapshotContext&)` (routes every FCk_Handle-derived field through `FSnapshotContext::Snapshot_Handle`) +
  `ForEachHandle(struct, memory, visitor)`. Lifted verbatim from CkDynamic (which now calls the shared copy).
- **Registry accessor:** `FCk_ReplicatedFragmentHandlerRegistry::Get_SaveHandlerTypes()` (Produce ∧ Transport&Save).
  10 migrated features flipped `Transport=NetAndSave`; 6 deferred (Team, Player, Inventory Spatial+DataOnly,
  RenderTarget, 2dGridOccupancy) gained per-entity Produce (no SeedContainer, capture-only per [P1-R1]).
- **`Request_Save` dual-writes** Model A + v3. Load still consumes Model A (v3 read is 3B's job).

Commits: CkF `81f7b6505`/`349947218`/`d0ce51877`, CkTests `5bb9798`. Gate GREEN: Ck.Snapshot **51/51/0**, Net framework
delta-zero.

---

## 3. THE TEST GATE — how to run + read (unchanged from 3A; internalize before touching the load SM)

All build/test via **UnrealToolbox from BB root** (`D:\Repositories\CkRepos\BusterBlock`), editor CLOSED (a PreToolUse
hook blocks Build.bat while the editor runs). Run in background; wait for real completion.
```powershell
.\CkAuto\UnrealToolbox.exe --build --target Editor --config Development --output CkAuto\logs\<name>.log     # compile-check
.\CkAuto\UnrealToolbox.exe --build --test --test-pattern "Ck.Snapshot" --discover-fresh --output CkAuto\logs\p3b.log
.\CkAuto\UnrealToolbox.exe --test --test-pattern "Net" --discover-fresh --output CkAuto\logs\p3b-net.log
```
**Gotchas (learned the hard way — all held true through 3A):**
- **Premature `Result: Succeeded`** — the toolbox prints it for a ShaderCompileWorker sub-step BEFORE the real editor
  build. REAL done = `Total time in Unreal Build Accelerator local executor: NNN seconds` (build) or `**** TEST
  COMPLETE. EXIT CODE: N ****` (test). The FINAL `Result: Failed/Succeeded` line at the tail is the build verdict.
- **`--discover-fresh` mandatory** after adding/removing a test (per pattern — a scoped Ck.Snapshot discover leaves the
  Net cache partial, so discover-fresh Net too).
- **Compile-check (`--build` only) after the bulk of edits** — in 3A it isolated a single link error (CoreOnline) from
  ~15 files of new code, worth the 15 min. Do it before the full gate on a big phase.
- **Read verdicts from the `--output` log**, not the "completed" notification. Grep `Result=\{Fail`, `error C####`,
  `LNK####`, `Angelscript: Error` naming YOUR files. `FProInstanceInstance` AS errors + texture/Steam boot warnings =
  engine noise. `warning ... LF will be replaced by CRLF` on commit = benign normalization (my LF writes, repo is CRLF).

**Baseline deltas (diff against NAMES):**
- `Ck.Snapshot`: **51/51/0** currently. 3B: the 11 `Parity.*_MPReload` + `M2a/M2b*` specs now run through the **v3
  load path** (rebuild+hydrate). EXPECTED CASUALTIES to REWRITE (not delete): `Ck.Snapshot.LifecycleStrip` +
  `Ck.Snapshot.M2b2a.ReplicatedRespawn` (assert Model-A load mechanics — rewrite to end-state parity). Registry-level
  (`Core.RoundTrip`, `DynamicFragment.*`, `Audit.*`, all the `*RoundTrip`, Oracle, LoadGate) stay green untouched
  (Model A still compiles). Any OTHER red → decision gate (PHASE_3B §"Decision gate"): expected-casualty-you-rewrote,
  or oracle-allowlist (N1 shape only), else STOP→Blockers.
- `Net`: framework `Ck.*.Net` **delta-zero** + the recorded kiosk env-red trio (`Bb_AutoTest_RentnetKiosk_DamageToDestroy`,
  `..._DispensesLootOnDeath`, `RentnetKioskDriver_SpawnAndRelease`) + `Ck.StateMachine.Net.OwningClientAuth_SubSm_Auth‌orityGatedTask`
  (flakes; may not fire). Filter: `grep Result=\{Fail | grep -viE "Rentnet|StateMachine.Net.OwningClientAuth"` → empty = good.
- 3 two-signal pins MUST stay green: `Ck.Attribute.Net.Values_AppliedBefore_OnReplicationComplete`,
  `Float_InitialBakedValue_Replicates`, `Float_PreComposition_StashedValue_Applies`.

---

## 4. Phase 3B scope (read PHASE_3B.md for the real spec — this is orientation)

**3B is HIGH-BLAST: it rewrites the load state machine that the Parity/M2 gate exercises end-to-end.** There is NO safe
intermediate commit — the whole load pipeline must work before the gate is green. Budget accordingly; consider a
`--build`-only compile-check after the SM rework before the full gate. If context runs tight mid-phase, CHECKPOINT in
PROGRESS with the exact state (like the Phase-2 2.1–2.3 checkpoint) — a half-done load rewrite left dirty is fine to
resume from, but do NOT commit a non-gate-green partial.

**Load SM today** (`CkSnapshot_Subsystem.{h,cpp}`): `ELoadPhase{Idle,TearingDown,Traveling,AwaitingWorld,Restoring,
RespawningActors}`. `Request_Load` (`.cpp:191`) format-gates on **Model-A** `_Header.Get_FormatVersion()==2`, latches,
sets `ReconstitutionFlag(Full)`, subscribes `OnPostWorldInitialization` (stamps `EarlyWindow` on the fresh world
pre-BeginPlay), tears down, starts an `FTSTicker` → `DoTick_Load`. `DoInitiate_Teardown`/`DoIs_TeardownComplete`/
`DoInitiate_Travel` (OpenLevel vs seamless ServerTravel by client count) **KEEP their shapes**. The tail
(`DoRun_Restore`→`DoStamp_RespawnMarkers`→`DoIs_RespawnComplete`, the respawn-quiescence countdown) is what 3B
REPLACES. Poll helpers iterate (never `view.empty()` on in_place storages — fence #5). Frame caps = 600.

**3B.1 — new pipeline** `Idle → TearingDown → AwaitingWorld → Rebuilding → Hydrating → Reconciling → Settling → Idle`:
- On world-ready: `Set_IsLoadGateActive(true)` on the new world's `UCk_EcsWorld_Subsystem_UE` (Phase-2 load gate)
  INSTEAD of any reconstitution phase.
- **NEW: a v3 READER** — deserialize `FCk_Snapshot_V3_Tables` from `_SnapshotBytesV3` (symmetric with the writer's
  `StaticStruct()->SerializeItem`; see the round-trip in `Test_Snapshot_V3_Capture.cpp` RecipeParamsHandleRemap for the
  blob-read idiom). Build the **saved-id → live-handle map** as entities are rebuilt (owners first, by file order).
- **NEW: a v3 `FSnapshotContext` mode** — the params/payload blobs were written with SAVE-mode `Snapshot_Handle` (raw
  saved ids). On load, remap each blob handle through `TMap<uint32/uint64, FCk_Handle>` (the saved-id map). Add this
  mode beside the continuous-loader mode in `CkEcs/Snapshot/CkSnapshot_Context.h` (a map-backed `Snapshot_Handle`
  branch). Then `ck::snapshot::RemapHandles(blobStruct, memory, readerArchive, v3Context)` rewrites blob handles to
  live handles.
- **Rebuilding:** RuntimeSpawned → `UCk_Utils_EntityScript_UE::Request_SpawnEntity` with recipe class + remapped params
  (owner via the saved-id map; missing owner → loud ensure + skip + LoadReport). `ActorSpawnIntent` entries ride the
  existing `FTag_ActorRespawn_Pending` + `FProcessor_ActorRespawn` (kernel). EngineOwned → rendezvous poll (SaveKey via
  the resolver rehydrated from LIVE world-side SaveKey fragments; player via PlayerState). ConstructSpawned → resolve
  AFTER owner constructed (owner handle → labeled child via record/label utils); not found → orphan-report. Exit when
  every entry mapped/reported AND all spawned scripts finished construction.
- **Hydrating:** write each mapped entity's payload list into `FFragment_PendingHydration` + `FTag_Hydration_PendingApply`
  (the DORMANT queue Phase 1 built + the `FProcessor_Hydration_Dispatch` Phase 2 wired into `FGroup_Hydration` — 3B
  ACTIVATES them). Payload handles remap through the v3 context. Exit when no entity carries the tag (dispatcher drains
  under the gate) or the 5s/2s dispatcher timeout dropped stragglers (LoadReport).
- **Reconciling:** framework-side subtractive pass (spec §4.2): per saved owner, enumerate live labeled ConstructSpawned
  children; any NOT in the saved child set → `Request_DestroyEntity` (destruction pipeline is GATED — these PARK and
  complete on the first post-gate frame; only QUEUE here). Never direct registry destruction (fence).
- **Settling:** `Request_PumpToQuiescence(LoadKernel)` from the FTSTicker (outside a scheduler tick — the re-entrancy
  skip can't bite) → `Set_IsLoadGateActive(false)` → `DoFinish_Load`. First normal frame drains accumulated
  NeedsSetup/requests.
- **`Request_Load` v3-only HARD BREAK** (fork ruling 5): read `_SnapshotBytesV3`/`_HeaderV3`; reject a v2-only save with
  `Failed_IncompatibleSave`. DELETE the Model-A load fallback read; KEEP Model-A capture/restore code compiled (oracle +
  registry tests use it until Phase 5).

**3B.2 — retire reconstitution suppression:** delete `ECk_ReconstitutionPhase` + accessors + `_ReconstitutionPhase`
(`CkEcsWorld_Subsystem.h/.cpp`), `DoIs_WorldReconstituting` + call sites (`CkEntityScript_Utils.cpp:42-71,147,196`),
`Get_IsSnapshotRespawnable` (`CkEntityScript.h/.cpp`), the `OnPostWorldInitialization` watch + `DoSet_ReconstitutionFlag`
+ `DoUnsubscribe_WorldInitWatch` + the respawn-quiescence countdown in `CkSnapshot_Subsystem`. Verify:
`rg --no-ignore -n "Reconstitution|IsSnapshotRespawnable" Source` → zero outside docs/history.

**3B.3 — tests + gate:** existing Parity/M2 specs are the main gate (now run through v3). Rewrite LifecycleStrip +
M2b2a.ReplicatedRespawn. NEW: `Ck.Snapshot.Rebuild.NoDuplicateGrants`, `.LostGrantStaysLost`, `.OrphanHydrationLoud`,
`.OracleParity` (create `docs/campaigns/saveload-rebuild-hydrate/oracle-allowlist-p3.txt` — N1 driver/SM-subordinate
duplicate lines go here, each `# Phase-4-pending:`-tagged), `Ck.Snapshot.V3.InstancedStructDiskSmoke`. Record load
wall-time → PROGRESS. See PHASE_3B §"Known interaction to expect" ([B1]-shaped deferred-feature Apply reds → annotate as
Phase-4B, don't invent an authority-side drain) + the Decision gate.

Commits: `feat(CkSnapshot): v3 rebuild+hydrate load pipeline`; `refactor(CkEcs,CkEcsExt): retire reconstitution
suppression`; (CkTests) one commit per test cluster.

---

## 5. Design forks likely in 3B → route through a Fable agent, VERIFY against code, record, implement on Opus

Per the unattended protocol. Anticipated forks (don't improvise these on Opus):
- The exact **saved-id map keying** + how the v3 `FSnapshotContext` mode integrates with `Snapshot_Handle` (the writer
  wrote raw ids; the reader must map them — confirm the id width and null/sentinel handling).
- **ConstructSpawned adoption timing** — the record/label lookup must run AFTER the owner's Construct re-created the
  child; sequence this against the Rebuilding poll (owners-first ordering helps, but confirm the labeled child exists
  at lookup time vs. needs its own poll).
- **Reconcile vs. gated destruction interaction** — the parked-destroy semantics (Phase 2.2) mean absence is only
  observable ≥1 frame post-gate; the `LostGrantStaysLost` test must tick past gate-open (PHASE_3B says ≥2 frames).
- Any **N1 duplicate** that surfaces in OracleParity → allowlist + annotate (NOT suppress). If a duplicate does NOT
  match the driver/SM-subordinate shape → STOP → Blockers.

Precedents this campaign: [P3A-F1]/[P3A-F2] (Fable, verified), [BI-1]/[P2-D2] (Fable, verified). A Fable agent =
`Agent` tool, `model: "fable"`, read-only, give it exact file:line anchors + the question, VERIFY its ruling against the
cited code yourself (agents over-report), record in PROGRESS §Decisions, then implement on Opus.

---

## 6. Critical files / where things live

- **Tracker:** `docs/campaigns/saveload-rebuild-hydrate/PROGRESS.md` (trust over memory).
- **Load SM (rework target):** `Source/CkSnapshot/Public/CkSnapshot/Subsystem/CkSnapshot_Subsystem.{h,cpp}`
  (`ELoadPhase` `.h:107`; `Request_Load` `.cpp:191`; `DoTick_Load`/`DoRun_Restore`/`DoIs_RespawnComplete`/
  `DoRehydrate_SaveKeyResolver`/`DoStamp_RespawnMarkers`/`DoFinish_Load` in `.cpp:~436-620`).
- **v3 format + reader target:** `Source/CkSnapshot/Public/CkSnapshot/SaveGame/CkSnapshot_Header.h` (the V3 structs),
  `Snapshot/CkSnapshot_CaptureV3.cpp` (the writer to mirror on read).
- **Handle remap:** `Source/CkEcs/Public/CkEcs/Snapshot/CkSnapshot_Context.h` (add v3 map-backed mode),
  `CkSnapshot_HandleWalk.{h,cpp}` (`RemapHandles`/`ForEachHandle`).
- **Load gate (Phase 2):** `Source/CkEcs/Public/CkEcs/Subsystem/CkEcsWorld_Subsystem.{h,cpp}`
  (`Get_/Set_IsLoadGateActive`, `Request_PumpToQuiescence(scope)`); scheduler `CkProcessor{Scheduler,Groups}.{h,cpp}`.
- **Hydration queue (dormant → activate):** `CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.inl.h`
  (`FFragment_PendingHydration`, `FTag_Hydration_PendingApply`) + `CkPersistence_ReDrive_Processor.*` /
  `CkReplicatedFragmentContainer_Processor.*` (`FProcessor_Hydration_Dispatch`, `ck::persistence_apply::ApplyOne`).
- **Reconstitution machinery to DELETE (3B.2):** grep `Reconstitution|IsSnapshotRespawnable` across Source.
- **Recipe/provenance (3A, consume on load):** `CkEcs/EntityScript/CkEntityScript_SpawnRecipe.h`;
  `CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h` (`FTag_ConstructSpawned`).

---

## 7. Ruled out / don't re-investigate

- Kiosk `Net` reds + `StateMachine.Net` flake = recorded env-reds/flake ([BI-1], verified) — not regressions.
- v3 recipe GC storage = holder-UObject-pinned-by-fragment ([P3A-F1], verified) — settled.
- Handle-in-blob remap = shared `ck::snapshot::RemapHandles` ([P3A-F2], verified) — reuse it (add the load-side map mode).
- ConstructSpawned discriminator = owner lacks `FTag_EntityScript_HasBegunPlay` at child create ([P3A-D1]) — settled.
- Deferred-six Produce keyed per-entity mirroring `*_Replicate` ([P3A-D2]) — settled; capture-only (no re-drive).
- Model A stays compiled (oracle + registry tests) until Phase 5 — do NOT delete it in 3B.

---

## 8. Recommended first moves

1. Verify HEADs `851ad572e` / `5bb9798`, clean trees, editor closed (`tasklist | grep -i BusterBlockEditor`).
2. Read PROGRESS (status + decisions), PROMPT, spec §4.2–§4.5, PHASE_3B.md.
3. Read the FULL load SM tail (`DoTick_Load`+helpers, `.cpp:436-620`) + the reconstitution machinery to retire + the
   dormant hydration queue + `CkSnapshot_Context.h`. Plan the phase in PROGRESS (§Phase-3B progress) before editing.
4. Route the saved-id-map / FSnapshotContext-v3-mode / ConstructSpawned-timing forks through a Fable agent; verify.
5. Implement 3B.1 (reader + map + SM rework) → 3B.2 (retire) → compile-check → 3B.3 (tests + allowlist) → gate → commit.
6. When green (modulo the annotated allowlist): commit per phase (no push), update PROGRESS + record the load-time
   baseline, write CONTINUATION_PROMPT_Phase4A.md, proceed to 4A.

---

## 9. Suggested first message to paste

> Continue the CkSnapshot rebuild+hydrate campaign, UNATTENDED, from Phase 3B. Phases 0–3A are done+committed+green.
> Read `Plugins/CkFoundation/docs/campaigns/saveload-rebuild-hydrate/CONTINUATION_PROMPT_Phase3B.md` FIRST, then
> PROGRESS.md (canonical), PROMPT.md, the spec §4.2–§4.5, and PHASE_3B.md. You're Opus; stay Opus for implementation;
> route design forks through a Fable agent and verify its ruling against code. Gate via UnrealToolbox from BB root
> (editor closed), read verdicts from the --output logs, commit per phase by name (no push). 3B is HIGH-BLAST (rewrites
> the e2e-tested load path, no safe intermediate commit) — compile-check before the full gate, and checkpoint in
> PROGRESS if context runs tight rather than commit a non-green partial. Watch the gate gotchas + recorded kiosk env-reds.
