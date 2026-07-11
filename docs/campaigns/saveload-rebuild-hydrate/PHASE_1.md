# PHASE 1 — Unify the handler registry: `Produce`/`SeedContainer`, one re-drive processor, local hydration queue

Behavior-neutral under Model A. The 12 per-feature `*_ReplicateOnRestore` processors (16 registrations) and their
9 done-tags are deleted, replaced by ONE framework processor driven by the registry.

## Entry criteria
- PROGRESS.md shows Phase 0 done; `git log --oneline -4` shows the Phase-0 commits.
- Baseline table exists in PROGRESS.md. Re-run `--test --test-pattern "Ck.Snapshot"` → matches it. Else STOP.

## Steps

### 1.1 Extend `FCk_ReplicatedFragmentHandlerRegistry::FHandler`
`Source/CkEcs/Public/CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h` (`FHandler`, `:43-56`).
Add (house comments in the same register as the existing Apply comment):

```cpp
// Authority-side counterpart to Apply: emit this feature's current hydration payload for the
// entity, or unset when the feature is absent / has nothing to persist. Used by the restore
// re-drive (Phase 1), the fidelity oracle (Tier-2), and the save path (Phase 3A).
TFunction<TOptional<FInstancedStruct>(FCk_Handle& Entity)> Produce;

// Typed container seed bound at registration (RegisterLazyTyped) — re-establishes the FastArray
// entry + the entity-side TFragment_ContainerEntryRef<T> that the feature's Replicate processor
// keys on. Type-erased callers cannot do this themselves (the ContainerRef fragment is typed).
TFunction<ECk_AddedOrNot(FCk_Handle& Entity, const FInstancedStruct& Data)> SeedContainer;

// Which pipelines consult this handler. Net-only default keeps Phase 1 behavior-neutral;
// features flip Save on when their payload becomes save-file surface (Phase 3+).
ECk_PersistenceTransport Transport = ECk_PersistenceTransport::Net;
```
New enum beside `ECk_RepFragment_ApplyResult`:
```cpp
enum class ECk_PersistenceTransport : uint8 { Net = 1 << 0, Save = 1 << 1, NetAndSave = Net | Save };
```
New registration helper (template, in the same header, below `RegisterLazy`):
```cpp
// Registers a handler whose payload type is T_RepData: fills SeedContainer with the typed
// TryAddContainerFragment call and resolves the type lazily. Feature registrars migrate to this.
template <typename T_RepData>
static auto
RegisterLazyTyped(
    FHandler InHandler) -> void;
```
Implementation: wrap `InHandler`, set `SeedContainer = [](FCk_Handle& E, const FInstancedStruct& D) {
return UCk_Utils_Net_UE::TryAddContainerFragment<T_RepData>(E, D.Get<T_RepData>()); }`, resolver
`[]{ return T_RepData::StaticStruct(); }`, forward to `RegisterLazy`. (Captureless lambdas; fence 1/2 in PROMPT.)
NOTE: `CkNet_Utils.h` is includable from this header's .cpp only — put the template body in an `.inl` or make it a
thin declaration whose body lives where `TryAddContainerFragment` is visible; follow whichever the neighboring code
supports with the LEAST new include surface, record the choice in PROGRESS.md.

### 1.2 Migrate the 12 features' registrars to `RegisterLazyTyped` + add `Produce`
For each feature below, in its `_Fragment.cpp` registrar: switch `RegisterLazy` → `RegisterLazyTyped<RepData>`,
add a `Produce` that mirrors what its `*_ReplicateOnRestore` processor built (read that processor FIRST — it names
exactly which Current state seeds the payload), leave `Apply` untouched.

