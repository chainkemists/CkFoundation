# PHASE 1 — Unify the handler registry: `Produce`/`SeedContainer`, one re-drive processor, local hydration queue

> **REVISED 2026-07-11 after blocker [B1]** (see `PHASE_1_RESEARCH.md` + PROGRESS.md). Ruling: **migrate the 6
> clean features only** (Velocity, Acceleration, Attribute×5, TagSet, MontagePlayer, AnimPlan — 10 of the 16
> registrations); **DEFER 6** (Team, Player, Inventory Spatial, Inventory DataOnly, RenderTarget, 2dGridOccupancy)
> whose restore processors also do non-container reconstitution (child re-replication, grid re-stamp, RT
> re-create/repaint, unconditional tag re-derive). The deferred processors stay **VERBATIM — do not touch them**:
> that work is Model-A repair which Model B retires structurally (items→recipes, grids→Construct, RT→Construct +
> Phase-4B re-author, Team/Player tags→normal Assign in hydration Apply). They go inert at Phase 3B (v3 loads never
> stamp `FTag_Snapshot_JustRestored`) and are deleted in Phase 5. Do NOT build a `Reconstitute` hook and do NOT
> slim the deferred processors — both rejected (dead-end surface / churn without end-state value).
> **Participation rule (encode in the FHandler comment):** `SeedContainer` present ⇒ handler participates in the
> Model-A generic re-drive; `Produce` WITHOUT `SeedContainer` ⇒ capture/oracle-only (this is how the deferred 6
> gain `Produce` at Phase 3A without double-seeding against their still-alive restore processors).

Behavior-neutral under Model A: 10 of 16 `*_ReplicateOnRestore` registrations and the migrated features' done-tags
are deleted, replaced by ONE framework processor driven by the registry.

