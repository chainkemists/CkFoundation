# Continuation — CkSnapshot rebuild+hydrate: 4B.3 AS smoke + Phase 5 (partial)

**One-line:** Phase 4B hydration-parity casualties are ALL closed (TagSet/RenderTarget/Attributes×5/AnimPlan green;
Ck.Snapshot 52/49/3, the 3 reds = the [INV-A] trio deferred to Adam). Two technically-ready tracks remain:
**Track A — 4B.3 AS SaveGame smoke** (a new CkEcs framework handler + 2 CkTests gates) and **Track B — Phase 5 partial**
(gate Model A behind the fidelity-oracle flag; delete the dead Model-A restore paths). You are **Opus**; stay Opus for
implementation; route any design fork through a **Fable** agent (`Agent`, `model:"fable"`, read-only) and VERIFY every
ruling against cited code before implementing. **No push. Stage only files you changed BY NAME. No Co-Authored-By.**

The design below was scoped by a Fable agent and the two load-bearing linchpins were Opus-verified (see §Linchpins).
Treat every OTHER file:line as "verify at implementation start" (campaign protocol) — anchors drift.

## SESSION PROGRESS (2026-07-12, Opus unattended)
- ✅ **TRACK A (4B.3 AS smoke) DONE + COMMITTED.** CkF `09ca84f41` (framework `FCk_SaveData_EntityScriptFields` handler),
  CkTests `a0c2f2d` (AS fixture + 2 gates). Gate: Ck.Snapshot **54/51/3** (both AS tests green, the 3 = [INV-A] trio),
  Net **102/99/3** framework delta-zero. Implemented exactly per §TRACK A below; [P4B-D5] recorded (post-BeginPlay timing).
  Nothing pushed.
- ⏳ **TRACK B (Phase 5 partial) IN PROGRESS — clusters 1a + 2 DONE + COMMITTED, checkpointed before 5.2.2.**
  Cluster 1a `521e3834d` (oracle-gate Model-A registration + drop dual-write + Get_SaveSlotHeader v3-synthesis, [P5-D1]);
  cluster 2 `8bdd08346` (delete dormant FProcessor_ActorRespawn). Both GREEN (Ck.Snapshot 54/51/3, delta-zero). **NEXT SESSION:
  execute cluster 3 (5.2.2 — the big 9-processor JustRestored/re-drive delete) per PROGRESS §Phase-5 progress cluster 3 (full
  surface + safe order + the CkAttribute stale-include snag + the FloatAttribute.Gate check are all recorded there), then
  clusters 4 (Transform _Previous revert), 5 (machinery gate + Shipping compile), 6 (docs + amended 5.3 grep + final
  Ck.Snapshot+Net gate).** Re-run the Net gate after cluster 3 (it deletes replicated-feature restore processors). Nothing pushed.

## 0. STATE (verify at start)
- CkFoundation `feature/save-load-improvements` HEAD = `075caaa6f` (docs) on top of the 4B feature commits:
  `19613a1eb` (invariant fix) · `5b7c1e077` (Attributes×5) · `db83e687a` (AnimPlan) · `ede66976f` (RenderTarget) ·
  `5088f5336` (TagSet) · `705e7d57e` (StateMachine).
- CkTests HEAD = `773a4d2` (UNCHANGED since 4A.1 — Track A adds the FIRST CkTests commit of this arc).
- Tree CLEAN. NOTHING pushed. Editor CLOSED.
- Gate baselines: Ck.Snapshot **52/49/3** (the 3 = `Parity.{GridPlacements,InventoryDataOnly,InventorySpatial}_MPReload`,
  all [INV-A]). Net **102/98/4** (kiosk env-trio `Bb_AutoTest_RentnetKiosk*` + the documented
  `Ck.StateMachine.Net.OwningClientAuth_SubSm` flake — both ignorable). A NEW framework `Ck.*.Net` red = investigate.
- PROGRESS.md is CANONICAL — read §Decisions [P4B-D2/D3/F2/D4/N1], §Blockers [INV-A]/[N1-A]. Trust it over memory.