| Feature | Registrar file | ReplicateOnRestore to mirror-then-delete |
|---|---|---|
| Velocity | `CkPhysics/.../Velocity/CkVelocity_Fragment.cpp` | `CkVelocity_Processor.h:241` + `.cpp:337-372` |
| Acceleration | `CkPhysics/.../Acceleration/CkAcceleration_Fragment.cpp` | `CkAcceleration_Processor.h:216` + `.cpp:~285` |
| Team | `CkRelationship/.../Team/CkTeam_Fragment.cpp` | `CkTeam_...:71` |
| Player | `CkRelationship/.../Player/CkPlayer_Fragment.cpp` | `...:25` |
| Float/Integer/Byte/Vector/Rotator Attribute | `CkAttribute/.../<Kind>Attribute/Ck<Kind>Attribute_Fragment.cpp` | `CkAttribute_ReplicateOnRestore.h` (one template, 5 instantiations) |
| TagSet | `CkTagSet/CkTagSet_Fragment.cpp` | `CkTagSet_Processor.cpp:137-160` |
| MontagePlayer / AnimPlan | `CkAnimation/...` | headers `:121` / `:92` |
| Inventory Spatial / DataOnly | `CkInventory/...` | `:89` / `:75` |
| RenderTarget | `CkRenderTarget/Net/CkRenderTarget_Replication.cpp` | `:81` — **special: its payload is the authored batch ring; Produce emits the persisted AuthoredLog-derived RepData exactly as its restore pass builds it (`CkRenderTarget_Processor.cpp:459-503`)** |
| 2dGridOccupancy | `CkGrid/.../Occupancy/Ck2dGridOccupancy_Fragment.cpp` | `:82` |
| StateMachine | **SKIP** — `FProcessor_Sm_RestoreRedrive` is a control-flow replay, not a rep-seed. It stays until Phase 4A. Do not touch. |

