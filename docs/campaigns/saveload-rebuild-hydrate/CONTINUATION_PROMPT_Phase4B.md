# Continuation — CkSnapshot rebuild+hydrate, Phase 4B (then 4A.2 → 5)

**One-line:** Phase 4A.1 DONE (SM Save-transport hydration). Phase 4B IN PROGRESS — the client-shaped
hydration-scope authority-write pattern is validated: **TagSet DONE** (`Parity.TagSet_MPReload` green,
Ck.Snapshot **52/46/6**, 7→6 casualties). Remaining: Grid + RenderTarget (client-shaped, Fable-designed
below), Attributes + AnimPlan (empty-seed Produce), Inventory×2 (**DEFERRED — [INV-A] Adam fork**),
params-mutators, MontagePlayer, AS smoke. You are **Opus**; stay Opus for implementation; route design
forks through a **Fable** agent (`Agent`, `model:"fable"`, read-only) and VERIFY every ruling against cited
code yourself before implementing.

## 0. STATE (verify at start)
- CkFoundation `feature/save-load-improvements` HEAD = the latest 4B docs commit, on top of (newest first):
  - `5088f5336` feat(CkTagSet): hydration-scope authority write on save-load (Phase 4B)
  - `2f0aa0a7f` docs(CkSnapshot): Phase 4A.1 DONE
  - `705e7d57e` feat(CkStateMachine): SM redrive as Save-transport hydration; delete RestoreRedrive
- CkTests HEAD = `773a4d2` docs(CkSnapshot): reword SM round-trip comment for HydrationResume rename.
- Tree CLEAN. NOTHING pushed. Editor CLOSED.
- Net baseline for 4B diffs = `CkAuto/logs/p4b-tagset-net.log` (102 total; 3 fails = the kiosk env-trio
  `Bb_AutoTest_RentnetKiosk*`; no framework `Ck.*.Net` red). A NEW framework `Ck.*.Net` red = investigate.
- Remaining Ck.Snapshot casualties (6): `Parity.{AnimPlan,Attributes,GridPlacements,InventoryDataOnly,
  InventorySpatial,RenderTarget}_MPReload`.

## 1. READ FIRST (in order), in Plugins/CkFoundation/docs/campaigns/saveload-rebuild-hydrate/
1. PROGRESS.md — §Status board (4A.1 DONE row), §"4A.1 IMPLEMENTATION STATE" (what shipped + the gate),
   §Decisions [P4A-D1] (SM RegisterLazy/no-SeedContainer, Fable-confirmed), §"Phase-3B DONE" (the 9 casualties;
   2 now closed by 4A.1), the Unattended execution protocol.
2. PHASE_4B.md (the phase spec) — NOTE the reconciliation gap in §2 below.
3. `docs/specs/2026-07-10-CkSnapshot-rebuild-hydrate-design.md` §6 (Phase-4 coverage). `*.md` is gitignored —
   campaign docs are force-added (`git add -f`).

## 2. The 7 remaining Ck.Snapshot casualties 4B must close (the real success measure)
All are v3-load parity reds where value-emitting Produce is necessary but NOT sufficient — the Apply must write
AUTHORITY-side under hydration scope. **4A.1's SM branch is the exact template:** an Apply whose FIRST statement is
`if (FCk_HydrationApplyScope::Get_IsActive()) { <authority-side write>; return Applied; }` (above the client-shaped
path). Two sub-classes:

- **Empty-seed Produce (2):** `Parity.Attributes_MPReload`, `Parity.AnimPlan_MPReload`. Their Produce is empty-seed
  ([P1-D2], `CkAttribute_RestorePersistence.h`) → the VALUE isn't persisted at all. 4B must make their Produce
  value-emitting (per-owner upsert-merge — the comment flags it: multiple attributes share one owner container) AND
  ensure the Apply writes authority-side.
