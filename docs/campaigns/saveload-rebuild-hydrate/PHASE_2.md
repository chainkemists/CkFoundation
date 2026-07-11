# PHASE 2 — Load gate (`ECk_ProcessorLoadPolicy`), `FGroup_Hydration` late dispatch, ReplicationComplete fire-gating

Load `ckecs-domain-reference` + `ckecs-architecture-contract` before starting.

## Entry criteria
- PROGRESS shows Phase 1 done; snapshot pattern green at Phase-1 counts.

## Steps

### 2.1 `ECk_ProcessorLoadPolicy` trait (mirror `PumpPolicy` exactly)
1. `Scheduler/CkProcessorDescriptor.h`: beside `ECk_ProcessorPumpPolicy` (`:79-84`):
   ```cpp
   // Whether a processor ticks while a CkSnapshot load is rebuilding the world. Default = gated:
   // feature processors do nothing mid-load and need no knowledge of loads. Only the framework
   // construction/lifecycle/hydration kernel opts in (spec §4.3).
   enum class ECk_ProcessorLoadPolicy : uint8 { GatedDuringLoad, RunsDuringLoad };
   ```
   + field `ECk_ProcessorLoadPolicy _LoadPolicy = ECk_ProcessorLoadPolicy::GatedDuringLoad;` on `FProcessorDescriptor`.
2. `Scheduler/CkProcessorTraits.inl.h` `BuildDescriptor` (~`:216`, beside the PumpPolicy block):
   `if constexpr (requires { T_Processor::LoadPolicy; }) { Descriptor._LoadPolicy = T_Processor::LoadPolicy; }`
3. Mirror onto `FProcessorGraphNode` exactly as `_PumpPolicy` is mirrored (`CkProcessorGraph.h:74`, copied in
   `DoCreateNodes` `CkProcessorGraph.cpp:205,242`).
4. `Scheduler/CkProcessorScheduler.{h,cpp}`: precompute `_LoadPassOrder` (subset of `_MainPassOrder` where
   `RunsDuringLoad`) and `_LoadPumpOrder` in the ctor beside `:69-80`; `Tick` gains
   `ECk_SchedulerTickScope InScope = ECk_SchedulerTickScope::Full` (new enum `{Full, LoadKernel}` in the scheduler
   header) selecting which order the main pass + pump iterate.
5. `Subsystem/CkEcsWorld_Subsystem.h/.cpp`: `Get_/Set_IsLoadGateActive()` (plain bool member, same shape as the
   reconstitution accessors at `.cpp:139-162`); `ACk_EcsWorld_Actor_UE::Tick` (`.cpp:35-50`) resolves the subsystem
   once (cache a weak ptr on the actor at spawn) and passes `LoadKernel` scope when active.
   `Request_PumpToQuiescence` (`.cpp:235-262`) gains a scope parameter, defaulted `Full` (pre-save behavior
   unchanged); the load orchestrator (Phase 3B) will call it with `LoadKernel`.

### 2.2 Mark the kernel (exhaustive list — do not extend it)
Add `static constexpr auto LoadPolicy = ECk_ProcessorLoadPolicy::RunsDuringLoad;` to EXACTLY these classes:
- CkEcs EntityScript (`CkEntityScript_Processor.h`): `SpawnEntity_HandleRequests`, `ContinueConstruction`,
  `Replicate`, `FinishConstruction`, `PendingReplicationRetry`, `BeginPlay`.
- CkEcs EntityLifetime: `EntityLifetime_EntityJustCreated` ONLY.
- CkEcs Net: `ReplicatedFragments_Dispatch`, `FProcessor_Hydration_Dispatch` (Phase 1.5),
  `ReplicationDriver_FireOnDependentReplicationComplete`, `Persistence_ReDriveOnRestore` (Phase 1).