### 1.3 ONE framework re-drive processor
New: `Source/CkEcs/Public/CkEcs/Net/ReplicatedFragmentContainer/CkPersistence_ReDrive_Processor.h/.cpp`.
```cpp
// Replaces the per-feature *_ReplicateOnRestore family: on FTag_Snapshot_JustRestored entities whose
// replication driver is re-established, every registered handler with Produce+SeedContainer re-seeds
// its container entry from live state. Per-entity progress in FFragment_Persistence_ReDrivePending
// (remaining payload types); retry until the driver accepts (NotAdded => retry next tick, loud after
// the same 5s/2s window the dispatcher uses).
class CKECS_API FProcessor_Persistence_ReDriveOnRestore : public ck::TProcessor<...>
{   using Group = FGroup_Replication;   // same neighborhood the old per-feature ones lived in
    static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly; ... };
```
Fragment (same header): `struct FFragment_Persistence_ReDrivePending { TArray<TWeakObjectPtr<const UScriptStruct>> _Remaining; float _PendingForSeconds = 0; }`.
Flow per entity: first sight of `FTag_Snapshot_JustRestored` + `UCk_Utils_EntityReplicationDriver_UE::Has` →
populate `_Remaining` from all registered handlers with `Produce` → each tick: for each remaining type, `Produce`;
unset → drop from remaining (feature absent on this entity); set → `SeedContainer`; `NotAdded` → keep, accumulate
timeout (CK_ENSURE + drop when exceeded); on empty → remove fragment. Do NOT remove `FTag_Snapshot_JustRestored`
(multi-consumer until Phase 4A; the pending-fragment IS this processor's done-marker).
Registrar `CK_REGISTER_PROCESSOR` in the .cpp.

### 1.4 Delete the 12 features' restore machinery
Delete each `*_ReplicateOnRestore` processor class + its `CK_REGISTER_PROCESSOR` + its `FTag_*_RestoreReplicated` /
`FTag_*_RestoreRedriven` done-tag (StateMachine's excluded). Verification:
`rg --no-ignore -n "ReplicateOnRestore|RestoreReplicated" Plugins/CkFoundation/Source` → expected remaining hits:
StateMachine's redrive only + any comment references you consciously updated. Anything else → you missed one.

### 1.5 Local hydration queue + its own dispatch processor (dormant until Phase 3B)
New fragment beside the container types: `struct FFragment_PendingHydration { TArray<FInstancedStruct> _Entries;
float _PendingForSeconds = 0; }` + transient tag `FTag_Hydration_PendingApply` (writer adds both).
New SIBLING processor `FProcessor_Hydration_Dispatch` (same file family as the net dispatcher, sharing its
handler-resolution/timeout code via a named-namespace helper — do NOT duplicate the drain logic): drains
`FFragment_PendingHydration` through `Resolve()->Apply` with identical NotReady/5s-2s-timeout semantics; entry
removed on Applied, fragment+tag removed when empty. Differences from the net dispatcher: `NetModeRequirement =
All` (a loading standalone/authority world hydrates locally), same `Group` as the net dispatcher (moves with it in
Phase 2), `MarkedDirtyBy = FTag_Hydration_PendingApply`.

### 1.6 Oracle Tier-2 (Produce-diff)
Extend `CkSnapshot_FidelityOracle.h/.cpp`:
```cpp
CKECS_API auto Capture_Payloads(ck::SnapshotRegistryType&, FCk_RegistryHandle) -> TMap<FString /*sig*/, TArray<TPair<FString /*type*/, FInstancedStruct>>>;
CKECS_API auto Diff_Payloads(const ..., const ...) -> TArray<FString>;
```
Runs every registered `Produce` per entity, keyed by the Tier-1 signature; diff = per-type payload
`ExportText`-style comparison. New test `Ck.Snapshot.Oracle.ProduceDiffBaseline` (CkTests, same fixture as 0.6):
mutate one attribute value between two captures → diff reports exactly one line; unmutated → empty.

### 1.7 Gate + commit
```powershell
CkAuto\UnrealToolbox.exe --build --test --test-pattern "Ck.Snapshot" --discover-fresh --output CkAuto\logs\p1-snapshot.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Net" --output CkAuto\logs\p1-net.log
```
**Decision gate:** the 11 `Ck.Snapshot.Parity.*_MPReload` specs are THE proof the generic re-drive equals the 12
deleted processors (they assert restored values reach clients). Expected: all green, full delta-zero vs baseline,
+2 new oracle tests green. One or more Parity specs red → your `Produce` for that feature diverges from what its
old restore processor seeded — re-read that processor's body (it is still in git history), fix Produce ONLY. Red
elsewhere → STOP → Blockers.

Commits: (CkFoundation) `feat(CkEcs): Produce/SeedContainer persistence contract + generic restore re-drive`;
`refactor(12 modules): delete per-feature ReplicateOnRestore processors + done-tags` (may be one commit per module
if cleaner); (CkTests) `test(CkSnapshot): oracle Produce-diff baseline`.

## Exit criteria
- `rg -c "RegisterLazyTyped" Source` ≥ 12 registrar files (CkFoundation).
- `rg --no-ignore -l "ReplicateOnRestore" Source` → only CkStateMachine (+ this campaign's docs).
- All 11 Parity specs named green in the log; delta-zero; PROGRESS updated.

## Fences
- Do NOT flip any handler's `Transport` to Save yet.
- Do NOT touch StateMachine's redrive (Phase 4A) or the dispatcher's GROUP (Phase 2).
- **`Produce` is READ-ONLY by contract** (state it in the FHandler comment). Feature-side work that must accompany
  a re-seed — e.g. the attribute template's `FTag_MayRequireReplication` re-arms
  (`CkAttribute_ReplicateOnRestore.h:90-98`) — belongs in that feature's registrar-supplied `SeedContainer` lambda
  (do the typed `TryAddContainerFragment`, then the re-arms, in that lambda). `RegisterLazyTyped` must therefore
  accept an OPTIONAL caller-supplied `SeedContainer` override; when absent it synthesizes the default typed seed.