- **Client-shaped Apply (5):** `Parity.TagSet_MPReload`, `Parity.GridPlacements_MPReload`,
  `Parity.InventoryDataOnly_MPReload`, `Parity.InventorySpatial_MPReload`, `Parity.RenderTarget_MPReload`. Their Apply
  stamps a ClientOnly sync fragment (e.g. TagSet: `FFragment_TagSet_SyncReplication` drained only by the `ClientOnly`
  `FProcessor_TagSet_SyncReplication`, `CkTagSet_Processor.h:98`) → on the loading AUTHORITY the sync never drains →
  server value never restored. 4B re-authors the Apply to a hydration-scope authority write (the 4A.1 SM pattern), NOT
  an ad-hoc authority sync drain. The v3-load DIAG already prints their symptom on the authority
  (`Hydration payload [Ck_RepData_Inventory_Spatial_Items] / [Ck_RepData_RenderTarget] ... NotReady for 5s ... Dropping`).

**RECONCILIATION GAP (resolve at 4B start):** PHASE_4B.md's WRITTEN steps (4B.1 params-mutators, 4B.2 RenderTarget
re-author, 4B.3 AS smoke) do NOT explicitly enumerate the Attributes/AnimPlan empty-seed + TagSet/Grid/Inventory×2
client-shaped Apply work the 7 casualties require. The campaign's own CONTINUATION_PROMPT_Phase4A §3 + PROGRESS
§"Phase-3B DONE" frame those 7 as 4B's job. Treat "close the 7 Ck.Snapshot casualties" as 4B's PRIMARY exit
criterion; fold PHASE_4B.md's params-mutator/AS-smoke steps in as additional oracle-coverage work; record the merged
4B scope in PROGRESS. Route to Fable only if the two framings genuinely conflict on approach.

## 3. PHASE_4B.md's own steps (oracle coverage — additive to the 7-casualty closure)
- 4B.1 Params-mutator census (8 sites, decisions already tabled): Save-transport payloads (`Transport = Save` only —
  NOT on the wire) or annotate declared-transient (`oracle-declared-transient.txt`). MontagePlayer = post-hydration
  `Request_RebindSkeletalMeshComponent` from Apply under hydration scope.
- 4B.2 RenderTarget re-author (overlaps casualty #RenderTarget above).
- 4B.3 AS smoke matrix: `Ck.Snapshot.AS.SaveGameFields_RoundTrip` + `NonSaveGameField_Drops` — a framework-level
  `FCk_SaveData_EntityScriptFields` handler serializing the script instance's SaveGame-tagged fields (framework handler,
  not per-script). Load `ck-angelscript-interop` + Script/CLAUDE.md first; never write `.as` during a test run; grep
  the fresh log for `Angelscript: Error` before claiming green.
- 4B.4 Gate: `Ck.Snapshot --discover-fresh` + `Net`. Exit: the 7 casualties GREEN; OracleParity green with only the
  declared-transient list; the 2 AS tests green.

## 4. Then: 4A.2 (deferred N1) and Phase 5
- **4A.2** (deferred, [N1-A]-blocked — needs an Adam scope call + a new BB-style fixture world): the N1 discriminator
  ([P4A-F2]: extend `Get_IsSnapshotRespawnable` to the non-bridged path + the SM free-run gate on
  `FTag_Hydration_PendingApply` in `FProcessor_Sm_HandleRequests`) + the
  `Ck.Snapshot.Rebuild.SpawnerResumesPastSpawnDecision` test. Do NOT start 4A.2 until [N1-A] scope is decided by Adam.