- CkEcsExt: `ActorRespawn`.
Everything else stays default-gated — **including the ENTIRE destruction pipeline** (`DestroyEntity`, all four
`DestructionPhase_*`, `EntityScript_EndPlay`, `OwningActor_Destroy`). Reason (do not revisit): feature `*_EndPlay`
processors are gated during load; if the pipeline advanced an entity EndPlay→Teardown while they were gated, their
cleanup window would be silently skipped and world-side components would leak. Destruction requested during a load
(e.g. reconciliation) therefore PARKS at the deferred stage and drains — with full feature participation — on the
first normal frame after gate-open. Nothing in the load requires a destruction to COMPLETE mid-load.
(Transform/SceneNode also deliberately gated — restored positions arrive via hydration + actor spawn transforms.)

### 2.3 `FGroup_Hydration` + move the dispatcher
`Scheduler/CkProcessorGroups.h`: insert `FGroup_Hydration` into the chain between `FGroup_PostTransform` and
`FGroup_Replication` (declare the struct + edit the chain comment `:11-28`; register it wherever the sibling groups
are registered — find with `rg -n "CK_REGISTER_GROUP" Source/CkEcs`). Change
`CkReplicatedFragmentContainer_Processor.h:24-35`: `using Group = FGroup_Hydration;` (keep `RunAfter
FinishConstruction`? NO — FinishConstruction is in `FGroup_Gameplay_Script`, a different group; group order alone
now guarantees it. DELETE the RunAfter, note it in the commit message).

### 2.4 ConstructedThisFrame defer (closes the pump-phase stomp)
Groups `FGroup_Gameplay_TimeDelta/Gameplay/AI/Audio/Rendering` run BEFORE `FGroup_Gameplay_Script` in the main
pass — an entity composed this frame has those features' Setups still pending (they drain in the pump, AFTER
`FGroup_Hydration`). Mechanism:
- New transient tag `FTag_EntityScript_ConstructedThisFrame` (`CK_DEFINE_ECS_TAG_TRANSIENT`, in
  `CkEntityScript_Fragment.h` beside the lifecycle tags `:23-27`). `FProcessor_EntityScript_FinishConstruction`
  adds it when construction completes (same place the `.cpp:182`-area stamps run).
- Dispatcher skips entities carrying it (both FastArray entries and `FFragment_PendingHydration`).
- Dispatcher's `DoTick`, AFTER the per-entity loop: `_TransientEntity.Clear<FTag_EntityScript_ConstructedThisFrame>();`
  (registry-wide clear — the idiom at `CkIsmRenderer_Processor.cpp:24-31`). Next frame's dispatch applies.

### 2.5 Fire-gating (CTO blocker 2 + note N2: aggregate over dependents)
`Net/EntityReplicationDriver`: new helper on `UCk_Utils_EntityReplicationDriver_UE` (or Net utils — put it beside
`Get_IsReplicationCompleteAllDependents`, `CkEntityReplicationDriver_Utils.cpp:381-405`):
```cpp
// True while this entity OR any dependent driver's entity still has replicated-fragment entries
// pending apply (FTag_RepFragments_PendingApply / queued removals / FFragment_PendingHydration).
static auto Get_HasUndrainedReplicatedFragments_IncludingDependents(const FCk_Handle& InHandle) -> bool;
```
Traverse the same dependent set the counters at `CkEntityReplicationDriver_Fragment.cpp:377-408` maintain.
`FProcessor_ReplicationDriver_FireOnDependentReplicationComplete` (`CkEntityReplicationDriver_Processor.cpp:19-32`):
before broadcasting, if the helper returns true → return WITHOUT consuming the fire tag (retry next tick). The
dispatcher's 5s/2s timeout bounds a stuck entry, so the fire cannot hang forever.

### 2.6 Retire the `5eda3ac8a` guards
Delete the `Has<FTag_*_NeedsSetup> → NotReady` blocks in `CkVelocity_Fragment.cpp` and
`CkAcceleration_Fragment.cpp` Apply handlers (the late group + 2.4 + 2.5 supersede them). Keep the
`TryAddContainerFragment` NotAdded-retry part of that commit — it lives in the deleted ReplicateOnRestore
processors, already gone in Phase 1; verify nothing of `5eda3ac8a` remains except history:
`git show 5eda3ac8a --stat` and check each file's current state.