**Implementation source of truth for the migrations: the per-feature recipe table in `PHASE_1_RESEARCH.md`**
(Produce reads, SeedContainer extras — owner-hosted containers, upsert-merge for attributes, `MayRequireReplication`
re-arms, AnimPlan's SET-but-empty payload). Where this doc and the research table disagree on a feature detail,
the research table wins (it is code-verified).

## Entry criteria
- PROGRESS.md shows Phase 0 done and [B1] RESOLVED; `git log --oneline -6` shows the Phase-0 commits + `5a6baf5a9`.
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
Implementation + include surface: **re-apply the reverted §1.1 design verbatim from `PHASE_1_RESEARCH.md`'s last
section** (it is blessed): `RegisterLazyTyped<T>` body in a new `CkReplicatedFragmentContainer.inl.h` included at
the very bottom of `CkNet_Utils.h` (both the registry class and `TryAddContainerFragment` complete there; feature
registrars already include `CkNet_Utils.h` → zero new include surface). Body synthesizes the default captureless
typed seed only when the registrar supplied no custom `SeedContainer`, then forwards to `RegisterLazy`.

### 1.2 Migrate the SIX clean features' registrars to `RegisterLazyTyped` + add `Produce`
For each, in its `_Fragment.cpp` registrar: switch `RegisterLazy` → `RegisterLazyTyped<RepData>`, add `Produce` +
(where the research table says so) a custom `SeedContainer` lambda, leave `Apply` untouched. **Follow the
`PHASE_1_RESEARCH.md` recipe table row-by-row** — it carries the code-verified details (owner-hosted containers
for attributes/AnimPlan, upsert-merge keyed by (name,component), re-arm tags, AnimPlan's empty-payload seed).

Migrate: **Velocity, Acceleration, FloatAttribute+IntegerAttribute+ByteAttribute+VectorAttribute+RotatorAttribute
(5 registrar files, shared recipe), TagSet, MontagePlayer, AnimPlan** — 10 registrar files.

Do NOT touch (deferred — see the header ruling): **Team, Player, Inventory Spatial, Inventory DataOnly,
RenderTarget, 2dGridOccupancy** (their registrars keep plain `RegisterLazy`; their restore processors stay).
StateMachine: **SKIP** as before (`FProcessor_Sm_RestoreRedrive` is control-flow replay; Phase 4A).

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
populate `_Remaining` from all registered handlers having **BOTH `Produce` AND `SeedContainer`** (the participation
rule — Produce-only handlers are capture/oracle-only and must never be re-driven) → each tick: for each remaining
type, `Produce`; UNSET → drop from remaining (feature absent on this entity; note: a SET-but-empty payload — the
AnimPlan case — is seeded, not dropped); SET → `SeedContainer`; `NotAdded` → keep, accumulate timeout (CK_ENSURE +
drop when exceeded); on empty → remove fragment. Do NOT remove `FTag_Snapshot_JustRestored` (multi-consumer — the
6 deferred restore processors and Phase-4A SM still key on it; the pending-fragment IS this processor's
done-marker). Registrar `CK_REGISTER_PROCESSOR` in the .cpp.

### 1.4 Delete the SIX migrated features' restore machinery
Delete each migrated feature's `*_ReplicateOnRestore` processor class + `CK_REGISTER_PROCESSOR` + done-tag, per the
research table's Delete column (incl. the shared `CkAttribute_ReplicateOnRestore.h` template + its 5 registrations
+ shared `FTag_Attribute_RestoreReplicated`). **Relocation note:** nothing from the deleted 6 needs relocating
(Team/Player's `DoRestore*IDTag` helpers belong to DEFERRED processors — untouched). Verification:
`rg --no-ignore -l "ReplicateOnRestore|RestoreReplicated|RestoreRedriven" Plugins/CkFoundation/Source` → expected
remaining files: CkStateMachine (redrive), CkTeam/CkPlayer (CkRelationship), CkInventory Spatial + DataOnly,
CkRenderTarget, CkGrid Occupancy — and NOTHING from Velocity/Acceleration/Attribute/TagSet/Montage/AnimPlan.
More or fewer files → you deleted the wrong set → STOP before committing.

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
**Decision gate:** all 11 `Ck.Snapshot.Parity.*_MPReload` specs green + full delta-zero vs baseline (incl. the one
pre-existing `Net` red by name) + 2 new oracle tests green. Proof mapping: the MIGRATED features are proven by
`Parity.Acceleration/AnimPlan/Attributes/TagSet_MPReload` plus `Ck.Snapshot.MontagePlayer.StateRoundTrip` and the
velocity/physics round-trip coverage; the DEFERRED features' specs (`Parity.TeamPlayer/InventorySpatial/
InventoryDataOnly/RenderTarget/GridPlacements_MPReload`) must be green via their UNTOUCHED processors — if one of
those goes red you touched something the ruling said not to. A migrated feature's parity spec red → your `Produce`/
`SeedContainer` diverges from the research recipe — fix against the recipe ONLY. Red elsewhere → STOP → Blockers.

Commits: (CkFoundation) `feat(CkEcs): Produce/SeedContainer persistence contract + generic restore re-drive`;
`refactor(CkPhysics,CkAttribute,CkTagSet,CkAnimation): delete migrated ReplicateOnRestore processors + done-tags`;
(CkTests) `test(CkSnapshot): oracle Produce-diff baseline`.

## Exit criteria
- `rg --no-ignore -l "RegisterLazyTyped" Source` → exactly the 10 migrated registrar files (+ the registry
  header/.inl.h).
- `rg --no-ignore -l "ReplicateOnRestore|RestoreReplicated|RestoreRedriven" Source` → exactly the 7 deferred/SM
  files named in §1.4 — no attribute/velocity/acceleration/tagset/montage/animplan hits.
- All 11 Parity specs named green in the log; delta-zero; PROGRESS updated ([B1] marked resolved-and-executed).

## Fences
- Do NOT flip any handler's `Transport` to Save yet.
- Do NOT touch StateMachine's redrive (Phase 4A), the 6 deferred features' restore processors/registrars (Phase
  3A adds their Produce; Phase 5 deletes their processors), or the dispatcher's GROUP (Phase 2).
- **`Produce` is READ-ONLY by contract** (state it in the FHandler comment). Feature-side work that must accompany
  a re-seed — e.g. the attribute template's `FTag_MayRequireReplication` re-arms
  (`CkAttribute_ReplicateOnRestore.h:90-98`) — belongs in that feature's registrar-supplied `SeedContainer` lambda
  (do the typed `TryAddContainerFragment`, then the re-arms, in that lambda). `RegisterLazyTyped` must therefore
  accept an OPTIONAL caller-supplied `SeedContainer` override; when absent it synthesizes the default typed seed.