- **Phase 5** (PHASE_5.md + VALIDATION.md): decommission Model A under `CK_WITH_FIDELITY_ORACLE` (gate, don't delete);
  delete the dormant `FProcessor_ActorRespawn` per [P3B-F1]; VALIDATION.md is the definition of done.

## 5. Locked (don't re-litigate)
- SM save-transport handlers use `RegisterLazy` + `.Produce` + `NetAndSave`, NO SeedContainer ([P4A-D1], Fable-confirmed:
  SeedContainer's sole consumer is the Model-A `FProcessor_Persistence_ReDriveOnRestore`, JustRestored-gated). The
  net-path SM Apply is byte-identical; the hydration branch is additive, gated by `FCk_HydrationApplyScope::Get_IsActive()`
  (set only by `FProcessor_Hydration_Dispatch`). The branch is FIRST, above echo-suppress (load-bearing — echo-suppress
  AutoDetects player-pawn SMs to OwningClientAuthoritative and would else swallow the payload). Do NOT weaken.
- The client-shaped-Apply re-author pattern (hydration-scope authority write) is the template for the 5 client-shaped
  casualties — do NOT invent an authority-side sync-fragment drain.
- Sub-SM saved control-state is NOT restored (pre-existing v1 bound — the old Model-A orphan-destroy also discarded it;
  the parent's task re-creates the sub-SM fresh in InitialState). In scope only if a later phase adds sub-SM persistence.
- `FTag_Sm_IsSubMachine` is now write-only (its sole reader, the deleted orphan-destroy, is gone). Kept as the sub-SM
  discriminator for 4A.2/N1 sub-SM handling. Reuse or remove it there.
- Cross-repo: CkTests never ahead of / merged before CkFoundation. No push.

## 6. Fable 4B design map for the remaining client-shaped casualties (VERIFIED against code where noted; re-verify anchors)
Uniform SHELL (hydration-scope branch FIRST → authority write → Applied/NotReady, the TagSet/4A.1 shape), per-feature BODY.
**TagSet is the DONE reference** (`CkTagSet_Fragment.cpp` Apply: `Set_Tags(saved)` REPLACE + `AddOrGet<FTag_TagSet_MayRequireReplication>()` arm; NotReady until `Has<FFragment_TagSet>`).

- **Grid — `Parity.GridPlacements_MPReload`** (Apply-branch-only IF occupants map). Registrar `Ck2dGridOccupancy_Fragment.cpp:39-79`;
  Entity IS the grid (Has `FFragment_RecordOf_GridPlacements`). Under hydration scope: cast to `FCk_Handle_2dGridSystem`
  (`UCk_Utils_2dGridSystem_UE::Cast`), then for each `FCk_2dGridPlacement_ReplicatedEntry` in
  `New.Get<FCk_RepData_2dGridPlacements>().Placements` (accessors `Get_Occupant/Anchor/Rotation/Cells`,
  `Ck2dGridPlacement_Fragment_Data.h:117-120`) call `UCk_Utils_2dGridOccupancy_UE::Request_AddPlacement(Grid, Occupant,
  Anchor, Rotation, Cells)` (`Ck2dGridOccupancy_Utils.h:32-37` — immediate compose, self-arms replication, auto-replaces
  a stale placement for that occupant). **ALL-OR-NOTHING:** resolve ALL occupants first (`ck::IsValid(Get_Occupant())`);
  if ANY invalid → NotReady BEFORE placing any (else a partial placement duplicates on the dispatcher retry). ADD suffices
  (nothing seeds placements at Construct). **⚠️ UNVERIFIED RISK (check first):** whether the `GridPlacements_MPReload`
  test's OCCUPANTS are v3-captured (mapped). If they are inventory-item-style anonymous entities (see Inventory below),
  the occupants never map → perpetual NotReady-timeout → Grid stays red and becomes the SAME [INV-A] class. Read the test
  fixture before implementing; if occupants are anonymous, fold Grid into [INV-A].

- **RenderTarget — `Parity.RenderTarget_MPReload`** (the special one; PHASE_4B §4B.2). NOT the request path — **transplant the
  Model-A `FProcessor_RenderTarget_ReplicateOnRestore` body** (`CkRenderTarget_Processor.cpp:397-500`) into a static helper
  `ck_render_target_processor::HydrateFromSavedChannel(Child, channel)` in `CkRenderTarget_Processor.cpp` (needs friend access
  to `Current._NextBatchSeq`, `CkRenderTarget_Fragment.h:214,237`). The payload is **CHILD-keyed** (Produce reads the sync
  child's Params+AuthoredLog, `CkRenderTarget_Replication.cpp:120-140`), NOT owner-keyed like the net Apply's `TryGet_RenderTarget`
  loop — the hydration branch operates on the child directly. Helper: refill `FFragment_RenderTarget_AuthoredLog` from the
  payload batches; `Current._NextBatchSeq = LatestSeq+1`; `FProcessor_RenderTarget_HandleRequests::DoApplyBatch(Child, Current,
  Batch.Get_Cmds())` per batch in order; if `Get_Replication()==Replicates` && host → refill the owner container via
  `TryAddContainerFragment`+`TryUpdateContainerFragment<FCk_RepData_RenderTarget>` (mirror `:470-500`). **EXACTLY-ONCE repaint:**
  evaluate ALL NotReady preconditions (child composed + NOT `Has<FTag_RenderTarget_NeedsSetup>`; if Replicates → owner driver
  present via `UCk_Utils_EntityReplicationDriver_UE::Has(Owner)`) BEFORE any mutation (replaying translucent draws per retry
  accumulates alpha — Model-A warns at `:414-417`). Repaint runs unconditionally; only the container-refill half gates on
  Replicates (DoesNotReplicate RTs are still save-worthy). **⚠️ RISK Fable flagged:** verify the load's Full settle-pump runs
  RenderTarget Setup (`FGroup_Gameplay_Rendering`) before the hydration dispatcher's 5s timeout — check `DoTick_Load`'s pump scope.

- **Inventory DataOnly + Spatial — `Parity.Inventory{DataOnly,Spatial}_MPReload` — [INV-A], DEFER to Adam.** Their payload
  carries only ITEM HANDLES (`FCk_InventoryItem_{DataOnly,Spatial}_ReplicatedEntry`), but item entities are built via
  `UCk_Utils_EntityReplicationDriver_UE::Request_BuildAndReplicate` with a `UCk_InventoryItem_Definition` archetype
  (`CkItem_Utils.cpp` `Create`) — NOT the EntityScript spawn path, so **no `FFragment_SpawnRecipe`** → v3 provenance Rule 5
  ("anonymous scratch", skipped `CkSnapshot_CaptureV3.cpp:267-271`) → items are not captured and not rebuilt. On the loading
  authority the saved handles remap to nothing → an Apply branch would drain against invalid handles + assert-storm. **Real
  fix is a product/architecture fork (Adam):** (a) payload ENRICHMENT — Produce emits per-item re-creation data (definition
  soft path / `FCk_InventoryItem_CoreInfo`+traits + stack count, which is an IntegerAttribute → intersects the Attributes
  empty-seed workstream; note `GetOrCreate_TransientItemDefinition` complicates asset-path-only), and the Apply re-creates via
  `UCk_Utils_Item_UE::Create`/`Request_AddItemByDefinition` then places (Spatial); OR (b) make items SpawnRecipe-carrying so the
  loader rebuilds them and the Apply stays a pure re-link (item-architecture change with net implications). Do NOT wire an Apply
  branch that stamps the sync fragment authority-side — worse than the honest red.

- **Attributes + AnimPlan — `Parity.{Attributes,AnimPlan}_MPReload`** (empty-seed Produce, [P1-D2]): SEPARATE workstream — make
  their Produce value-emitting (the per-owner upsert-merge the comment flags: multiple attributes share one owner container)
  AND ensure the Apply writes authority-side. Stack-count for Inventory is an IntegerAttribute → these two workstreams intersect;
  consider sequencing Attributes before the Inventory [INV-A] decision.