### 2.7 New tests (CkTests)
1. `Test_Snapshot_LoadGate_Scope.spec.cpp` → `"Ck.Snapshot.LoadGate.GatedSkipsKernelTicks"`: register two trivial
   test-only processors (one default, one `RunsDuringLoad`), both counting `ForEachEntity` hits on a fixture entity
   (test-processor precedent: the Dynamic pump tests / CkTests fragment fixtures — find with
   `rg -l "CK_REGISTER_PROCESSOR" Plugins/CkTests/Source`). Flip `Set_IsLoadGateActive(true)`, tick N frames:
   assert gated counter == 0 AND kernel counter > 0 (the inverse assertion — over-gating is a silent hang);
   flip off, assert gated counter resumes.
2. `Test_Physics_Net_ApplyAfterLateSetup.spec.cpp` → `"Ck.Physics.Net.Velocity_ApplyAfterLateSetup"` (stomp repro):
   MP spec (copy the shape of `Test_Snapshot_AccelerationParity_MPReload_Gate.spec.cpp`'s client/server plumbing or
   the `CkAutoTest_NetSubject_*` fixtures in `Plugins/CkTests/Source/CkTests/Private/CkTests/Net/`): compose a
   velocity entity so the replicated value arrives while `FTag_Velocity_NeedsSetup` is pending; assert the client's
   final velocity == replicated value (pre-fix this loses to Setup's Params seed).

### 2.8 Gate + commit
```powershell
CkAuto\UnrealToolbox.exe --build --test --test-pattern "Ck.Snapshot" --discover-fresh --output CkAuto\logs\p2-snapshot.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Ck.Attribute.Net" --output CkAuto\logs\p2-attrnet.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Net" --output CkAuto\logs\p2-net.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Ck.Physics.Net" --output CkAuto\logs\p2-physnet.log
```
**Decision gates:**
- The three pins green (`Values_AppliedBefore_OnReplicationComplete`, `Float_InitialBakedValue_Replicates`,
  `Float_PreComposition_StashedValue_Applies`). `Values_AppliedBefore...` red → your fire-gating misses a pending
  source (check N2: dependents, removals, hydration fragment) → fix helper, re-run. Still red after one fix → STOP.
- Both new tests green; snapshot + net delta-zero vs baseline.
- Any test HANGING (toolbox timeout) → suspect over-gating (a kernel-needed processor left gated) → check the 2.2
  list was applied exactly → if the list itself seems wrong → STOP → Blockers (do NOT extend the kernel yourself).

Commits: `feat(CkEcs): ECk_ProcessorLoadPolicy trait + scheduler load-kernel scope`;
`feat(CkEcs): FGroup_Hydration late dispatch + ConstructedThisFrame defer`;
`feat(CkEcs): gate OnReplicationComplete fire on pending-apply drain (incl. dependents)`;
`refactor(CkPhysics): retire per-feature NeedsSetup apply-guards`; (CkTests) `test: load-gate scope + late-setup stomp repro`.

## Exit criteria
- `rg -n "LoadPolicy = ECk_ProcessorLoadPolicy::RunsDuringLoad" Source | wc -l` == 12 (6 EntityScript +
  EntityJustCreated + 2 dispatchers + fire + Persistence_ReDrive + ActorRespawn — recount against 2.2 and
  reconcile BEFORE committing).
- All logs green per gates above; PROGRESS updated with the new test names added to the protected inventory.

## Fences
- Do NOT mark any feature processor RunsDuringLoad (including Setups — they drain post-gate by design).
- Do NOT build any "setup settled" predicate (rejected; see PROMPT).
- Do NOT change the net dispatcher's ClientOnly requirement. The local-queue drain is already the sibling
  `FProcessor_Hydration_Dispatch` (Phase 1.5, NetMode All) — in THIS phase move BOTH dispatchers to
  `FGroup_Hydration`, mark BOTH `RunsDuringLoad` (the kernel list's "Dispatch" entries cover both), and apply the
  2.4 ConstructedThisFrame skip to both.
