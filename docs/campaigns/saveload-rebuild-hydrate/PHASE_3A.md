# PHASE 3A — Save side: provenance stamping, spawn-recipe retention, format-v3 writer

Model A capture stays untouched and green; v3 capture is written ALONGSIDE it (Model A becomes oracle-only in
Phase 5). Spec §4.2 is the design authority for everything here — re-read it first.

## Entry criteria
- Phase 2 done per PROGRESS; snapshot + net patterns green at Phase-2 counts.

## Steps

### 3A.1 `FTag_ConstructSpawned` stamping (framework-central)
- Tag: `CK_DEFINE_ECS_TAG_TRANSIENT(FTag_ConstructSpawned)` in `CkEcs/EntityLifetime/`. TRANSIENT is deliberate:
  the v3 writer reads the LIVE tag at capture time and records provenance as entity-table metadata — the tag itself
  must never round-trip through Model A's opt-out tag capture while both models coexist.
- Stamp site: the entity-creation path in `UCk_Utils_EntityLifetime_UE::Request_CreateEntity` (and the
  EntityScript spawn path if it doesn't route through it — verify with `rg -n "Request_CreateEntity" Source/CkEcs`):
  condition = the LIFETIME OWNER (a) has `FFragment_EntityScript_Current` and (b) does NOT yet have the
  construction-complete state (the same `HasBegunPlay`/FinishConstruction tags Phase 2's ConstructedThisFrame work
  used — `CkEntityScript_Fragment.h:23-27`). Owner not a script entity or already begun-play → no stamp
  (RuntimeSpawned by default).

### 3A.2 Spawn-recipe retention
New fragment in `CkEcs/EntityScript/CkEntityScript_Fragment.h` (runtime fragment, ck-namespace):
```cpp
// Retained construction recipe for save-file capture (spec §4.2 RuntimeSpawned). Stamped at spawn
// by the EntityScript spawn pipeline; params serialized with handle-remap at capture time.
struct CKECS_API FFragment_SpawnRecipe
{
    CK_GENERATED_BODY(FFragment_SpawnRecipe);
    TSubclassOf<UCk_EntityScript_UE> _ScriptClass;
    FInstancedStruct _SpawnParams;
    CK_PROPERTY_GET(_ScriptClass); CK_PROPERTY_GET(_SpawnParams);
    CK_DEFINE_CONSTRUCTORS(FFragment_SpawnRecipe, _ScriptClass, _SpawnParams);
};
```
Stamped where the spawn request is consumed and the script instance created (`FProcessor_EntityScript_
SpawnEntity_HandleRequests` — it has both class and params in hand). Always stamped (cheap; capture filters).

### 3A.3 v3 capture (new files in CkSnapshot: `Snapshot/CkSnapshot_CaptureV3.h/.cpp`)
Per spec §4.2 + §4.1. Entry: `ck::snapshot::Run_CaptureV3(UWorld&, FArchive&, FCk_Snapshot_HeaderV3&) -> result`.
Persist-set rule (capture filter, in this order — first match wins):
1. Entity carries any `FTag_DestroyEntity_*` or is the transient → skip.
2. `FFragment_SaveKey` present, or owning actor is a player pawn/controller/state
   (`UCk_Utils_OwningActor_UE` + engine `IsPlayerControlled`-family checks) → **EngineOwned**: write rendezvous key
   (SaveKey GUID | player identity via PlayerState unique-id string, empty=standalone-player-0) + payloads.
3. `FTag_ConstructSpawned` present → **ConstructSpawned** IF it has a unique GameplayLabel under its owner
   (query the owner's record via the label utils); write (owner saved-id, label) + payloads. Unlabeled → SKIP
   entity entirely (save-transient) BUT if any handler `Produce` returns data for it → save-time AUDIT WARNING
   naming owner entity, script class, label-less child's fragment set (spec §4.2 last block).
4. `FFragment_SpawnRecipe` present → **RuntimeSpawned**: write recipe (class path; params via a
   `FSnapshotContext`-remapped tagged-property serialize — reuse `FSnapshotArchive_Writer`; loud
   `CK_ENSURE_IF_NOT` on non-asset object refs mirroring `CkEntityReplicationDriver_Utils.cpp:300-308`),
   lifetime-owner saved-id, context-owner saved-id, `FFragment_ActorSpawnIntent` if present, + payloads.
5. Otherwise → skip (anonymous scratch; count them, report in the save log line).
Payloads: for every handler with `Produce` AND `Transport & Save`: (type path, tagged-property bytes).
**Ordering:** write entities in lifetime-topology order (owners before dependents — walk
`FFragment_LifetimeDependents` from roots). Forward handle refs inside SPAWN PARAMS are unsupported v3 —
`CK_ENSURE` loudly at capture if a params handle references a not-yet-written entity (fence: do NOT build a
park/fixup system; hydration payloads may reference anything, they apply after the full map exists).
Header v3: new `FCk_Snapshot_HeaderV3` beside the old (FormatVersion = 3), sections: entity table / payload table /
(oracle-only deep captures under `CK_WITH_FIDELITY_ORACLE`).

### 3A.4 Flip `Transport` to `NetAndSave` for the 12 migrated features
One-line change per Phase-1 registrar. (SM excluded until 4A.)

### 3A.5 Request_Save v3 switch
`UCk_Snapshot_Subsystem_UE::Request_Save`: after the existing quiescence pump (unchanged, Full scope), write BOTH
images for now: Model A bytes (existing) AND v3 bytes into the same `UCk_Snapshot_SaveGame` (add a `_SnapshotBytesV3`
field). Loading still consumes Model A until Phase 3B lands. This keeps every existing test green while v3 becomes
inspectable.

### 3A.6 Tests (CkTests) + gate
New: `"Ck.Snapshot.V3.CaptureClassification"` — fixture world with: one SaveKey entity, one script entity spawning
a labeled child + an unlabeled child in Construct, one runtime-spawned script entity; run `Run_CaptureV3`; assert
the entity-table classifications (EngineOwned/ConstructSpawned/RuntimeSpawned counts + the unlabeled child absent +
audit warning emitted when given a payload). `"Ck.Snapshot.V3.RecipeParamsHandleRemap"` — spawn params containing a
`FCk_Handle` to a sibling; capture; assert the serialized param bytes reference the sibling's saved-id (round-trip
through a reader stub).
```powershell
CkAuto\UnrealToolbox.exe --build --test --test-pattern "Ck.Snapshot" --discover-fresh --output CkAuto\logs\p3a.log
CkAuto\UnrealToolbox.exe --test --test-pattern "Net" --output CkAuto\logs\p3a-net.log
```
Expected: delta-zero + 2 new green. Commits: `feat(CkEcs): ConstructSpawned provenance + SpawnRecipe retention`;
`feat(CkSnapshot): v3 recipe+payload capture alongside Model A`; (CkTests) `test(CkSnapshot): v3 classification + param remap`.

## Exit criteria
- Both new tests green by name; delta-zero everywhere; a manual `Request_Save` in the test fixture produces a
  SaveGame whose v3 section is non-empty (asserted inside CaptureClassification).

## Fences
- Do NOT touch Request_Load / the load state machine (3B).
- Do NOT capture tags into v3 (tags are derived state under Model B; authoritative tag-like state = payloads).
- Do NOT make `FFragment_SpawnRecipe` a USTRUCT/UPROPERTY carrier — runtime fragment, TSubclassOf+FInstancedStruct
  members are GC-visible ONLY via the script instance's own strong ref; verify: params FInstancedStruct may hold
  object refs — if GC-tracing is needed, mirror how `_ReplicationData_EntityScript` stores the same data
  (`CkEntityReplicationDriver_Fragment_Data.h:106` — UPROPERTY on the driver UObject). If unclear at
  implementation time: STOP → Blockers (GC-lifetime is not improvisable).
