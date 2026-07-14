# Continuation — CkSnapshot rebuild+hydrate, Phase 4B (then 4A.2 → 5)

**One-line:** Phase 4A.1 is DONE + COMMITTED — CkStateMachine now has a Save-transport hydration path
(`FProcessor_Sm_HydrationResume`), so SM run-state restores across a v3 load. Gate `Ck.Snapshot` **52/45/7**
(both `Parity.StateMachine*_MPReload` GREEN, 9→7 casualties; the 7 remaining are Phase-4B feature-payload
casualties). You are **Opus**; stay Opus for implementation; route design forks through a **Fable** agent
(`Agent`, `model:"fable"`, read-only) and VERIFY every ruling against cited code yourself before implementing.

## 0. STATE (verify at start)
- CkFoundation `feature/save-load-improvements` HEAD = the 4A.1 docs commit, on top of:
  - `705e7d57e` feat(CkStateMachine): SM redrive as Save-transport hydration; delete RestoreRedrive
- CkTests HEAD = `773a4d2` docs(CkSnapshot): reword SM round-trip comment for HydrationResume rename.
- Tree CLEAN after the 4A.1 commits. NOTHING pushed. Editor CLOSED.
- Net baseline for 4B diffs = `CkAuto/logs/p4a-net.log` (102 total; 3 fails = the kiosk env-trio
  `Bb_AutoTest_RentnetKiosk*`; the SM.Net flake `OwningClientAuth_SubSm_AuthorityGatedTask` passed this run).
  A NEW framework `Ck.*.Net` red = investigate before 4B code.

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
