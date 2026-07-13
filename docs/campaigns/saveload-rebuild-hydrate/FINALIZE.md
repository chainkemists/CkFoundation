# FINALIZE.md — CkSnapshot unification close-out (locked design + phased plan)

> Authored 2026-07-12 (Opus, unattended close-out session). This is the LOCKED design for finishing the
> save/load campaign: fix the [INV-A] reds, delete Model A entirely, remove the footguns, unify on ONE
> persistence pipeline, and clean the branch. It supersedes PHASE_5.md's "gate-don't-delete" doctrine per
> Adam's directive (REMOVE the legacy; one pipeline). Every load-bearing claim is marked **[C]** (Opus
> code-verified, file:line) or **[A]** (audit-asserted; verify at implementation before acting).
>
> Design inputs: a 5-way read-only audit (4 Fable + 1 Opus-Plan for [INV-A] after the Fable usage cap).
> Raw audit reports live in the session scratchpad (`audit_{B,C,D,E}_*.md`, `AuditA` agent). This file is
> the reconciled, code-verified synthesis — trust THIS over the raw audits on any conflict.

## North star

Today there are TWO snapshot pipelines. The goal is ONE.
- **(a) v3 rebuild+hydrate — KEEP.** Save = `Run_CaptureV3` → `Get_SaveHandlerTypes` → per-feature `Produce`,
  serialized via `FMemoryWriter` + `FObjectAndNameAsStringProxyArchive` + the shared `ck::snapshot::RemapHandles`
  walker. Load = reload the level normally → rebuild (recipe/identity tables) → `FProcessor_Hydration_Dispatch`
  drains `FFragment_PendingHydration` via the SAME `Apply` handlers as net receive, under a load gate. **[C]**
- **(b) Model A — DELETE.** `CK_REGISTER_SNAPSHOTABLE` + 74 per-fragment `SerializeSnapshot` + `FragmentRegistry`
  + `Archive_Writer/Reader` + `TagDriver`/`TagRegistry` + `Run_Capture`/`Run_Restore` + the fidelity oracle
  (`FidelityOracle`/`Audit`/`Policy`). Gated `#if CK_WITH_FIDELITY_ORACLE` (= `!UE_BUILD_SHIPPING`); test-only. **[C]**

**Key reframe (governs the whole close-out):** Model A is ALREADY dead in the live/shipping path — the live
save/load has been v3-only since Phase 3B; Model A only runs in oracle TESTS. Deleting it therefore removes
no *live* capability — it removes the regression-net that *detected* v3 coverage gaps. So the fidelity gaps
(G1–G16 below) are PRE-EXISTING v3 limitations, not regressions the deletion introduces. **[C]**

## Current state (verified at session start)

- CkFoundation HEAD `a15880b26` (feature/save-load-improvements), tree clean, nothing pushed. CkTests `a0c2f2d`.
- Steady gate: `Ck.Snapshot` 54/51/3 — the 3 reds are the [INV-A] trio (`Parity.GridPlacements_MPReload`,
  `Parity.InventoryDataOnly_MPReload`, `Parity.InventorySpatial_MPReload`). `Ck.*.Net` framework delta-zero
  (documented `StateMachine.Net.OwningClientAuth_SubSm` flake + kiosk env-trio). Shipping Game compile OK.
- The real replicated-fragment contract is **Produce / Apply (+ optional Remove)**. `SeedContainer` is DEAD —
  zero invocation sites (only assignments + the `.inl.h` default-synth + a helper def); its sole ex-caller
  `FProcessor_Persistence_ReDriveOnRestore` was deleted in Phase-5 cluster 3. **[C]** The prompt's
  "Produce/Apply/SeedContainer" phrasing is stale; unification = *deleting* SeedContainer.

---

## Objective 1 — fix the [INV-A] trio (Grid + Inventory×2)

**Root cause [C]** (`CkSnapshot_CaptureV3.cpp:225-278`): v3 classifies every candidate entity into a provenance.
Rule 5 (`:267-271`) is the else-branch — an entity with NO `FFragment_SaveKey`, NO player rendezvous, NO
`FTag_ConstructSpawned`, NO `FFragment_SpawnRecipe` is `AnonymousSkipped`: it never enters the entity table, so
its Produce is never called and it never enters `_SavedIdMap`. Inventory items are built via
`Request_BuildAndReplicate` with a `UCk_InventoryItem_Definition` archetype (not the EntityScript spawn path) →
no `FFragment_SpawnRecipe` → Rule 5 → skipped. Grid occupants are the same anonymous class. The inventory/grid
payloads carry only item HANDLES → remap to nothing on load → Apply fails the valid-occupant gate → NotReady →
5s drop → red. Because an anonymous item is skipped *before* Produce runs, **payload-enrichment alone is
insufficient — the item must first be promoted to a captured+rebuilt provenance.**

**LOCKED design (Audit A, Opus-verified): Fork (a) sub-approach A1 — promote items to a first-class captured
provenance — PLUS a second, load-bearing correction Audit A discovered.** Two coupled defects, both fixed together:

**Defect 1 — items are Rule-5 anonymous [C].** `UCk_Utils_Item_UE::Create` (`CkItem_Utils.cpp:50-57`) builds a
`FCk_EntityReplicationDriver_ConstructionInfo(Definition->GetClass())` + `Set_ConstructionScriptArchetype(Definition)`
→ `Request_BuildAndReplicate`, which runs plain `Request_CreateEntity` + `Construct(NewEntity)`
(`CkEntityReplicationDriver_Utils.cpp:199-215`) — never the EntityScript spawn path → no `FFragment_SpawnRecipe`; and
items are added post-BeginPlay so no `FTag_ConstructSpawned` → Rule 5 → skipped. **Fix (mirror `FFragment_SpawnRecipe`
exactly):**
- Add `ck::FFragment_BuildRecipe` + a GC-safe holder `UCk_BuildRecipe_UE` (structural twin of
  `FFragment_SpawnRecipe`/`UCk_EntityScript_SpawnRecipe_UE`, `CkEntityScript_SpawnRecipe.h:27-78`), pinning the
  `TArray<FCk_EntityReplicationDriver_ConstructionInfo>` (each carries `_ConstructionScript : TSubclassOf<...>` +
  `_ConstructionScriptArchetype : TObjectPtr<const ...>`). Stamp it in `Request_TryBuildAndReplicate` right after the
  entity is built, **regardless of replication** (so non-replicated definition-built entities are captured too — don't
  key on the rep driver's internals).
- Add `ECk_Snapshot_V3_Provenance::DefinitionBuilt` (`CkSnapshot_Header.h` enum). Insert a classification rule in
  `CaptureV3.cpp` **between Rule 4 and Rule 5** (after the `FFragment_SpawnRecipe` check `:262`, before the anonymous
  else `:267`): `else if (Handle.Has<ck::FFragment_BuildRecipe>()) { Provenance = DefinitionBuilt; bPersist = true; }`
  (safe: bridged actors/ConstructSpawned are caught earlier). Pass-2 `case DefinitionBuilt:` writes the recipe fields
  (ConstructionScript class path + archetype `FSoftObjectPath`) + the already-captured `_ContextOwnerSavedId`.
- Add a `case DefinitionBuilt:` to `DoRebuild_Tick` (`CkSnapshot_Subsystem.cpp:572-703`), shaped like the pure-ECS
  RuntimeSpawned branch (`:654-701`): resolve the driver-bearing owner from `_ContextOwnerSavedId` (fallback
  `_LifetimeOwnerSavedId`); defer if unmapped; reconstruct the `ConstructionInfo` from the captured class/archetype
  paths; `Request_BuildAndReplicate(Owner, ConstructionInfo)` (once, guarded by `_SpawnedRuntimeIds`);
  `Request_TransferLifetimeOwner(item, mappedInventory)` if the captured lifetime owner differs; `_SavedIdMap.Add(...)`.
- **The ITEM's own fragment state rides its own Produce/Apply for free:** because the item is now a persisted entity,
  Pass-2's existing "run every Save-flagged Produce" loop emits the item's CkTagSet and spatial-placement payloads keyed
  to its saved-id, restored by each feature's OWN Apply. This is why A1 beats A2 (no bespoke inventory-payload item state)
  and beats fork (b) (no item net-model change).
- **⚠️ CORRECTION (adversarial-verified) — item-owned CHILD attribute state (stack count) is NOT captured by A1 as
  scoped [C].** Stack count is not a fragment on the item; it is a SEPARATE child attribute entity created by
  `CkItemTrait_Stackable.cpp:55` (`IntegerAttribute::Add(item, params)`). The item is not a script entity, so that
  attribute child gets no `FTag_ConstructSpawned` (`CkEntityLifetime_Utils.cpp:472-474`), no `SpawnRecipe`, no
  `BuildRecipe` → Rule 5 anonymous → skipped → its runtime value is NOT captured; on load the item's definition Construct
  re-creates it at the definition default (`_InitialCount`). A 5/10 potion stack restores as 10/10. **The two F1 gates
  still pass HONESTLY** — their fixtures use non-stackable items (`ItemDef_InvGym_Shield` Tags-only, `ItemDef_InvGym_Sword`
  Dimensions-only) and assert only count/coordinate/rotation, never stack count — so this is NOT a fixture false-green, but
  it IS a real fidelity gap the design must not claim is closed. See **G17** (obj 4). It is the same class as [INV-A] one
  level deeper (the item's attribute children are themselves anonymous). Robust closure (recommended, for Adam to scope):
  extend the capturable-provenance classification to definition-built-item CHILDREN (a labeled attribute child under a
  DefinitionBuilt owner → treat as ConstructSpawned/adopt-by-label) so item attributes round-trip via the EXISTING
  attribute Produce/Apply — OR enrich the DefinitionBuilt entry with runtime attribute values. **Not required for the two
  F1 gates; do NOT silently ship the "no new capture code" narrative.** Pre-existing (v3 already doesn't persist it live).

**Defect 2 — the inventory/grid `Apply` has no authority-side load re-drive [C].** The v3 Produce is CHILD-keyed
(inventory/grid entity) but the record/container lives on the OWNER (`CreateInventory` puts `RecordOfInventories` on
the owner; CkInventory doctrine confirms containers ride the outer entity). So on load `Apply(inventoryEntity, …)`
reads an empty `RecordOfInventories` on that entity → `NotReady` → 5s drop; and it only stamps a **`ClientOnly`**
SyncReplication fragment, so the server never rebuilds its `RecordOfInventoryItems` → `Get_NumItems == 0` on the server
regardless of the client. **Fix (mirror the PROVEN CkAttribute pattern `CkIntegerAttribute_Fragment.cpp:111-168`):** add
an authority-side branch to the DataOnly / Spatial / 2dGridOccupancy `Apply` lambdas, gated on
`FCk_HydrationApplyScope::Get_IsActive()` (`CkReplicatedFragmentContainer.h:47-66` — the RAII load-path hook, already
used by the SM redrive): connect each `New` entry's item handle into THIS entity's `RecordOfInventoryItems` (+ Spatial:
re-stamp placement from `_Coordinate`/`_Rotation`; grid: `Request_AddPlacement` per placement); return `Applied`, or
`NotReady` (all-or-nothing) if any referenced handle is invalid. Net (owner-keyed) path unchanged; clients converge via
the existing `ClientOnly` SyncReplication.

**Both defects must land together:** Defect-1-only leaves the re-created item unconnected to `RecordOfInventoryItems`
(the connect happens in `AddByDefinition`, not `Request_BuildAndReplicate`); Defect-2-only leaves the payload's item
handles resolving to `entt::null`.

**Ordering guarantee [C]:** provided by the existing phase machine — `Rebuilding` maps every persisted entity
(now incl. `DefinitionBuilt` items) before `Hydrating` runs `DoHydrate_Enqueue`; the lifetime-depth topology sort
(`CaptureV3.cpp:281-284`) + owner-mapped gate order owner→item→payload; belt-and-suspenders, the hydration branch
returns `NotReady` (retry→loud 5s drop) if any handle is unresolvable.

**Convention fit:** `FFragment_BuildRecipe : Request_BuildAndReplicate :: FFragment_SpawnRecipe : EntityScript spawn`;
`DefinitionBuilt` is the 4th sibling of the 3-provenance design (no new load-SM phase); the Apply branch is a verbatim
mirror of the shipped attribute handler using the framework's purpose-built `FCk_HydrationApplyScope`. Pure CkFoundation
(`CkEcs` + `CkSnapshot` + `CkInventory` + `CkGrid`); **no BB edit**; fixtures create items/occupants through the real
production paths (verified — no fixture change needed; item defs are on-disk assets → stable soft-paths).

**Files:** `CkEcs/.../CkEntityReplicationDriver_Utils.cpp` (stamp) + new `FFragment_BuildRecipe` header;
`CkSnapshot_CaptureV3.cpp` (classify + capture); `CkSnapshot_Subsystem.cpp` (rebuild branch); `CkSnapshot_Header.h`
(enum + entry field); `CkInventory_DataOnly_Fragment.cpp` + `CkInventory_Spatial_Fragment.cpp` + `Ck2dGridOccupancy_Fragment.cpp`
(Apply hydration branch, mirror `CkIntegerAttribute_Fragment.cpp:111-168`).

### GridPlacements escalation — obj-1 delivers 2/3, not 3/3 [C — STOP-and-flag for Adam]
`Parity.GridPlacements_MPReload` has an **independent engine-death** unrelated to [INV-A]: per its own spec header
(`Test_Snapshot_GridPlacementsParity_MPReload_Gate.spec.cpp:13-19`), when a `2dGridSystem` rides the top-level
actor-bridged **REPLICATED** entity through save + seamless travel, the editor self-terminates ~4s into the post-travel
ECS boot — and it "dies with the grid alone (no occupant, no placement) and dies identically on builds WITHOUT the grid
snapshot wiring." A nested-`FEcsWorld` (`TUniquePtr`) lifetime bug on a replicated bridged entity through seamless
travel — orthogonal to provenance work, unfixable by this design; its own workstream. `InventorySpatial` dodges it
because its grids ride UNREPLICATED CHILD entities. The grid occupant is itself unlabeled `ConstructSpawned` (Rule 3),
re-created by Construct — it does NOT need Defect 1; it needs Defect 2's grid re-drive (which we still implement, and
`InventorySpatial` exercises). **Outcome: `Parity.InventoryDataOnly_MPReload` + `Parity.InventorySpatial_MPReload` →
GREEN; `Parity.GridPlacements_MPReload` → STILL RED (engine-death, flagged to Adam; covered at registry level by
`Ck.Snapshot.GridPlacements.RoundTrip` until the death is fixed).**

### Other Audit-A escalations (record; Adam)
- **Transient item definitions can't round-trip [C]** (`GetOrCreate_TransientItemDefinition` mints `RF_Transient`
  defs, `CkItem_Utils.cpp:89`) — the archetype is captured by soft-object path, so runtime-built defs have no stable
  path. The three gates use on-disk assets → pass. Document the constraint ("saved inventories must use on-disk item
  definitions") or later serialize CoreInfo+traits into the entry (larger payload).
- **v3 save-format bump** — a new provenance value + entry fields change the v3 stream shape; pre-existing v3 saves
  become incompatible. Acceptable (v3 not shipped), but an explicit on-disk decision worth Adam's sign-off. (Far less
  irreversible than fork (b)'s item net-model change — the reason A1 is preferred.)

### F1 implementation & verification risks (adversarial-flagged — validate EMPIRICALLY, not by prediction)
- **The green is a long multi-tick rendezvous cascade** (bridged actor respawn → label-rendezvous inventory →
  `DefinitionBuilt` item rebuild under context owner → `Request_TransferLifetimeOwner` → hydration connect + placement
  re-stamp). Each step is grounded, but the composite is ONLY provable by building + running `Ck.Snapshot` on the real
  listen-server seamless-travel PIE. A code-read "green" is a prediction — the F1 gate must be a real run.
- **`Request_BuildAndReplicate` under the load gate is untested** — existing rebuild branches use `Request_SpawnEntity`/
  `SpawnActor`; the `DefinitionBuilt` branch drives the full build+replicate path (rep-driver setup +
  `OnReplicationComplete`/dependents broadcasts, `CkEntityReplicationDriver_Utils.cpp:231-252`) while
  `Set_IsLoadGateActive(true)`. Validate that firing replication-complete signals under the gate during `Rebuilding`
  doesn't trip half-built-world assumptions. Budget for a debug loop, not a mechanical port.
- **Single stamp choke point CONFIRMED**: all build overloads funnel into the 2-arg `Request_TryBuildAndReplicate`
  (`CkEntityReplicationDriver_Utils.cpp:176-263`); stamp `FFragment_BuildRecipe` after `Construct` (`:215`), before the
  `DoesNotReplicate` early-return (`:218`), so replicated AND non-replicated definition-built entities are captured.
- **F3 edge (Concepts.h):** ~10 fragment headers include `CkSnapshot_Concepts.h` solely for the `SerializeSnapshot`
  fwd-decl of `ck::FSnapshotContext`; the per-fragment sweep must drop the include AND the decl in lockstep per file, or
  a dangling reference survives the Shipping compile.

### Fable design-pass ruling (2026-07-13, unattended close-out; read-only agent, code-verified before recording)
Fable validated the locked [INV-A] design against the WIP (ce454b823): **verdict SOUND** — stamp/classify/rebuild/Apply
are a faithful structural mirror of SpawnRecipe/RuntimeSpawned (stamp after Construct before the DoesNotReplicate return;
Rule 4.5 between Rule 4 and the anonymous else; rebuild defers-until-owner-mapped with a once-guard + synchronous
`Request_BuildAndReplicate_Multiple`; Apply gated on `FCk_HydrationApplyScope::Get_IsActive()`, all-or-nothing NotReady).
It surfaced **4 real flaws** (Opus to fold F-1/F-2/F-3 into the F1 fix; F-4 is a contract note):
- **[F1-FABLE-F1] silent owner-unpersisted skip** — `Subsystem.cpp:719-723`: a DefinitionBuilt entry whose lifetime
  owner is unpersisted is added to `_SkippedIds` with NO log (the sibling failure paths log Errors at :715/:752). A
  definition-built ITEM under an unpersisted owner is data loss, not boot-infra → must log (≥Warning) loudly.
- **[F1-FABLE-F2] triage blind spot** — the stall-dump provenance switch `Subsystem.cpp:992-997` (and the sibling at
  ~1006-1011) has no `DefinitionBuilt` case → a stuck item is invisible in the stall Error. Add the case.
- **[F1-FABLE-F3] orphan-on-failed-build** — `_SpawnedRuntimeIds.Add(SavedId)` at :757 runs BEFORE the build at :762;
  if `Request_TryBuildAndReplicate` returns invalid (host gate / rep-driver ensure), the entry is never mapped, never
  skipped, never retried (once-guard) → permanent log-less orphan. Re-order: add to the once-guard only on a valid build,
  else leave unresolved (retry) or skip loudly.
- **[F1-FABLE-F4] transfer moved to Apply (contract deviation, functionally covered)** — FINALIZE §obj-1 required the
  REBUILD branch to `Request_TransferLifetimeOwner(item, mappedInventory)`; the impl moved it into the Apply hydration
  branch (`CkInventory_DataOnly_Fragment.cpp:46`). Equivalent IF Apply reaches `Applied`; until then the rebuilt item's
  lifetime owner is the subject (a save in that window captures the wrong owner). Document, or restore the rebuild-side transfer.

Root-cause ranking (Fable, code-verified): **H1 capture-side miss** (item not among the captured entries; mechanism a
runtime falsification — most plausibly the silent world-guard skipping the stamp at `CkEntityReplicationDriver_Utils.cpp:222`,
or an earlier classify rule) > **H2 rebuild silent skip** (:719, owner ∉ `_PersistedIds` — contradicted by the immediate
`Request_TransferLifetimeOwner` `Replace` at `CkEntityLifetime_Utils.cpp:572` + the inventory being Rule-3 persisted) >
**H3 invalid build handle** (orphan via F-3 — contradicted by build4's zero-orphan arithmetic). Opus's F1-DIAG instrumentation
is a superset of Fable's decisive probe (adds the stamp-lands check + classify trace + rebuild-gate trace), so one run disambiguates.

### Fable 2nd ruling + instrumented-run root causes (2026-07-13, code-verified) — the ACTUAL [INV-A] fix
The instrumented run (F1-diag/F1-fix1) + a 2nd Fable read-only pass pinned BOTH reds to concrete, in-repo causes:
- **DataOnly root cause = an UNREGISTERED FIXTURE TAG.** The fixture requests `RequestGameplayTag("Inventory.AutoTest_Net")`
  which is **not registered** (grep of all sources: only `TAG_Inventory_AutoTest_Net_Spatial` is `UE_DEFINE_GAMEPLAY_TAG`d,
  `CkTests_Fragment_Data.cpp:48`). Unregistered ⇒ empty tag ⇒ `CreateInventory` labels the DataOnly inventory with `None`
  ⇒ `Get_IsUnnamedLabel`=true ⇒ Rule 3 refuses to persist it ⇒ (a) the item's lifetime owner is unpersisted AND (b) the
  inventory's Produce payload is **dropped** (`CaptureV3` runs Produce only for persisted entities; the exact
  "unlabeled ConstructSpawned child … payload DROPPED" audit fires, suppressed by the spec's `bSuppressLogWarnings`). So
  **ctx-owner-first rebuild ALONE is insufficient** — the instrumented run confirmed it DOES rebuild the item under the
  subject (`BUILT resolved=[Shield]`), but with no inventory payload NOTHING connects it → NumItems=0. **The essential fix
  is registering the tag** (CkTests fixture). The `.h` even documents this exact lesson for the Spatial tag — it just was
  not applied to the DataOnly one. **FINALIZE's "no fixture change needed" (obj-1) is empirically FALSIFIED.**
- **Spatial client root cause (Fable H1, ~90%, verified) = the hydration Apply never RE-ARMS replication.** The
  authority push processor `FProcessor_Inventory_Spatial_Replicate` is `MarkedDirtyBy = FTag_Inventory_MayRequireReplication`,
  armed only via FireSignals(record-delta)/Relocate/`Request_TryReplicateInventory`. The load-path Apply calls
  Connect+Transfer+PlaceItemOnGrid (none arm the tag) → the Replicate processor never runs → the respawned owner's
  replicated container keeps its EMPTY Construct-time value → the client receives an empty item list → count 0 →
  Stage-6 timeout (server-local assertion passes). **Fix: call `UCk_Utils_Inventory_UE::Request_TryReplicateInventory`
  at the end of BOTH hydration Apply branches** (DataOnly + Spatial). This also fixes the DataOnly client once the tag lands.
  NOT the GridPlacements engine-death class (that is a replicated-bridged-grid + seamless-travel editor death; these grids
  ride unreplicated child entities and the post-travel server is alive).
- **Fable H2 (latent, recorded, NOT fixed):** the NET (non-load) Apply branch has no item-validity gate — if a container
  entry arrives with an unresolved item handle (`FCk_Handle::NetSerialize` drops unresolved, no UE unmapped-object rescue),
  it is silently lost with no second push. The live path self-heals via a later push; the single post-load re-arm push
  MAY race the item's replication. If Spatial greens flakily, harden by mirroring the load branch's NotReady gate in the
  net branch and/or re-arming once at OnLoadComplete. Deferred unless it manifests.

**[F1-D6] Fix set (executor decision, Fable-endorsed, code-verified):** (1) CkTests: register + use
`TAG_Inventory_AutoTest_Net` (fixture bug — the essential DataOnly fix). (2) CkFoundation: `Request_TryReplicateInventory`
re-arm in both hydration Apply branches (Spatial + DataOnly client convergence). (3) Keep ctx-owner-first rebuild
(design-compliant with obj-1; harmless with the tag fix — both forms compute the same build owner for persisted
inventories) + F-1/F-2/F-3. CkTests must be committed WITH/AFTER CkFoundation, never leading.

**Phase-F1 gate (revised):** `Ck.Snapshot` = 52/2 (Inventory×2 flip green), **1 remaining red = `GridPlacements`
(engine-death, Adam-flagged)**, `Ck.*.Net` delta-zero. Do obj-1 FIRST — while the oracle still cross-checks the new
item Produce — THEN purge (obj 2). Per Audit C, `GridPlacements.RoundTrip` (registry-level) is deleted only after the
engine death is resolved and its parity twin greens, so it is retained through the purge as the grid coverage.

---

## Objective 2 — remove Model A entirely (no gated dead code)

Complete delete/keep map (Audit B, cross-checked with D; my spot-verifications noted).

### DELETE whole file
`CkEcs/Snapshot/`: `CkSnapshot_FragmentRegistry.{h,cpp}` (hosts the `CK_WITH_FIDELITY_ORACLE` define at
`FragmentRegistry.h:17` **[C]** + the macro), `CkSnapshot_Archive_Writer.{h,cpp}`, `CkSnapshot_Archive_Reader.{h,cpp}`,
`CkSnapshot_TagDriver.{h,cpp}`, `CkSnapshot_TagRegistry.{h,cpp}`, `CkSnapshot_FidelityOracle.{h,cpp}`,
`CkSnapshot_Audit.{h,cpp}`, `CkSnapshot_Policy.h`, `CkSnapshot_CkEcsFragments_Registration.cpp`,
`CkEcs/Concepts/CkSnapshot_Concepts.h`.
`CkSnapshot/Snapshot/`: `CkSnapshot_Capture.{h,cpp}` (Model A, NOT CaptureV3), `CkSnapshot_Restore.{h,cpp}`.

### KEEP
- `CkSnapshot_Context.{h,cpp}` — **SHARED**; PRUNE the Model-A-only bits: `_Saver`/`_Loader` members, the entt
  saver/loader ctors, the `_Loader` branch in `Snapshot_EnttEntity`, `IsSaving()` (verify no callers), the
  `snapshot.hpp` include. Keep `SnapshotRegistryType`, `Snapshot_Handle`, re-homing, `_SavedIdMap`,
  `_LoadRegistryHandle`, the map-backed ctor. **[A]**
- `CkSnapshot_HandleWalk.{h,cpp}` — untouched; fully shared (v3 save `CaptureV3.cpp:111`, load `Subsystem.cpp:541`). **[C-ish]**
- `CkSnapshot_RestoreMarker.h` + `FTag_Snapshot_JustRestored` — KEEP per [P5-D3]; BB consumers
  (`Bb_SnapshotRestore.cpp`, `Bb_CombatReceiverRestore.cpp`) still `Has<>` it. Removal needs BB-coordinated
  cleanup → **Adam-gated / BB-repo ticket** (do NOT edit BB). Its comment must be reworded campaign-free (obj 5).
- `CkSnapshot_RestoreInvariants.{h,cpp}` — feature-agnostic dangling-handle sweep; consumed by the surviving v3
  gate test (`AttributeParity_MPReload_Gate.spec.cpp:299`); all three walked fragments live under v3.
- `CkSnapshot_LoadReport.{h,cpp}` — KEEP; optional prune of Model-A-only fields (`_Skipped*`, `_LoadedHeader`) →
  Adam (BlueprintType surface). Add a `_PayloadsDropped` counter for F1.
- `CkSnapshot_Header.{h,cpp}` — KEEP file + the frozen `FCk_Snapshot_Header` TYPE (frozen `Get_SaveSlotHeader`
  return + `LoadReport._LoadedHeader`). Model-A-only members (`_Manifest`/`_TransientEntityId`/`_TagSectionByteOffset`
  + `FragmentManifestEntry`) → vestigial; prune is an Adam call (BlueprintType).
- `CkSnapshot_SaveGame.{h,cpp}` — already v3-only. `CkSnapshot_CaptureV3.*`, `Subsystem/*` — the live path.
- `CkSnapshot_SaveKey_Fragment.*` — KEEP the fragment (v3 rendezvous); its `.cpp` shrinks to (near) empty.

### Per-fragment sweep (~45 registration TUs, ~56–74 SerializeSnapshot sites, 126 CK_REGISTER_SNAPSHOTABLE lines) **[A]**
Per file: delete the `SerializeSnapshot` decl+body, the `using IsSnapshotable = void;`, the `CK_REGISTER_SNAPSHOTABLE`
line(s), the `FSnap_*` alias hoists, and the 3 snapshot includes (`FragmentRegistry.h`, `Archive_Writer.h`,
`Archive_Reader.h`). Specials: `FFragment_EntityScript_Current::_SnapshotLoadPin` (`CkEntityScript_Fragment.h:82`,
Model-A-only — delete member + its fill path); `FCk_Chrono::SerializeSnapshot` (CkCore — Timer's only consumer,
deletes with Timer's method — see obj-1/Timer note); `CkDynamic` `SerializeSnapshot` (thin `RemapHandles` call —
delete method, KEEP the shared walker); templated `TFragment_EntityHolder`/`TFragment_RecordOfEntities`/attribute
families (delete with the Policy fork). The 20 modules: CkAnimation, CkAttribute, CkDynamic, CkEcs, CkEcsExt,
CkEntityCollection, CkEntityTag, CkGrid, CkInteraction, CkInventory, CkLabel, CkObjective, CkPhysics,
CkRelationship, CkRenderTarget, CkSnapshot, CkSpatialQuery, CkStateMachine, CkTagSet, CkTimer.

### The TagRegistry landmine (§ obj-2, one atomic edit) **[C]**
`CkTag.h:23-26`: `CK_DEFINE_ECS_TAG` emits `_TagAutoReg_##name = Register_SnapshotableTag<T>()` for **every** tag
(229 sites, all configs). `CK_DEFINE_ECS_TAG_TRANSIENT` (`:33-35`) is *already* the registrar-free form. Removal:
(1) rewrite `CK_DEFINE_ECS_TAG` to the `_TRANSIENT` body (struct + `static_assert`), drop the
`#include CkSnapshot_TagRegistry.h` (`:5`) + the opt-OUT commentary; (2) same commit delete TagRegistry + TagDriver;
(3) collapse `CK_DEFINE_ECS_TAG_TRANSIENT`'s 13 sites to `CK_DEFINE_ECS_TAG` and delete `_TRANSIENT` (recommended)
OR keep it as a pure alias (BB uses it — alias avoids a BB-touching sweep). Purely subtractive — `Register_SnapshotableTag`
is `entt::type_hash` + a captureless lambda, no UObject, no static-init ordering effect (the header comment says so). **[C]**

### Retire CK_WITH_FIDELITY_ORACLE
Define at `FragmentRegistry.h:17` only; referenced by 8 CkFoundation files (all delete-whole) + 2 CkTests oracle
tests (delete). Dies with zero residue. **[C]** Deletion MUST be atomic + grep-gated: post-delete
`rg --no-ignore CK_WITH_FIDELITY_ORACLE` ⇒ 0 hits repo-wide (any surviving `#if` silently evaluates 0 in ALL configs —
the [P5-D4] landmine, now repo-wide).

### The fidelity oracle dies WHOLE (settles my earlier "keep Tier-1/2" hypothesis)
`FidelityOracle` Tier-1 (structural) / Tier-2 (Produce-diff) are architecturally Model-A-independent, BUT their only
consumers are the two gated `Oracle.*` tests, and those are themselves entangled: `Oracle.StructuralBaseline` runs a
Model-A round-trip (`Run_Capture_Registry`/`Run_Restore_Registry`) → DELETE; `Oracle.ProduceDiffBaseline` uses
`Capture_Payloads` (no round-trip) → REPLACE with a ~40-line oracle-free `Ck.Snapshot.V3.ProduceSensitivity`
(resolve the Velocity handler, `Produce` before/after a mutation, assert payload bytes differ / equal-when-unmutated). **[A]**
A generic v3-round-trip structural net (capture→save→load→capture→diff) is NOT built today; building it is optional
future work (the "coverage cliff" mitigation), not a blocker. Recorded as a follow-up.

### Policy-macro fork (Adam call; recommendation = Option A) **[A]**
The holder/record base templates take a required `T_Policy` whose only effects are (i) the `IsSnapshotable` marker
(Model-A capture eligibility) and (ii) `CK_SNAPSHOT_ANNOUNCE_EXPECTED` (Model-A registration audit). Both die with
Model A. **Option A (recommended, "no dead code"):** drop `T_Policy`, resurrect plain
`CK_DEFINE_ENTITY_HOLDER`/`CK_DEFINE_RECORD_OF_ENTITIES(_AND_UTILS)` (currently `static_assert(false)` tombstones),
rename ~85 call sites across ~44 files (C++-only, zero BB/AS), delete `Policy.h` + `Audit.*`. **Option B (interim):**
keep `_ROUNDTRIP`/`_TRANSIENT` as aliases of one policy-less macro (zero churn, misleading names linger). Audit D
Option (c): repurpose the forced choice as a v3-facing capture-time audit ("RoundTrip-classified holder with no
Save-transport Produce") — serves the fidelity-gap detection. **Decision: default to Option A; Adam may prefer (c)
for the audit value.**

### Commit-cluster order (every intermediate state compiles, Dev + Shipping)
The pervasive-include problem dissolves because the 45 fragment `.cpp`s that `#include` the archives/registry ARE the
files whose SerializeSnapshot surface we delete — so every include edge vanishes BEFORE the providers are deleted. **[A]**
1. **CkTests cluster 0 (port-before-delete):** add the `Ck.Snapshot.V3.HandleWalk.*` suite + `V3.ProduceSensitivity`
   + `Parity.AttributeModifier_MPReload` (migrated FloatAttribute.Gate) — all compile against CURRENT CkFoundation.
   Gate green (new + old). See obj-2 test-fate below.
2. **CkSnapshot module:** delete `CkSnapshot_Capture.*`, `CkSnapshot_Restore.*`; shrink `SaveKey_Fragment.cpp`.
3. **Per-fragment sweep** (may split per-module): the recipe across 45 TUs + declaring headers + specials.
4. **CkEcs teardown (atomic):** delete FragmentRegistry/Archive_*/TagDriver/TagRegistry (+ CkTag.h edit)/FidelityOracle/
   Audit (+ announce-macro removal)/Policy/CkEcsFragments_Registration (+ decl removals in CkHandle.h /
   CkEntityLifetime_Fragment.h)/Concepts; prune FSnapshotContext; delete dead `Get_ReDriveHandlerTypes` +
   `Get_ProduceHandlerTypes`. CK_WITH_FIDELITY_ORACLE ceases to exist. Gate: **Dev build + full Ck suite + Shipping
   Game compile proof** (this cluster is where a missed include/symbol shows in Shipping — Class-4 stale-binary trap).
5. **CkTests cluster (delete/migrate):** delete the 15 + strip Model-A legs from the 6 migrated. Gate against #4.
6. **Docs/comment sweep** (obj 5) + exit greps: `rg --no-ignore` for `CK_REGISTER_SNAPSHOTABLE|SerializeSnapshot|
   IsSnapshotable|FSnapshotPolicy|CK_WITH_FIDELITY_ORACLE|Run_Capture|Run_Restore|Capture_Tags|snapshot_audit` ⇒ 0
   (outside the `CkSnapshot_RestoreMarker.h` carve-out).
Cross-repo: neither repo pushes until the whole sequence is green; CkTests must not lead CkFoundation; superproject
pointer bumps move both submodules together.

### Fable F3 execution ruling (2026-07-13, code-verified — SUPERSEDES the cluster order below where they conflict)
A read-only Fable pass verified the purge against code and corrected the plan. **Load-bearing corrections:**
- **CkTests deletions are NOT a trailing cluster.** 19 CkTests files `#include CkSnapshot_Capture.h`/`_Restore.h`
  UNCONDITIONALLY (above any `#if`) → they must be co-deleted in the SAME commit pair as **cluster 2**; 2 more
  (Audit_UnregisteredRoundTrip, Oracle_ProduceDiffBaseline) co-delete with **cluster 4**. "CkTests must not lead" still
  holds — but it must also not LAG. The 19 (cluster 2): Core/SaveKey/EmptyTag/MultiTag_RoundTrip, LifecycleStrip,
  SceneNode/StateMachineState/InteractTarget_RoundTrip, RestoreRobustness, Oracle_StructuralBaseline, GridPlacements/
  TimerChrono/MontagePlayerState_RoundTrip, DynamicFragment_{TypedHandleRemap,HandleRemap,AsFragment,Coverage},
  PropertyFuzz_RoundTrip, FloatAttribute_Gate.spec.
- **[F3-D1 — ADAM] Adam-gated test deletions forced to pre-cluster-2.** TimerChrono/MontagePlayerState/GridPlacements_RoundTrip
  are in the coupled-19 → they CANNOT survive past cluster 2. FINALIZE's "delete GridPlacements.RoundTrip only after its
  parity twin greens" is UNACHIEVABLE (compile-coupled). Executor decision (mission pre-authorizes implement-and-record):
  DELETE them at cluster 2; **record the coverage loss** — grid save/load then has NO passing coverage (parity twin is the
  engine-death red, registry twin deleted); Timer-resume + MontagePlayer-state round-trip coverage also drop. Adam: accept,
  or land v3 Timer/MP payloads + resolve the grid engine-death first.
- **[F3-D2] Policy-macro fork = DEFER Option A.** T_Policy's only reads are the `TSnapshotMarker<T_Policy>` base (sole
  effect: the `IsSnapshotable` typedef) — Option A (drop T_Policy, 57 macro sites / 40 files + 4 raw-template lines +
  resurrect the 4 tombstones + delete Policy.h) is mechanically SAFE and BB/AS-free, but it is NOT on the purge's critical
  path. **Mandatory minimal cluster-4 edit:** drop the 2 `CkSnapshot_Audit.h` includes + the 2 `CK_SNAPSHOT_ANNOUNCE_EXPECTED`
  lines from `CkEntityHolder_Fragment.h`(:13,:76) + `CkRecord_Fragment.h`(:16,:117); KEEP `CkSnapshot_Policy.h` inert (it
  includes nothing). Option A runs as a standalone rename commit (this campaign or later). Under defer the `IsSnapshotable|
  FSnapshotPolicy` exit greps carry a Policy.h carve-out until Option A lands. Option (c) = new design work, follow-up only.
- **Scout corrections (verified):** 53 `CK_REGISTER_SNAPSHOTABLE` files (not 46) + 1 doc row; 74 `SerializeSnapshot`;
  10 `CK_WITH_FIDELITY_ORACLE` (8 CkF + 2 CkTests, all in delete-whole files, zero landmines); `CK_DEFINE_ECS_TAG_TRANSIENT`
  is **BB-FREE** (12 sites, safe to collapse into `CK_DEFINE_ECS_TAG`). `Get_ProduceHandlerTypes` needs an EXPLICIT 2-line
  deletion in the KEEP files (`CkReplicatedFragmentContainer.h`/`.cpp`) — sole caller is `CkSnapshot_FidelityOracle.cpp:234`.
- **Cluster 3 sweep scope:** feature modules only; the IsSnapshotable file census (54) ∪ SerializeSnapshot ∪ CK_REGISTER
  is the checklist (SerializeSnapshot alone misses Tier-A marker-only registrants). EXCLUDE the CkEcs-owned fragments
  (CkHandle.h, CkEntityLifetime_Fragment.h, CkEntityScript_Fragment.{h,cpp}+_SnapshotLoadPin, CkNet_Fragment.h) — their
  bodies are centrally registered in CkSnapshot_CkEcsFragments_Registration.cpp → they go ATOMICALLY with cluster 4.
  Sweep Timer + `FCk_Chrono::SerializeSnapshot` (CkCore, Timer's only caller `CkTimer_Fragment.cpp:46`) same commit.
  Keep `Test_Snapshot_DynamicFragment_Fixtures.h` (included by the surviving `Test_Snapshot_V3_Capture.cpp`).
- **Exit greps** word-bounded: `\bRun_Capture\(|Run_Capture_Registry` (survivor `Run_CaptureV3`), plus `TSnapshotMarker`,
  `CK_SNAPSHOT_ANNOUNCE_EXPECTED`, `FCk_Snapshot_TagRegistry|Register_SnapshotableTag`, `Get_ProduceHandlerTypes`,
  `_SnapshotLoadPin`, `FSnap_`. Carve-outs: `CkSnapshot_RestoreMarker.h`/`FTag_Snapshot_JustRestored` (BB consumers
  `Bb_SnapshotRestore.cpp`/`Bb_CombatReceiverRestore.cpp` — live, do NOT delete), `docs/campaigns/*`, Policy.h (until Option A).
- **Cluster 0 (port-before-delete)** builds vs CURRENT CkF: `Ck.Snapshot.V3.HandleWalk.*` (on the existing
  `Test_Snapshot_V3_Capture.cpp` harness) + `V3.ProduceSensitivity` + `Parity.AttributeModifier_MPReload` (replaces
  FloatAttribute.Gate — the only modifier-across-save/load coverage). Executor note: if context-limited, the purge may run
  ahead of cluster-0 authoring with the coverage gap RECORDED (v3 path is already covered by the surviving Parity/V3/
  LoadGate/Meta tests) — but prefer port-first.

### Model-A test fate (CkTests — Audit C) **[A]**
29 Model-A-coupled tests across 21 files. **The must-survive handle-remap coverage** ports to a new
`Ck.Snapshot.V3.HandleWalk.*` suite built on the ALREADY-EXISTING v3-native harness
(`Test_Snapshot_V3_Capture.cpp:212-242` `SerializeBlob_Save`/`DeserializeBlob_Mapped`), exercising the SAME
production `RemapHandles` via the same two context modes v3 uses — strictly stronger than what dies. Port: typed
single/array (the tombstone regression), TSet element+rehash, TMap key+rehash / typed values, bare single/array,
nested, empty/invalid, self-ref, AS-authored struct, fuzz graph.
- **DELETE (15 files):** Core_RoundTrip, SaveKey_RoundTrip, EmptyTag, MultiTag, LifecycleStrip, SceneNode,
  StateMachineState (redundant with SM parity), InteractTarget, Audit_UnregisteredRoundTrip, Oracle_StructuralBaseline,
  RestoreRobustness (×3), + 5 named-storage DynamicFragment_Coverage cases.
- **MIGRATE (6):** DynamicFragment_TypedHandleRemap (highest priority — the tombstone incident), _HandleRemap (walker
  legs), _AsFragment (AS-handle leg), PropertyFuzz (fuzz graph), 3 Coverage walker cases → HandleWalk suite;
  FloatAttribute.Gate → `Parity.AttributeModifier_MPReload` (v3 subsystem harness; base 42.5 + revocable +17.5 →
  Final==60 post-reload; this is the ONLY modifier-across-save/load coverage — migrate, don't delete).
- **REPLACE (1):** Oracle_ProduceDiffBaseline → `V3.ProduceSensitivity`.
- **Adam-gated deletes:** Timer.ChronoRoundTrip (timer resume dies with `FCk_Chrono::SerializeSnapshot` — accept
  timers-don't-resume OR land Timer v3 payload first), MontagePlayer.StateRoundTrip (pair with a new v3 MP gate),
  GridPlacements.RoundTrip (delete AFTER `Parity.GridPlacements_MPReload` green, i.e. after obj-1),
  DynamicFragment named-storage cases (Q1 — dynamic-fragment save has no v3 path; accept AS-SaveGame-fields-only OR
  add a v3 dynamic-fragment handler).
- **KEEP untouched:** the 11 `Parity.*_MPReload`, M2a/M2b/M2b2a/M2b2b, the 3 `V3.*`, `LoadGate.*`,
  `Meta.RepDataRestoreCoverage` (the v3 coverage ratchet, replaces the deleted Audit test's role), the 2 `AS.*`.
- **VALIDATION.md/PHASE_5.md must be amended** (they still name the two `Oracle.*` tests + 5 not-yet-written
  `Rebuild.*` tests as acceptance, and encode the superseded gate-not-delete doctrine).

---

## Objective 3 — footguns/tripwires (fix in-scope; record the rest)

**Fix-now (live v3 path):**
- **F1 [C]** — silent corrupt-payload drop (`Subsystem.cpp:736-738`): `DoDeserialize_V3Blob` fails → bare `continue`,
  no log/ensure/counter (the sibling owner-unmapped drop at `:733` is orphan-accounted; this one is invisible).
  Fix: `CK_ENSURE_IF_NOT(Data.IsValid(), ...)` naming `Get_TypePath()`+`Get_OwnerSavedId()`, recovery counts into a
  new `_PayloadsDropped` LoadReport field.
- **F2 [C]** — `DoIs_HydrationComplete` (`:759-784`) is dead code; `Settling` finishes on a frame countdown so
  `OnLoadComplete` can fire with payloads still pending. Fix: `Settling` waits on `DoIs_HydrationComplete()` with the
  frame-cap as a loud abort (`CK_TRIGGER_ENSURE` + report field). **Behavior change to completion timing → flag Adam.**
- **F3 [C]** — `_ContextOwnerSavedId` captured (`CaptureV3.cpp:351-352`, `Header.h:130`) but never read → a
  post-spawn `Request_Override` context owner silently reverts. Fix (robust): re-link in `DoRebuild_Tick` after the
  owner maps (mirror the lifetime-owner wait). Fallback: delete field + capture line (don't imply fidelity we don't
  deliver). Prefer the re-link.
- **F4 [C]** — dead `SeedContainer` machinery: delete `FHandler::SeedContainer` + its participation-rule doc, the
  `RegisterLazyTyped` default-seed synthesis, `Get_ReDriveHandlerTypes`, `ck::attribute_restore::SeedContainer` + the
  5 `.SeedContainer =` assignments + MontagePlayer/AnimPlan seeds. After this, "a feature declares its persisted state
  in ONE place" is literally true. (Erases ~40 comment violations at the root.)

**Record-only (out-of-scope follow-ups):** R1 GC-untraced pending-hydration payloads (`FFragment_PendingHydration._Entries`
+ `FFragment_Sm_HydrationResume` hold class/asset refs across load frames; narrow window; robust fix = a `TStrongObjectPtr`
pin array cleared at `DoFinish_Load`, or a GC-delay scope) — real, low-probability/high-severity; R2 net-side
`_LastAppliedData` untraced; R3 stale-binary trap migrates to the `RegisterLazy*` registrars (keep the doctrine note,
retarget it); R4 save-payload order nondeterministic (`TMap` iteration — sort by `GetPathName()` if OracleParity byte-diff
is ever wanted); R5 partial-load policy; R6 `LoadIfFindFails` silent asset-null; R7 attribute component-drift logs at
Verbose (bump to Warning — batch with F4's file touch); R8 tombstone idioms (repeat both in any new raw-storage loop);
R9 hydration >5s timeout; R10 `Request_Save` mutates world (deliberate — record so nobody "fixes" it).

---

## Objective 4 — unify (persistence + replication are one)

**CONFIRMED [C via audit chain]:** after Model-A deletion, persistence flows ONLY through the handler registry —
save via `Get_SaveHandlerTypes → Produce` (`CaptureV3.cpp:191,369-384`), load via `FProcessor_Hydration_Dispatch →
Apply` (`CkReplicatedFragmentContainer_Processor.cpp:129-229`, the SAME `Apply` net receive uses). "Declared in ONE
place" holds with three DELIBERATE, documented qualifications: (1) delete `SeedContainer` (F4) or the contract stays
three-legged on paper; (2) Transform's persistence lives in the capture core as the `_ActorSpawnTransform` recipe
field, not the feature registrar — because a Transform Produce is hazardous (`SyncFromActor` stomp) — document at the
registrar-contract site; (3) EntityScript state is declared on the script class via `UPROPERTY(SaveGame)` (a second,
complementary surface, by design).

**Fidelity gap — Model-A-captured state NOT in the v3 Produce surface (19 registrations today).** These are
PRE-EXISTING v3 limitations (Model A is test-only), NOT regressions the deletion introduces. Classification:
- **Real holes → Adam product/scope decisions (STOP-and-flag; do NOT silently close or silently drop):**
  - **G1 Transform** — non-bridged movers + the EngineOwned player-pawn position + moved ConstructSpawned children do
    not restore (only bridged RuntimeSpawned get `_ActorSpawnTransform`). Closure = a per-provenance transform column
    (loader applies via actor `SetActorTransform` for actor-backed; a load-kernel-safe request for pure-ECS movers;
    avoid the `SyncFromActor` stomp). A design task.
  - **G2 Dynamic fragments** — AS/runtime-typed fragment state is save-invisible (the per-type Save census structurally
    can't enumerate a fallback handler). Decide: (a) declare it save-transient by contract (AS state persists via
    `UPROPERTY(SaveGame)` script fields, Track A), or (b) add a synthetic per-entity dynamic-fragment Produce. The single
    most consequential functionality call in the deletion.
  - **G3 Timer** elapsed/goal, **G4 EntityTag** runtime tags, **G5 EntityCollection** contents — next tier; each is a
    tracked-Produce task or a documented by-design transient. G3 intersects the 4B.1 params-mutator list.
  - **G17 item-owned child attribute state (stack count) [adversarial-verified]** — a definition-built item's attribute
    children (stack count via `IntegerAttribute::Add`, `CkItemTrait_Stackable.cpp:55`) are anonymous (Rule 5) → runtime
    value not captured; reverts to the definition default on load. Robust closure = extend the capturable provenance to
    definition-built-item children so item attributes round-trip via the existing attribute pipeline (recommended), or
    enrich the `DefinitionBuilt` entry with runtime attribute values. Not required for the two F1 gates (non-stackable
    fixtures); pre-existing v3 gap. Decide: close in F1 vs defer.
- **Known/deferred:** G10 = the [INV-A] trio (obj 1); G11 MontagePlayer (SKMC rebind, 4B.1); G12 EntityScript full
  instance → SaveGame-fields-only (deliberate narrowing — document loudly at doctrine level); G13 SM canonical-single-event
  (deliberate, [SM-A]); the 4B.1 params-mutator sites (Timer/Substep/Goap/2dGridCell/WorldSpaceWidget/Camera/Pmg/Probe).
- **Covered (ii)/transient (i):** G6 SceneNode hierarchy (Construct), G7 Objective (attribute-backed), G8 InteractTarget
  (transient by design), G14 core structural (rebuild), G16 tags (re-derive).

**Disposition:** the deletion + obj-1 fix + F1–F4 complete the "one pipeline / no legacy / reds fixed / clean" mandate.
The fidelity gaps G1/G2/G3/G4/G5 are genuine product-scope decisions surfaced for Adam (post-campaign coverage work),
NOT silently closed and NOT silently dropped. Removing Model A regresses no live fidelity; it removes the detection net,
which the optional v3-round-trip structural test (above) can restore later.

---

## Objective 5 — hygiene (comments + ck::algo), across the touched surface

Touched set: `git merge-base origin/dev HEAD` = `9fbf90c0feae` **[A — re-derive at exec]**; ~143 surviving source files.
~290 house-rule-violating comment lines across 84 files; ~120 die free with the obj-2 deletion.
- **Comment sweep (comments-only commit; diff shows zero non-comment token changes except one ensure-string tag at
  `CkStateMachine_Processor.cpp:1058`).** Rule: **keep the invariant, delete the history** — a comment "X because Y (the
  Model-A re-drive was retired in Phase 5)" becomes "X because Y". Remove every reference to plan/phase/cluster/campaign,
  decision tags `[Pn-*]`/`[INV*]`/`[N1*]`, `Claude`/`Fable`/`Opus`, `Model A/B`, `v2`/`v3` (as campaign shorthand),
  `spec §`, `CTO`, `BusterBlock`/`BB`. Densest: `CkSnapshot_Subsystem.cpp` (34), `CkStateMachine_Replication.cpp` (13),
  `CkReplicatedFragmentContainer.h` (13). Preserve the load-bearing WHY blocks (the ATOMIC gate-open rationale, the
  GC-pin contract, the Old-side coalescing contract) minus their citations. Fix the two stale-FALSE "DORMANT in Phase 1"
  notes on the now-live hydration queue. Reword `CkSnapshot_RestoreMarker.h`'s comment campaign-free. False positives NOT
  to touch: `[INVALID REGISTRY]` runtime strings, Graphviz "cluster", AnimPlan "goal/cluster/state" domain vocab.
- **ck::algo pass (separate commit, gated):** 6 strong conversions in surviving campaign-heart code —
  `Subsystem.cpp:399-404` (NoneOf), `:383-387` (ForEachIsValid), `:317-318` (Transform-into-TSet — verify the overload
  compiles), `:597-605` (FindIf), `CaptureV3.cpp:193-204` (AnyOf), `CkEntityReplicationDriver_Utils.cpp:120-124` (AnyOf);
  plus `Classified.Sort → ck::algo::Sort`, `AnimPlan` ForEach, request-drain → `ForEachRequest`. Do-NOT-convert: EnTT
  view/reflection/`TActorRange` loops, the two stateful dispatcher drains. ADD candidates (TransformIf, GroupBy) — not
  worth adding for single call sites; leave.
- **V3-identifier question (Adam call; executor default = KEEP):** `Run_CaptureV3`/`FCk_Snapshot_HeaderV3`/
  `ECk_Snapshot_V3_Provenance`/`DoDeserialize_V3Blob`/`CkSnapshot_CaptureV3.*` etc. A save-format version suffix is
  legitimate on-disk-format vocabulary (a future v4 needs it); renaming ~50 symbols + 2 files is high churn/blame cost.
  Keep the identifiers; strip `v3`-as-campaign-shorthand from COMMENTS. Adam may override.
- **Tier-D pre-existing comments** (earlier campaigns' `Phase 7`/`Phase 10`/`spec §9` in `CkStateMachine_Processor.*`,
  scheduler `pre-Phase-3`): out of this campaign's scope; sweep only on Adam's go-ahead. Note some apparent pre-existing
  §5.x comments were actually moved by this branch's 4A.1 work → Tier B.

---

## Phased implementation plan (per-phase full-rebuild gate; Class-4)

Each phase: implement on Opus → **full `--build` (Class-4 stale-binary trap)** → gate via UnrealToolbox (editor
CLOSED; read verdicts from the `--output` log, `--discover-fresh` for Ck.Snapshot) → commit (stage only changed files
BY NAME; no Co-Authored-By; NO push) → update PROGRESS.md + this file. Maintainer (Adam) review MANDATORY before any push.

| Phase | Objective | Deliverable | Gate |
|---|---|---|---|
| **F1** | obj 1 | [INV-A] fix (items/occupants round-trip through the shared pipeline) | `Ck.Snapshot` ZERO reds; `Ck.*.Net` delta-zero. Oracle still present as cross-check. |
| **F2** | obj 3 | F4 (SeedContainer dead-code) + F1/F2/F3 live-path footguns | `Ck.Snapshot` still zero reds; `Ck.*.Net` delta-zero. F2 timing change flagged to Adam. |
| **F3** | obj 2+4 | Legacy purge (per the 6-cluster order) incl. CkTests HandleWalk/ProduceSensitivity/AttributeModifier migration | Dev build + **Shipping Game compile** + re-baselined `Ck.Snapshot` (~39-40, zero reds) + `Ck.*.Net` delta-zero + exit greps = 0 |
| **F4** | obj 5 | Comment sweep (comments-only) + ck::algo pass (separate) | Build clean; gate delta-zero; comment exit greps = 0 across touched files |

Order rationale: obj-1 FIRST (fix reds while the oracle can still cross-check the new item Produce), then live-path
footguns (small, testable against the green gate), then the purge (which also erases ~120 comment violations for free),
then the residual hygiene. If context runs out, CHECKPOINT at a clean committed phase boundary — never mid-purge.

## Decisions ledger (executor; verify-before-act; Adam may veto via PROGRESS)

- **[F-D1]** Fidelity oracle dies WHOLE; `Oracle.ProduceDiffBaseline` → oracle-free `V3.ProduceSensitivity`;
  `Oracle.StructuralBaseline` deleted. Rationale: both tests are Model-A-entangled; Tier-1/2 machinery has no other
  consumer. Verified: `FidelityOracle.h` (Model-A-independent but only oracle-test-consumed), Audit C rows #19/#20.
- **[F-D2]** SeedContainer is DEAD (zero invocations) → delete it; the contract is Produce/Apply(+Remove). **[C]**
- **[F-D3]** Keep the `V3` on-disk-format identifiers (not a comment); strip `v3`-as-campaign-shorthand from comments.
- **[F-D4]** Policy-macro fork default = Option A (drop `T_Policy`, plain macros, ~85-site rename); Adam may pick (c).
- **[F-D5]** obj-1 precedes the purge (oracle cross-check + `GridPlacements.RoundTrip` deletes only after its parity twin greens).
- **[F2-D1] (executor, 2026-07-13) — obj-3 F2 Settling timing change (ADAM behavior-review).** `Settling` now finishes only
  once `DoIs_HydrationComplete()` is true (no payloads pending apply) AND the minimum settle frames elapsed, with the
  frame-cap as a LOUD `CK_TRIGGER_ENSURE` abort. Previously it finished on a bare frame countdown, so `OnLoadComplete`
  could broadcast with hydration still in flight. **Behavior change: `OnLoadComplete` now fires later (after hydration
  drains).** Gate held Ck.Snapshot 53/1 + Net delta-zero (load consumers poll state, not the signal), but any consumer
  that assumed early `OnLoadComplete` semantics is affected — Adam to confirm the timing is acceptable.
- **[F2-D2] (executor, 2026-07-13) — obj-3 F3 `_ContextOwnerSavedId` re-link NARROWED (was "captured but never read").**
  The F1 ctx-owner-first rebuild now READS `_ContextOwnerSavedId` for DefinitionBuilt owner resolution, so the field is
  no longer dead and the "delete field + capture line" fallback is off the table. The residual footgun — a RuntimeSpawned
  entity whose context owner was `Request_Override`n POST-spawn silently reverting to the Construct default on load —
  is NOT closed (it needs a re-link in the RuntimeSpawned rebuild branch after the mapped ctx owner resolves). Recorded
  as a follow-up (edge case; no test exercises it). Adam: implement the RuntimeSpawned re-link or accept the narrowing.
- **[F2-D3] (executor, 2026-07-13) — obj-3 F1 silent-payload-drop made loud.** `DoHydrate_Enqueue` now fires
  `CK_ENSURE_IF_NOT(Data.IsValid())` naming the payload type + owner saved-id, recovery counts into a new
  `_PayloadsDropped` LoadReport field. Doctrinally correct (non-negotiable #3): loud in Dev, silent-but-counted in
  Test/Shipping. Content-drift (a fragment type removed since the save) legitimately trips it → surfaced, not hidden.

## Adam-decision / STOP-and-flag list (do NOT resolve unilaterally)
1. **[INV-A] item-model** — only if AuditA concludes the honest fix needs an irreversible item-MODEL/save-format/net
   change (else implement the CkFoundation-only robust fix — prompt pre-authorizes).
2. **RestoreMarker / FTag_Snapshot_JustRestored removal** — needs BB-coordinated cleanup (BB is off-limits this session).
3. **Fidelity gaps G1 (transform incl. player pawn), G2 (dynamic fragments), G3–G5, G17 (item stack-count / item-owned
   child attribute state)** — product-scope coverage decisions. G17 has a recommended robust closure (see obj 4); the
   rest are pre-existing v3 limitations Model A only *detected*, not fixed.
4. **F2 OnLoadComplete timing change** — behavior change for all load consumers.
5. **Policy-macro Option A vs (c); LoadReport/Header vestigial-field prune (BlueprintType surface); V3-identifier rename;
   Tier-D pre-existing comment sweep** — churn/scope calls.
6. **Timer / MontagePlayer v3 payloads (4B.1)** — land-before-delete vs accept-loss for their Model-A tests.

## Optional follow-ups (recorded, not blocking)
- A v3-round-trip structural regression net (capture→save→load→capture→structural diff), replacing the retired oracle's
  gap-detection role (the "coverage cliff" mitigation).
- R1 GC-pin for pending-hydration payloads. R4 deterministic save-payload order. The 4 oracle-free `Rebuild.*` tests
  (NoDuplicateGrants/LostGrantStaysLost/OrphanHydrationLoud/SpawnerResumesPastSpawnDecision) named in VALIDATION.md.