## 1. READ FIRST
1. PROGRESS.md (esp. the 4B row + [P4B-N1] additive-work assessment + [INV-A]/[N1-A] blockers).
2. PHASE_4B.md §4B.3 (the AS-smoke spec; note the pre-BeginPlay wording deviation in §Linchpins below).
3. PHASE_5.md + VALIDATION.md (Track B; note "Delete list ≠ delete files" — most of Model A is GATED, not deleted).
4. Skills: `ck-angelscript-interop` + Script/CLAUDE.md (before ANY .as work); `ck-tests-authoring-and-running`
   (test placement); `ck-change-control` (Phase-5 touches framework invariants).

## Linchpins (Opus-VERIFIED — the foundation is sound)
- The EntityScript instance lives in `FFragment_EntityScript_Current._Script` (`TWeakObjectPtr<UCk_EntityScript_UE>`,
  public `Get_Script()`) — `CkEcs/.../EntityScript/CkEntityScript_Fragment.h:78,85`. v3 re-Constructs it as a FRESH
  UObject on load (SaveGame UPROPERTYs reset to defaults) → restoring them is mandatory.
- `FProcessor_EntityScript_BeginPlay` is load-kernel (`LoadPolicy = RunsDuringLoad`, `CkEntityScript_Processor.h:168`),
  so it runs DURING the load-gate (Rebuilding phase) BEFORE hydration payloads enqueue → SaveGame fields restore
  **post-Construct, post-BeginPlay** (like every other v3-hydrated feature), NOT "pre-BeginPlay" as PHASE_4B.md:37-38
  wording implies. **Decision to record + designer-contract line:** "don't read SaveGame fields in an EntityScript's
  BeginPlay — they land at hydration." Executor judgment, campaign-precedented ([SM-A] class); do NOT build a
  BeginPlay-defer (scheduler-core change, out of scope).
- AS `UPROPERTY(SaveGame)` sets a REAL `CPF_SaveGame` flag (engine fork `AngelscriptClassGenerator.cpp:2845-2847`);
  `meta=(SaveGame)` is INERT (the historic SaveKey bug — do NOT copy BB's `BB_CosmeticOwnership_Feature.as` meta form).

## TRACK A — 4B.3 AS SaveGame smoke (do FIRST: self-contained + gate-verifiable)
Goal: `Ck.Snapshot.AS.SaveGameFields_RoundTrip` + `Ck.Snapshot.AS.NonSaveGameField_Drops` green. Design (Fable, verify anchors):

**A.1 New framework handler** — `Source/CkEcs/Public/CkEcs/EntityScript/CkEntityScript_SaveFields.h/.cpp` (CkEcs owns the
fragment + container registry + hydration scope; zero new deps).
- Payload USTRUCT `FCk_SaveData_EntityScriptFields { FString _ScriptClassPath; TArray<uint8> _FieldBytes; }`.
  The `FCk_SaveData_*` prefix deliberately ≠ `FCk_RepData_*` so the `Ck.Snapshot.Meta.RepDataRestoreCoverage` ratchet
  (enumerates `FCk_RepData_*`) is NOT tripped — confirm at gate time.
- Registrar mirrors `FCk_StateMachineRepHandlerRegistrar` (`CkStateMachine_Replication.cpp` ~:305) using **`RegisterLazy`**
  (NOT `RegisterLazyTyped` — it would synthesize a default SeedContainer → Model-A re-drive enrollment, [P4A-D1]), NO
  `SeedContainer`, `.Transport = ECk_PersistenceTransport::Save` (the FIRST Save-only handler; keeps it off the wire).
- `.Produce(Entity)`: return UNSET when NOT `Has<FFragment_EntityScript_Current>`, script weak-ptr invalid,
  `Get_InstancingPolicy()==NotInstanced` (shared CDO — writing on Apply would corrupt it), OR the script class has ZERO
  `CPF_SaveGame` FProperties (walk `TFieldIterator<FProperty>` + `HasAnyPropertyFlags(CPF_SaveGame)`) — this is the
  "no-op for scripts without SaveGame fields" answer. Else serialize: `FMemoryWriter` → `FObjectAndNameAsStringProxyArchive`
  with **`ArIsSaveGame=true`** + `SetIsPersistent(true)` (the SaveGame-flipped mirror of the v3 `SerializeInstancedStruct`
  idiom, `CkSnapshot_CaptureV3.cpp:92-113` — every existing snapshot archive sets `ArIsSaveGame=false`; this flip is the
  whole point, it's what makes the negation field drop) → `Script->SerializeScriptProperties(Proxy)`.
- `.Apply(Entity,New,Old)`: FIRST statement `if (NOT FCk_HydrationApplyScope::Get_IsActive()) return Applied;` (defensive —
  Save-only never rides the wire). Then NotReady until `Has<FFragment_EntityScript_Current>` + valid script; NotInstanced →
  warn+Applied (no-op); class-path mismatch → warn but keep applying (tagged-property reads are layout-tolerant); else
  symmetric `FMemoryReader` + proxy(`ArIsSaveGame=true`) + `SerializeScriptProperties` → Applied.
- Keying is AUTOMATIC (fragment on the script entity; Produce+Apply key on the same entity — no owner/child mismatch).
  `FTag_EntityScript_ConstructedThisFrame` dispatch-defer costs at most one pump pass; no special handling.
- Precedent to reuse: NONE exists for reflect-walking SaveGame props (grep confirmed) — this is the first `ArIsSaveGame=true`
  consumer. The comment at `CkActorSpawnIntent_Fragment.h:30` claiming the writer is `ArIsSaveGame=true` is STALE — ignore it.

**A.2 Tests** (CkTests — commit AFTER the CkF handler):
- AS fixture: `Plugins/CkTests/Script/CkSnapshot/CkSnapshot_AsSaveFields_EntityScript.as` — ONE class subclassing
  **`UCk_GenericEntityScript_UE`** (the Blueprintable/AS base; `UCk_EntityScript_UE` is NotBlueprintable) with
  `UPROPERTY(SaveGame) int`, `UPROPERTY(SaveGame) FString`, `UPROPERTY(SaveGame) TArray<int32>`, `UPROPERTY() int` (negation),
  `default _Replication = ECk_Replication::DoesNotReplicate;`. NOT a `UCk_AutoTest_Base`/`CkAutoTest_*` (so no wrapper gen).
- C++ gate: `Plugins/CkTests/Source/CkTests/Private/CkSnapshot/Test_Snapshot_AS_SaveFields_Gate.spec.cpp` — TWO
  `IMPLEMENT_SIMPLE_AUTOMATION_TEST` (EditorContext|EngineFilter). Template = the **M2a single-PIE harness**
  (`Test_Snapshot_M2a_LoadOrchestration_Gate.spec.cpp:100-110`, NumPIEClients=1, FramesForLoad=240 — Save transport needs no
  MP). Stages: spawn `ACk_AutoTest_NetSubject_M2aProbe` → resolve the AS `UClass` (mirror the Gauntlet matcher
  `CkGauntletAsBridgeController.cpp:263-271`, `TObjectIterator<UClass>` + `IsChildOf` + bare/prefixed name) → spawn the fixture
  as a RuntimeSpawned child of the probe entity via `Request_SpawnEntity(ProbeEntity, AsClass, {})` (probe HasBegunPlay ⇒
  RuntimeSpawned ⇒ recipe-carrying ⇒ Rule-4 persisted; owner persisted ⇒ loader respawns it — do NOT spawn under the
  transient = N1 hole) → mutate fields via reflection + precondition-assert `HasAnyPropertyFlags(CPF_SaveGame)` on the
  SaveGame ones (separates "AS specifier didn't take" from "framework didn't restore") → Save → Load → poll load-complete +
  probe re-present → walk `Get_LifetimeDependents` for the child by script-class → read fields. RoundTrip asserts the 3
  SaveGame fields == mutated; Drops asserts the plain int == class default (NOT the mutated value) + a SaveGame field
  restored (proves selective drop, not a broken payload). Note `Get_IsSnapshotRespawnable` is a C++ virtual, NOT AS-overridable.
- AS gotchas (from `ck-angelscript-interop`): never write `.as` during a live test run; after adding the fixture, ONE editor
  boot/AS recompile must precede the gate (`Script/Generated/` may churn — commit it with the CkTests commit, never hand-edit);
  grep the FRESH `Saved/Logs/*.log` for `Angelscript: Error` naming the fixture before claiming green.

**A.3 Gate + expected:** `--build --test --test-pattern "Ck.Snapshot" --discover-fresh` → **54/51/3** (the 3 reds stay the
[INV-A] trio by name); `--test --test-pattern "Net"` delta-zero. Confirm zero `Meta.RepDataRestoreCoverage` delta + no new
`v3 capture AUDIT` warnings. Commit: CkF `feat(CkEcs): FCk_SaveData_EntityScriptFields save-transport handler`; then CkTests
`test(CkSnapshot): AS SaveGame-field round-trip + negation gates`.

**A.4 Risks:** post-BeginPlay timing (record decision, above); poolable `InstancedPerEntity_Poolable` may carry stale plain
fields from a prior tenant (out of 4B.3 scope; fixture uses default instancing); AS class-name resolution fragility (mirror
the Gauntlet matcher; fail with a message distinguishing "AS compile failed" from "name mismatch").

## TRACK B — Phase 5 (PARTIAL now; FINAL is OracleParity/BB-driver-world-blocked)
**Gate-vs-delete boundary — "gate, don't delete" for everything the oracle/registry tests call:**
- **GATE under `#if CK_WITH_FIDELITY_ORACLE`** (`= !UE_BUILD_SHIPPING`, so it survives in all Dev/Test/Editor configs where
  tests run): the `CK_REGISTER_SNAPSHOTABLE` registrar body (gate the `Do_RegisterSnapshotable` call INSIDE the macro,
  `CkSnapshot_FragmentRegistry.h:145-159`; keep the `static_assert` at `:146-151` ungated), Model-A `Run_Capture`/`Run_Restore`
  + archives + tag machinery. **20 CkTests files / 55 `Run_Capture|Run_Restore` call sites depend on this** (Core/DynamicFragment/
  LifecycleStrip/SaveKey/SceneNode/…) — it stays as the deep-diff regression net.
- **DELETE (shipping-dead):** the Model-A dual-write in `Request_Save` (`CkSnapshot_Subsystem.cpp:201,215-220,237-238`) +
  `UCk_Snapshot_SaveGame::_SnapshotBytes/_Header` Model-A section; `FTag_Snapshot_JustRestored` + `FProcessor_Persistence_ReDriveOnRestore`
  + the six deferred `*_ReplicateOnRestore` processors (Team/Player/Inventory×2/RenderTarget/2dGridOccupancy) + CkGrid
  `FProcessor_2dGridSystem_RestoreRecompose`; `FProcessor_ActorRespawn` (dormant — VERIFIED nothing stamps `FTag_ActorRespawn_Pending`;
  M2b2a exercises the fresh-Construct replication path instead).
  - ⚠️ RenderTarget: delete ONLY `FProcessor_RenderTarget_ReplicateOnRestore` (its logic was transplanted to the LIVE
    `FProcessor_RenderTarget_HandleRequests::HydrateFromSavedChannel`, [P4B-D3] — that stays).
  - ⚠️ **KEEP `CkActorRebind_Utils` + `FTag_ActorJustRebound`** — its only production stamper is the dying ActorRespawn processor,
    BUT `FTag_ActorJustRebound` has LIVE cross-repo consumers (BB `Bb_SnapshotRestore.cpp`, AS `BB_PlayerCharacter*.as`);
    deleting it breaks the BB AS compile at editor boot. Keep with a retirement comment; flag BB-side dead-code cleanup as a
    SEPARATE Adam/BB-repo task (a CkFoundation session must NOT edit the BB superproject).
  - ⚠️ VERIFY-BEFORE-DELETE: `Ck.Snapshot.FloatAttribute.Gate` is the only Model-A LIVE-world restore test (where JustRestored
    still fires) — run it before AND after the JustRestored/re-drive delete; if it regresses, it needs its documented rewrite
    (VALIDATION.md allows it), not a delete rollback.
  - `FFragment_ActorSpawnIntent` STAYS (feeds v3's `_ActorClassPath`). `Get_IsSnapshotRespawnable` STAYS ([P3B-D1]) — so
    **amend PHASE_5.md §5.3's exit grep** from `IsSnapshotRespawnable` to `ReplicateOnRestore|RestoreReplicated|JustRestored|Reconstitution`.
- **Phase 5 CAN complete now:** 5.1 (oracle-gate Model A; verify Ck.Snapshot green + a Shipping compile
  `--build --target Game --config Shipping`), 5.2.1 (dual-write delete), the ActorRespawn delete (Rebind KEPT), 5.2.2 (with the
  FloatAttribute.Gate pre/post check), the Transform-`_Previous` half of 5.2.3, 5.2.4 docs, the amended 5.3 grep. Commit per
  cluster, compile between clusters. Mark Phase 5 "IN PROGRESS — partial; final blocked".

## BLOCKERS (do NOT attempt without an Adam decision)
- **[INV-A]** (Adam, product): Grid + Inventory×2 — the 3 standing Ck.Snapshot reds. Anonymous item/occupant entities that
  don't round-trip (payload-enrichment vs item-spawn-recipe rearchitecture vs post-campaign).
- **[N1-A]** (Adam, product): boot-singleton adoption (BB StoreDriver own-state) + 4A.2 N1 discriminator + `Rebuild.SpawnerResumesPastSpawnDecision`.
- **BB-driver-world prerequisite** (shared): blocks `Rebuild.OracleParity` (doesn't exist yet; oracle-allowlist-p3.txt empty;
  oracle-declared-transient.txt not created) → therefore 4B.1 params-mutators + MontagePlayer rebind, the 5.2.3 camera-revert
  fence, VALIDATION §2, and **Phase-5 FINAL**. Also VALIDATION-required-but-nonexistent: `Rebuild.{NoDuplicateGrants,LostGrantStaysLost,
  OrphanHydrationLoud}` (3B follow-ups — heavier PIE tests, NOT decision-blocked, schedulable any session as optional filler).
- **New small human decisions surfaced by this scoping** (record loudly; (a) is an executor default):
  (a) accept the post-BeginPlay SaveGame-field timing as the contract (Track A);
  (b) BB-repo dead-code cleanup of `FTag_ActorJustRebound` consumers — cross-repo, NOT an executor call from a CkF session;
  (c) VALIDATION.md line-item amendments (`Physics.Net.Velocity_ApplyAfterLateSetup` was subsumed per [P2-D3] and never written;
      the 5.3 grep pattern) — mechanical but they edit CTO-fixed acceptance text.

## SEQUENCE
1. **Track A (4B.3)** first — closes 2 of the 7 VALIDATION-required tests; last actionable 4B item.
2. **Track B (Phase 5 partial)** — 5.1 → 5.2.1 → ActorRespawn delete → 5.2.2 → 5.2.4/grep/Shipping-compile, gating between clusters.
3. Optional filler: the 3 unblocked `Rebuild.*` PIE tests (not decision-gated).

## PROTOCOL (unattended)
Implement on Opus; gate via `CkAuto\UnrealToolbox.exe --build --test ... --output <log>` (editor CLOSED; read verdicts from the
--output log, NOT the notification). Commit per feature/cluster when green + delta-zero. Route design forks to a read-only Fable
agent, VERIFY its rulings against cited code, record in PROGRESS §Decisions, return to Opus. STOP only for: a red gate you can't
fix even after a Fable consult; an irreversible/outward action; or a genuine human-only product/risk decision Fable flags for Adam.
Cross-repo "CkTests never ahead of / merged before CkFoundation" + "no push" hold absolutely.
