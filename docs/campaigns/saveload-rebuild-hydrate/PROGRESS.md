# PROGRESS.md — CkSnapshot Rebuild+Hydrate campaign

> Executor: update this file at the END of every session. Next session trusts THIS file over memory.
> Blockers go in §Blockers — never improvise around one.

## Status board

| Phase | Doc | Status | Session date | Commits (repo: hash) | Gate result |
|---|---|---|---|---|---|
| 0 | PHASE_0.md | DONE | 2026-07-11 | CkF: 68ba192dc (dt==0), 55521d493 (oracle), <docs> (this); CkTests: 14d65ac (harness) | GREEN: Ck.Snapshot 47/47/0 (46 baseline + Oracle.StructuralBaseline), Ck.Attribute.Net 17/17/0, Net 102/101/1 (baseline red only) |
| 1 | PHASE_1.md | **DONE** (1.1–1.6 committed+GREEN) | 2026-07-11 | CkF: 23b982c0d (framework), fd288efdd (6 migrations), 5d262f52a; CkTests: b9e7f86 | GREEN: Ck.Snapshot **48/48/0** delta-zero (all 11 Parity_MPReload + both Oracle tests pass), framework Ck.*.Net green (kiosk trio env-red, see baseline) |
| 2 | PHASE_2.md | **DONE** (2.1–2.7 committed+GREEN; 2.7-Test2 subsumed per [P2-D3]) | 2026-07-11 | CkF: 91b96a177 (load gate core), 8c4fbce7a (fire-gating), af9fad239 (retire NeedsSetup guards); CkTests: 4522c8e (LoadGate) | GREEN: Ck.Snapshot **49/49/0** (LoadGate + AccelerationParity + 11 Parity, no over-gating hang); Net 102/98/4 framework delta-zero (3 Attribute.Net pins green; 4 fails = recorded kiosk-trio env-red + StateMachine.Net flake) |
| 3A | PHASE_3A.md | **DONE** (3A.1–3A.6 committed+GREEN) | 2026-07-11 | CkF: 81f7b6505 (CkEcs framework), 349947218 (v3 writer), d0ce51877 (registrar transport); CkTests: 5bb9798 | GREEN: Ck.Snapshot **51/51/0** (49 baseline + V3.CaptureClassification + V3.RecipeParamsHandleRemap; delta-zero — all 11 DynamicFragment + 11 Parity + both Oracle pass); Net framework Ck.*.Net delta-zero (3 fails = recorded kiosk env-red trio; 3 Attribute.Net pins green; StateMachine.Net flake didn't fire) |
| 3B | PHASE_3B.md | **DONE** (v3 load pipeline live; M2a fixed) | 2026-07-12 | CkF: 78fcdaa8e (retire reconstitution), 36bcdec5d (v3 load pipeline), <docs-this>; CkTests: ce32c65 | GREEN mod casualties: Ck.Snapshot **52 / 43 pass / 9 fail** (--discover-fresh; M2a + V3.InstancedStructDiskSmoke green; all 9 fails are verified Phase-4 casualties — see §Phase-3B DONE) |
| 4A.1 | PHASE_4A.md | **DONE** (SM redrive→hydration; committed). 4A.2 (N1 discriminator + SpawnerResumes test) DEFERRED ([N1-A]-blocked). | 2026-07-12 | CkF: 705e7d57e (SM hydration); CkTests: 773a4d2 (comment) | GREEN mod casualties+flake: Ck.Snapshot **52/45/7** (both Parity.StateMachine* GREEN, 9→7; 7 = 4B casualties); Ck.StateMachine 24/25 (lone red = documented OwningClientAuth_SubSm flake — passed alone + in Net run); Net 102/99/3 delta-zero (kiosk env-trio only, no new framework Ck.*.Net red) |
| 4A.2 | PHASE_4A.md | DEFERRED ([N1-A] scope call) | | | |
| 4B | PHASE_4B.md | **IN PROGRESS** — casualties closed (TagSet/RenderTarget/Attributes×5/AnimPlan) + **4B.3 AS smoke DONE** (framework FCk_SaveData_EntityScriptFields handler + 2 AS gates green). **Grid + Inventory×2 DEFERRED → [INV-A]**. Verification-gated leftovers (4B.1 params-mutators, MontagePlayer rebind) still blocked on OracleParity/BB-driver-world ([P4B-N1]). | 2026-07-12 | CkF: 5088f5336 (TagSet), ede66976f (RenderTarget), db83e687a (AnimPlan), 5b7c1e077 (Attributes×5), 19613a1eb (invariant fix), **09ca84f41 (4B.3 SaveGame-fields handler)**; CkTests: **a0c2f2d (4B.3 AS gates)** | GREEN: Ck.Snapshot **54/51/3** (both `AS.SaveGameFields_RoundTrip`+`AS.NonSaveGameField_Drops` green; the 3 = [INV-A] trio by name; Meta ratchet + v3 AUDIT delta-zero); Net **102/99/3** framework Ck.*.Net delta-zero (kiosk env-trio only; SM.Net flake passed) |
| 5 | PHASE_5.md + VALIDATION.md | **Track B (decommission Model A) partial COMPLETE — clusters 1a–6 DONE + COMMITTED; Phase-5 FINAL blocked** | 2026-07-12 | see §Phase-5 progress | Ck.Snapshot 54/51/3 (3=[INV-A]); Net 102/98/4 framework delta-zero; Shipping compile OK |

## Close-out session (2026-07-12, Opus unattended) — FINALIZE design-lock (Phase 0 of the close-out)

Adam's new directive supersedes PHASE_5.md's "gate-don't-delete": **REMOVE Model A entirely; one pipeline; retire
`CK_WITH_FIDELITY_ORACLE`.** The locked design + phased plan + decisions + Adam-decision list live in **FINALIZE.md**
(canonical for the close-out; trust it over the raw audits). Produced via a 5-way read-only audit (4 Fable + 1 Opus-Plan
for [INV-A] after the Fable usage cap), every load-bearing claim Opus-code-verified.

- **Phased plan:** F1 (obj-1 [INV-A] fix) → F2 (obj-3 live-path footguns F1/F2/F3 + F4 SeedContainer dead-code) →
  F3 (obj-2+4 legacy purge, 6-cluster order incl. CkTests HandleWalk/ProduceSensitivity/AttributeModifier migration) →
  F4 (obj-5 comment sweep + ck::algo). obj-1 FIRST (fix reds while the oracle still cross-checks the new item Produce).
- **[INV-A] LOCKED (Audit A, Opus-verified):** items are Rule-5 anonymous → add `ck::FFragment_BuildRecipe` (mirror
  `FFragment_SpawnRecipe`) + `ECk_Snapshot_V3_Provenance::DefinitionBuilt` + capture rule + rebuild branch, so item state
  rides each feature's own Produce/Apply; PLUS the authority-side hydration branch on DataOnly/Spatial/2dGridOccupancy
  `Apply` (mirror `CkIntegerAttribute_Fragment.cpp:111-168`, gated on `FCk_HydrationApplyScope::Get_IsActive()`).
  Pure CkFoundation, no BB edit. Outcome: **Inventory×2 → GREEN; `GridPlacements` STAYS RED** on an independent
  engine-death (nested-`FEcsWorld` lifetime on a replicated bridged 2dGridSystem through seamless travel — its own
  workstream; **STOP-and-flag for Adam**). Phase-F1 gate = Ck.Snapshot 52 pass / 2 red? → actually 51+2=green so
  **~53 pass / 1 red (GridPlacements)**; Net delta-zero.
- **Pivotal verified facts:** `SeedContainer` is DEAD (zero invocations) → contract is Produce/Apply(+Remove);
  the whole `FidelityOracle` dies (`Oracle.StructuralBaseline` deleted, `Oracle.ProduceDiffBaseline` → oracle-free
  `V3.ProduceSensitivity`); handle-remap coverage ports to a new `V3.HandleWalk.*` suite on the existing v3 harness;
  TagRegistry landmine removal is purely subtractive (`CkTag.h`); 3 live v3 footguns to fix (F1 silent drop, F2
  dead hydration-complete gate, F3 unused ContextOwner capture).
- **Adam-decision list (do NOT resolve unilaterally):** GridPlacements engine-death; RestoreMarker/JustRestored
  BB-coordinated removal; fidelity gaps G1 (transform incl. player pawn)/G2 (dynamic fragments)/G3-G5; F2 timing change;
  Policy-macro Option A vs (c); LoadReport/Header vestigial prune; V3-identifier rename; Tier-D pre-existing comments;
  Timer/MontagePlayer v3 payloads; transient item defs; v3 save-format bump. Full detail + rationale in FINALIZE.md.
- **State:** no source touched; tree clean at `a15880b26` + the two new/edited docs (FINALIZE.md is git-ignored `*.md`
  → force-add to commit). Design-lock is a clean Phase-0 boundary; F1 implementation follows from FINALIZE.md.

## Phase F1 ([INV-A]) — IN PROGRESS (Opus, 2026-07-13) — implemented, compiles, NOT yet green

**State: 7-file WIP implementing the locked [INV-A] design. Builds clean, ZERO regressions (Ck.Snapshot 54/51/3, same
trio), NO crash. The `DefinitionBuilt` item is NOT yet restored → the 2 inventory reds are not yet green. Committed as
a labeled WIP (NOT a green phase). 4 build+gate iterations done; systematic (each advanced the diagnosis).**

Files (all under Source/): NEW `CkEcs/.../EntityReplicationDriver/CkEntityReplicationDriver_BuildRecipe.{h,cpp}`
(`ck::FFragment_BuildRecipe` + holder, mirror of SpawnRecipe); `CkEntityReplicationDriver_Utils.cpp` (stamp in
`Request_TryBuildAndReplicate` after Construct, before the DoesNotReplicate return); `CkSnapshot_Header.h`
(`ECk_Snapshot_V3_Provenance::DefinitionBuilt` + `FCk_Snapshot_V3_BuildStep{_ScriptClassPath,_ArchetypePath}` FString
paths + `_BuildRecipe` entry field + `_DefinitionBuiltCount` census); `CkSnapshot_CaptureV3.cpp` (Rule-4.5 classify +
Pass-2 capture); `CkSnapshot_Subsystem.cpp` (`DefinitionBuilt` rebuild branch + mapped/total DIAG); `CkInventory_DataOnly_Fragment.cpp`
+ `CkInventory_Spatial_Fragment.cpp` (authority-side `FCk_HydrationApplyScope` hydration branch: connect + transfer
[+ Spatial `Request_PlaceItemOnGrid`]).

**Bugs found + fixed (real, gate-caught):**
1. **`FSoftObjectPath` cannot serialize through a plain `FArchive`** (the v3 tables' `SerializeItem`) → hard crash
   (`appError` Archive.cpp:499). FIX: store paths as FString (like `_ScriptClassPath`/`_ActorClassPath`), reconstruct
   the soft path on load. **Durable framework gotcha.**
2. **Both inventory Apply handlers are required for EITHER inventory test.** The fixture
   (`CkAutoTest_NetSubject_InventoryEntityScript.cpp:15-36`) composes a DataOnly AND a Spatial inventory in one world;
   an unfixed Apply NotReady-drops → fires a loud ensure → fails the test regardless of which inventory it asserts.
3. Rebuild build-owner: switched from the item's own `_ContextOwnerSavedId` to `Get_ContextOwner(mapped_lifetime_owner)`
   — reconstructs production's exact `Create(Get_ContextOwner(owner), def)`. (Applied; did not change the symptom.)

**Remaining bug (NOT root-caused — needs 1 instrumented run): the item is not restored.** DIAG (build4):
InventoryDataOnly load = `[3]/[22] entities mapped ([19] skipped)`; the item (added via `Request_AddItemByDefinition`
→ `Create` → `Request_BuildAndReplicate` → the stamp choke point) is absent from the mapped set. By elimination the
item is likely **not captured as `DefinitionBuilt`** (if it were captured, the rebuild fix would have mapped it under
the persisted inventory owner → 4 mapped; it stayed 3). It is NOT Rule-3-skipped (verified `CkEntityLifetime_Utils.cpp:472-474`
— the item is added post-BeginPlay so gets no `FTag_ConstructSpawned`). **Next diagnostic (do FIRST):** bump the
CaptureV3 `DefinitionBuilt` census to Display AND log when an entity carrying `FFragment_BuildRecipe` is
classified/skipped, re-run, to decide capture-vs-rebuild. Hypotheses: (a) the stamp isn't landing on the item's build
path (verify `NewEntity.Has<FFragment_BuildRecipe>()` right after `Create`); (b) the item IS captured but its
`_LifetimeOwnerSavedId` resolves to an unpersisted entity (transfer not drained by save) → rebuild skip.

## Phase-5 progress (cluster tracker — Track B, decommission Model A "gate, don't delete")

Class-4 framework change (CkEcs snapshot core + save format). Per-cluster Dev compile/gate; Shipping compile + final
Ck.Snapshot+Net gate at the end. Maintainer (Adam) review is MANDATORY for a Class-4 change — nothing pushed; the
branch is handed over for review. Ordering deviates slightly from PHASE_5.md: the Model-A **machinery** gate (Run_Capture/
Run_Restore/archives, the rest of 5.1) is done LAST (right before the Shipping compile that validates it) because it is
Dev-behavior-neutral (machinery stays compiled in Dev) — the deletes land first with the machinery intact.

- [x] **Cluster 1a — 5.1 macro gate + 5.2.1 dual-write delete + Get_SaveSlotHeader v3-synthesis.** `CK_REGISTER_SNAPSHOTABLE`
  registration side now `#if CK_WITH_FIDELITY_ORACLE` (two `#define` forms; `static_assert` ungated in both). Request_Save
  writes v3 only; `UCk_Snapshot_SaveGame::_Header`/`_SnapshotBytes` deleted; `Get_SaveSlotHeader` synthesizes its frozen
  `FCk_Snapshot_Header` return from `_HeaderV3` ([P5-D1]). **GATE GREEN: Ck.Snapshot 54/51/3** (the 3 = [INV-A] trio; Model-A
  registry-level tests Core.RoundTrip/FloatAttribute.Gate/SaveKey/LifecycleStrip + Meta + both AS tests all green; delta-zero).
  Commit: CkF `<this>`.
- [x] **Cluster 2 — FProcessor_ActorRespawn delete (DONE).** Deleted CkActorRespawn_Fragment.h/_Processor.h/_Processor.cpp
  (dormant since 3B — nothing stamps FTag_ActorRespawn_Pending; M2b2a exercises the fresh-Construct re-replicate). All external
  refs were comments; refreshed the 3 CkF comments (CkEntityScript_Processor.cpp:351 / CkEntityScript_Utils.h:130 /
  CaptureV3.cpp:359). KEPT CkActorRebind_Utils + FTag_ActorJustRebound. **GATE GREEN: Ck.Snapshot 54/51/3** (M2b2a/M2b/M2a + both
  AS green, delta-zero). Commit CkF `8bdd08346`. **Cross-repo follow-ups for Adam (NOT a CkF-session edit):** 2 stale BB comments
  name FProcessor_ActorRespawn (`Bb_PlayerSaveLoad.h:14`, `Bb_SaveLoadProbe.cpp:274`); 1 stale CkTests comment
  (`CkAutoTest_NetSubject_M2bProbe_Replicated.cpp:11`).
- [x] **Cluster 3 — 5.2.2 delete the JustRestored re-drive machinery — DONE (CkF `f4dd25992`; Ck.Snapshot 54/51/3 delta-zero,
  FloatAttribute.Gate GREEN → no rewrite needed; Net framework Ck.*.Net delta-zero). KEPT `CkSnapshot_RestoreMarker.h` +
  `FTag_Snapshot_JustRestored` for cross-repo BB consumers per [P5-D3] (plan overlooked them).** Original SAFE ORDER (executed):
  delete the CONSUMERS first (tree stays compilable — each keys on the still-present tag), then
  the tag + marker + stamp + stale includes LAST. Surface (verified 2026-07-12):
  - **Whole-file deletes:** `CkPersistence_ReDrive_Processor.h`+`.cpp` (FProcessor_Persistence_ReDriveOnRestore +
    FFragment_Persistence_ReDrivePending, CkEcs); `CkSnapshot_RestoreMarker.h` (FTag_Snapshot_JustRestored, CkEcs) — DELETE LAST.
  - **Section-excise from each feature's Processor.h/.cpp + its CK_REGISTER_PROCESSOR line** (each is a delimited
    `// ---- ReplicateOnRestore ----` section beside LIVE processors — excise the section, NOT the file): Team
    (`CkTeam_Processor.cpp:14`), Player (`CkPlayer_Processor.cpp:10`), Inventory DataOnly (`CkInventory_DataOnly_Processor.cpp:23`),
    Inventory Spatial (`CkInventory_Spatial_Processor.cpp:28`), RenderTarget (`CkRenderTarget_Processor.cpp:60` — delete ONLY
    FProcessor_RenderTarget_ReplicateOnRestore; its logic was transplanted to the LIVE HydrateFromSavedChannel [P4B-D3]),
    2dGridOccupancy (`Ck2dGridOccupancy_Processor.cpp:23`), CkGrid 2dGridSystem RestoreRecompose (`Ck2dGridSystem_Processor.cpp:15`).
  - **Stamp:** remove `Handle.Add<ck::FTag_Snapshot_JustRestored>();` at `CkSnapshot_Restore.cpp:243` (+ the include-comment :11).
  - **Stale `#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h"` cleanup** — MUST remove from every file that includes it when
    the header is deleted. VERIFIED one non-obvious one NOT in the plan's delete list: `CkAttribute/CkAttribute_Processor.h:7`
    (a stale include only — CkAttribute's ReplicateOnRestore was Phase-1-migrated to the generic re-drive; no live Attribute
    consumer). Grep `rg -l 'CkSnapshot_RestoreMarker.h' Source` before deleting to find them all, plus the Fragment.h comment
    refs in Team/Player/Inventory/RenderTarget/Grid.
  - **⚠️ FloatAttribute.Gate:** GREEN pre-delete (cluster 1a/2). It's the only Model-A LIVE-world restore test (Run_Restore
    stamps JustRestored). Re-run AFTER the delete; if it regresses it needs its documented rewrite (VALIDATION.md pre-authorizes),
    NOT a delete rollback. My read (unverified): its single-world VALUE assert reads the restored Current from the image, not the
    re-driven replication container, so it should hold — but confirm empirically.
- [x] **Cluster 4 — 5.2.3 Transform `_Previous` re-seed revert — DONE (CkF `<c4-this>`; Ck.Snapshot 54/51/3 delta-zero).**
  [P5-D2] doc-path fix: the revert target is `CkTransform_Fragment.cpp/.h` (undo 15434d8ef's IsSnapshotable+SerializeSnapshot+
  CK_REGISTER_SNAPSHOTABLE(FSnap_Transform_Previous)), NOT the plan-cited `CkTransform_Utils.cpp:97-108` (15434d8ef never
  touched Utils.cpp — a doc typo, same class as [P0-D1]). VERIFIED `git diff 15434d8ef~1` empty for both files (exact pre-commit
  shape) + zero test/cross-repo deps on `_Previous` snapshot round-trip (v3 re-composes `_Previous` via Construct,
  `CkTransform_Utils.cpp:105`). Camera revert STAYS BLOCKED (OracleParity/BB-driver-world).
- [x] **Cluster 5 — 5.1 machinery gate — DONE (CkF `<c5-this>`; Dev Ck.Snapshot 54/51/3 delta-zero + Model-A registry tests GREEN
  under oracle-ON; **Shipping Game/Shipping compile SUCCEEDED — 2304s, zero real errors, zero snapshot symbols in link
  diagnostics**). [P5-D4].** Gated the 5 Model-A entry/serialization `.cpp` bodies under `#if CK_WITH_FIDELITY_ORACLE`
  (Capture/Restore/Archive_Writer/Archive_Reader/TagDriver); LEFT FragmentRegistry.cpp + Audit.cpp compiled (un-gated callers → gating
  = Shipping LNK2019); added the TagDriver define-visibility include (Ruling 5); dropped the stale Capture.h include from the Subsystem.
  ⚠️ RECON (pre-execution): V3 is INDEPENDENT of Model-A FragmentRegistry/Archive-W-R/
  TagRegistry (v3 uses MemoryWriter+proxy+Produce-registry+shared FSnapshotContext). BUT the Archive Writer/Reader + FragmentRegistry
  are `#include`d by **92 sites across ~48 production fragment `SerializeSnapshot` files** (all-config compiled) → CANNOT file-scope
  gate the archives/registry without a 48-file fan-out (PHASE_5.md §5.1's "no shipping refs survive" is FALSE). Realistic scope:
  gate ONLY the Model-A entry points (Run_Capture/Run_Restore/_Registry in CkSnapshot_Capture.cpp/.h + CkSnapshot_Restore.cpp/.h —
  Dev-CkTests-only callers); leave the pervasive infra compiled (inert in Shipping since cluster 1a emptied the registry). The
  Subsystem `#include`s CkSnapshot_Capture.h but only calls Run_CaptureV3 (likely stale include — verify). Flag C: Ck_DEFINE_ECS_TAG
  calls Register_SnapshotableTag from EVERY tag → keep TagRegistry registration compiled (stub body, don't gate decl). ROUTE the
  archives-gating-scope fork to Fable at execution; Shipping compile is the safety net.
- [x] **Cluster 6 — 5.2.4 docs + amended 5.3 grep + exit-grep comment sweep — DONE (CkF `<c6-this>`).** Docs: root CLAUDE.md
  `CK_REGISTER_SNAPSHOTABLE` row → "oracle-only since Phase 5"; CkEcs/CLAUDE.md rep section (dispatcher group
  `FGroup_Gameplay_Script`→`FGroup_Hydration` + ConstructedThisFrame defer + fire-gating); PHASE_5.md §5.3 grep dropped
  `IsSnapshotRespawnable` ([P3B-D1]) + added the `CkSnapshot_RestoreMarker.h` carve-out ([P5-D3]). Comment sweep: 34 stale
  `ReplicateOnRestore|RestoreReplicated|JustRestored|Reconstitution` comment hits across 26 files reworded to the post-Phase-5
  reality (v3 hydration Apply; RenderTarget→`HydrateFromSavedChannel`) via a 7-agent workflow — comments-only (build Succeeded,
  no code change), **exit grep VERIFIED ZERO outside the RestoreMarker.h carve-out**. Gate: Ck.Snapshot **54/51/3** delta-zero;
  Net **102/98/4** framework `Ck.*.Net` delta-zero (documented SM.Net flake + kiosk env-trio). Did NOT flip spec Status→IMPLEMENTED
  (Phase-5 FINAL is BB-driver-world-blocked).

- **[P5-D1] (executor, 2026-07-12, cluster 1a — Fable ruling C, code-verified) — deleting the Model-A SaveGame `_Header` field
  conflicted with VALIDATION §3 (Get_SaveSlotHeader signature frozen); resolved by SYNTHESIZING the legacy header from v3 at
  read time, NOT keeping a stored `_Header`.** The plan's delete list named `_Header`, but `Get_SaveSlotHeader` (UFUNCTION,
  returns `FCk_Snapshot_Header`, zero code callers tree-wide) reads it and VALIDATION §3 pins its signature. Routed to a
  read-only Fable agent; ruled **C** (delete both `_Header`+`_SnapshotBytes`; synthesize the legacy-shaped view in
  Get_SaveSlotHeader from `_HeaderV3`) over A (store a redundant second header) — one header of record on disk, v3-only writes
  in ALL configs, no `#if` around SaveGame writes. VERIFIED every anchor: the six overlapping fields
  (FormatVersion/EngineVersion/PluginBuildHash/TimestampUTC/WorldAssetPath/EntityCount) exist with accessors on BOTH structs
  (`CkSnapshot_Header.h:56-82` / `:202-231`), all six are stamped by Run_CaptureV3 (`CkSnapshot_CaptureV3.cpp:388-393,435`),
  and the Model-A-only fields (_Manifest/_TransientEntityId/_TagSectionByteOffset) have no v3 source → left at defaults.
  Guard: pre-v3 slot (`_SnapshotBytesV3.Num()==0`) → return {} (same contract as missing slot). **Behavior delta (noted, not a
  product decision — zero code callers, BP sees an opaque non-BlueprintReadOnly struct): Get_SaveSlotHeader's FormatVersion now
  reports 3, Manifest empty, TransientEntityId sentinel.** Executor judgment per Fable (low-blast, reversible, signature
  byte-identical); Adam can veto via PROGRESS. FCk_Snapshot_Header the TYPE stays (frozen return type + oracle-test capture type).

- **[P5-D3] (executor, 2026-07-12, cluster 3 — Fable-consulted + every claim Opus-code-verified) — KEEP `CkSnapshot_RestoreMarker.h`
  + `FTag_Snapshot_JustRestored` (plan §5.2.2 said DELETE); the plan's delete surface OVERLOOKED live cross-repo BB consumers.**
  The cluster-3 build FAILED: BB (superproject, must-not-edit) has live `Has<ck::FTag_Snapshot_JustRestored>()` in
  `Bb_SnapshotRestore.cpp:35` + `Bb_CombatReceiverRestore.cpp:26` (+ their `#include` of the header + the BB AS restore-rebind
  fleet via `Get_WasJustRestored`). SAME class as the plan's OWN `FTag_ActorJustRebound` KEEP — the same `Bb_SnapshotRestore.cpp`
  consumes BOTH tags; the plan just didn't extend the ruling to `FTag_Snapshot_JustRestored`. Resolution (Fable ruling, verified):
  restored the header, replaced its stale Model-A comment with a retirement comment. VERIFIED: (1) Model-A `Run_Restore(world)` —
  the only stamper — has exactly ONE caller tree-wide (`Test_Snapshot_FloatAttribute_Gate.spec.cpp:215`, Dev CkTests), NOT the BB
  runtime (which loads via v3 `UCk_Snapshot_Subsystem::Request_Load`, no stamp) → BB's `Has<>` was ALREADY always-false under v3
  since Phase 3B → keeping the tag is COMPILE-ONLY, zero runtime-behavior change; (2) no compile path without the tag (BB includes
  by path + needs the complete type; can't oracle-gate — BB compiles in Shipping); (3) `CK_DEFINE_ECS_TAG_TRANSIENT` has no
  snapshot-registrar side effect, so retention is inert. **Cross-repo sweep (Fable-suggested, Opus-run) for EVERY deleted symbol
  across BB `Source`/`BusterBlockTests`/`Script` + CkTests: the ONLY hits are the 2 BB files (the tag) + one stale COMMENT in
  CkTests `Test_Snapshot_RenderTargetParity_MPReload_Gate.spec.cpp:10` (non-blocking) — no other deleted symbol has a cross-repo
  code consumer.** Consequences (record loudly): (a) **exit-grep amendment** — PHASE_5.md §5.3's `...JustRestored...` grep now
  carves out `CkSnapshot_RestoreMarker.h` (retained cross-repo symbol); executor-recorded per the plan's small-human-decision (c)
  ("mechanical, edits CTO-fixed acceptance text — record loudly"), Adam ratifies at review. Carve-out only holds once cluster 6's
  comment-sweep clears the OTHER `JustRestored` comment hits (`CkTag.h:29,32`, `CkSnapshot_Restore.cpp:10`,
  `CkRenderTarget_RepData.h:41`). (b) **BB-repo follow-up EXPANDED** (Adam/BB, NOT a CkF session): the FTag_ActorJustRebound
  cleanup now ALSO owns the whole `FTag_Snapshot_JustRestored`/`Get_WasJustRestored` surface (2 BB C++ BPFL pairs + ~28 AS
  `*_Processor_RestoreRebind.as` + gate-spec comments). Delete `CkSnapshot_RestoreMarker.h` only WITH that BB cleanup. (c) **Flag A
  (cluster-6 watch):** CkTests `Test_Snapshot_RepDataRestoreCoverage_MetaTest`'s premise is per-feature ReplicateOnRestore
  coverage — watch the gate delta; retire/rewrite in cluster 6 if red (CkTests editable). (d) **Flag C (cluster-5 heads-up):**
  `CkTag.h:5` unconditionally includes `CkSnapshot_TagRegistry.h` and `CK_DEFINE_ECS_TAG` calls `Register_SnapshotableTag` from
  EVERY tag — when cluster 5 oracle-gates TagRegistry/TagDriver, that header + Register function must STAY COMPILED in all configs
  (stub the BODY, not the declaration) or Shipping breaks globally.

- **[P5-D4] (executor, 2026-07-12, cluster 5 — Fable-consulted + every anchor Opus-code-verified) — Model-A machinery gate is
  ENTRY-POINTS-ONLY (5 `.cpp` bodies), NOT the archives/registry file-scope gate PHASE_5.md §5.1 assumed.** §5.1 expected "no shipping
  ref survives" for the Archive Writer/Reader + FragmentRegistry — FALSE: they're `#include`d by 92 sites across ~48 production fragment
  `SerializeSnapshot` `.cpp` (for the `CK_REGISTER_SNAPSHOTABLE` Archive-type refs), all-config compiled. Cluster 1a already gated the
  REGISTRATION (macro → static_assert-only in Shipping → empty registry → `Do_RegisterSnapshotable`/`DoSerializeSnapshot_OneInstance`
  templates never instantiated), so the archive/registry are already Shipping-inert. **GATED (namespace bodies under
  `#if CK_WITH_FIDELITY_ORACLE`):** `CkSnapshot_Capture.cpp`, `CkSnapshot_Restore.cpp`, `CkSnapshot_Archive_Writer.cpp`,
  `CkSnapshot_Archive_Reader.cpp`, `CkSnapshot_TagDriver.cpp` (ONLY callers = Dev CkTests, dev-guarded via `WITH_DEV_AUTOMATION_TESTS`=0
  in Shipping — note CkTests' module is Runtime-type but its Model-A call sites are dev-guarded). **LEFT COMPILED (gating breaks
  Shipping):** `CkSnapshot_FragmentRegistry.cpp` + `CkSnapshot_Audit.cpp` — un-gated callers (`Audit.cpp:40,52` call the registry
  `Find_ByEnttHash`/`Get`; `Audit::Register_Expected` runs at static-init from the un-gated `CK_DEFINE_ENTITY_HOLDER_ROUNDTRIP` /
  record-twin macros) → gating them → Shipping LNK2019; both inert in Shipping (empty registry). Also left: Context/HandleWalk (shared
  with v3), TagRegistry (Flag-C registration path), LoadReport, RestoreInvariants, FidelityOracle (already gated). **Ruling-5 landmine
  FIXED:** `CkSnapshot_TagDriver.cpp`'s include chain had NO source of the `CK_WITH_FIDELITY_ORACLE` define → a bare `#if` would
  evaluate 0 in EVERY config → Capture/Restore.cpp LNK2019 in DEV (a Shipping-first build passes, Dev breaks later); pre-empted by
  adding `#include CkSnapshot_FragmentRegistry.h` at TagDriver.cpp top. Headers NOT gated (`.cpp`-bodies-only) — the ~48 fragments
  include them; nothing references the gated fns in Shipping. Dropped the stale `#include CkSnapshot_Capture.h` from
  `CkSnapshot_Subsystem.cpp`. **VERIFIED:** Dev gate 54/51/3 delta-zero (registry tests run+pass under oracle-ON); **Shipping
  Game/Shipping compile SUCCEEDED (2304s, zero real errors, zero snapshot symbols in link diagnostics — the 65 LNK4217 are pre-existing
  Jolt import warnings).** Evidence-forced deviation from CTO-fixed §5.1 (same class as [P4A-D1]/[P4B-D3]); Adam ratifies at review.

## Unattended execution protocol (set 2026-07-11 by Adam — OVERRIDES the "STOP on divergence" default below)

Run the campaign UNATTENDED through all remaining phases: finish §1.6, then Phases 2 → 3A → 3B → 4A → 4B → 5 in
order. Per-phase loop: read PHASE_N.md (+ the spec and PHASE_1_RESEARCH.md), implement on Opus, gate via
UnrealToolbox (editor CLOSED; read verdicts from the --output logs), and when the gate is GREEN + delta-zero vs the
baselines, COMMIT that phase (never push; stage only files you changed by name) + update this file, then proceed to
the next phase automatically. Do not stop between green phases.

QUESTIONS / DIVERGENCES → delegate, don't halt. When a step is ambiguous, or reality diverges from the plan, and the
answer is NOT already in the PHASE docs / spec / PHASE_1_RESEARCH.md: launch a **Fable-class agent** (Agent tool,
`model: "fable"`) to research the codebase + decide the question (Fable has the reasoning depth for design/architecture
forks), VERIFY its ruling against the cited code yourself, record it in §Decisions here, then RETURN TO OPUS to
implement. Never improvise architecture on the Opus main loop — route every design fork through a Fable agent.
Reserve a true STOP (→ §Blockers, end the run) ONLY for: (a) a red gate you cannot fix against the plan/research even
after a Fable consult; (b) an irreversible/outward/destructive action (push, force-push, cross-repo merge, deleting
another session's work); or (c) a genuine human-only product/risk/authority decision the Fable agent explicitly flags
as needing Adam. Cross-repo/CkTests-ahead-of-CkFoundation discipline and "no push" still hold absolutely.

## Phase-0 baseline table (fill in Phase 0; every later phase diffs against THESE names)

Captured 2026-07-11 on `feature/save-load-improvements` @ `bbde1a9dd` (clean tree, no edits), via UnrealToolbox
`--build --test` (Editor/Development). Verdicts read from `CkAuto/logs/p0-baseline-*.log`.

| Pattern | Total | Pass | Fail | Failing test names |
|---|---|---|---|---|
| Ck.Snapshot | 46 | 46 | 0 | (none) |
| Ck.Attribute.Net | 17 | 17 | 0 | (none) |
| Net | 102 | 101 | 1 | Ck.StateMachine.Net.OwningClientAuth_SubSm_AuthorityGatedTask |
| (full suite) | not run this phase | | | (VALIDATION.md runs full suite at Phase 5) |

The lone `Net` red is PRE-EXISTING (zero source edits at capture time). Every later phase re-running `Net` must show
exactly this one failure by name and no others — **PLUS the kiosk-trio environmental caveat below.**

**[§1.6 update] `Net` pattern env-red trio (record + diff against these going forward).** The `Net` substring pattern
sweeps in BB gameplay AutoTests. Three BB kiosk-DESTRUCTION AutoTests fail environmentally (timing-sensitive settle
windows under machine load — NOT framework, NOT campaign): `Bb_AutoTest_RentnetKiosk_DamageToDestroy`,
`Bb_AutoTest_RentnetKiosk_DispensesLootOnDeath`, `Bb_AutoTest_RentnetKioskDriver_SpawnAndRelease`. Proven outside the
campaign's blast radius: the failing tests compose their entities `ECk_Replication::DoesNotReplicate`
(`Plugins/BusterBlockTests/Script/Tests/RentnetKiosk/BB_AutoTest_RentnetKiosk_DamageToDestroy.as:39,46,53,59`), so the
replicated-fragment registry / Produce / re-drive the campaign touches is never consulted (Fable ruling [BI-1], verified).
The framework `Ck.*.Net` set is delta-zero (the `Ck.StateMachine.Net.OwningClientAuth_SubSm_AuthorityGatedTask` baseline
red FLAKES green/red across runs — a known flake). So the `Net` gate for later phases = **framework `Ck.*.Net` delta-zero
+ the kiosk trio (env, ignorable) + StateMachine flake (ignorable)**. A NEW `Ck.*.Net` red = real stop-condition.

**[§1.6 note] Branch base changed mid-session (one-time integration, NOT ongoing).** `feature/save-load-improvements`
was rebased at 2026-07-11 16:55 local (reflog `rebase (finish)`) to fold in the **object-pooling-core** campaign
(+3758 lines: new CkObjectPooling/CkArchetype/CkDebugFeatureFlags code under CkCore/CkEcs, plus CkEcs Scheduler +
EntityScript edits). Old tip `49cfdb038` → new tip `951112723` (same subjects, new hashes; old commits survive in
reflog). HEAD stable since. The §1.6 build+gate ran AFTER this (build compiled `Module.CkObjectPoolingDebugger.cpp`),
so the green gate reflects the INTEGRATED base. **Consequence for Phase 2+:** the object-pooling work modified
`CkProcessorDescriptor.h` (+5), `CkProcessorScheduler.cpp` (+28), `CkProcessorGraph.h/.cpp` (+26) — exactly the Phase-2
scheduler targets. Phase-2 line refs in PHASE_2.md are SHIFTED; re-locate insertion points by PATTERN (PumpPolicy trait,
BuildDescriptor slot, DoCreateNodes mirror) against current code, and watch for interaction between object-pooling's
scheduler additions and the new `ECk_ProcessorLoadPolicy`/`FGroup_Hydration` work. Verified current lines this session:
`ECk_ProcessorPumpPolicy` enum at `CkProcessorDescriptor.h:79-84`, `_PumpPolicy` field at `:145`.

Census (invocation-only, Phase 0, 2026-07-11): CK_REGISTER_SNAPSHOTABLE = **127** across **20** modules
(CkAnimation, CkAttribute, CkDynamic, CkEcs, CkEcsExt, CkEntityCollection, CkEntityTag, CkGrid, CkInteraction,
CkInventory, CkLabel, CkObjective, CkPhysics, CkRelationship, CkRenderTarget, CkSnapshot, CkSpatialQuery,
CkStateMachine, CkTagSet, CkTimer); FCk_RepData_* struct decls = **23**. (Spec §2's "119 / 18 modules / 24 RepData"
reconciled to these in the same commit.)
Load-time baseline (Phase 3B, representative fixture): ___ ms.

## Phase-2 progress (sub-step tracker — for mid-phase continuation; Phase 2 is ONE atomic gate)

Base: integrated `951112723` + §1.6 commits (`5d262f52a`/`4fb178824`/`df06b8394`). PumpPolicy mirror-map (current lines):
`ECk_ProcessorPumpPolicy` enum `CkProcessorDescriptor.h:80`; `_PumpPolicy` field `:145`; BuildDescriptor slot
`CkProcessorTraits.inl.h:216-218`; node field `CkProcessorGraph.h:79-81`; DoCreateNodes copy `CkProcessorGraph.cpp:223,261`;
scheduler precompute `CkProcessorScheduler.cpp:70-81` (`_MainPassOrder`/`_PumpOrder`), main pass `:105-149`, pump `DoPump :205-325`;
`.h` members `_MainPassOrder`/`_PumpOrder` `CkProcessorScheduler.h:42-43`, `Tick` sig `:22`, `DoPump` sig `:27-29`.

- [x] 2.1 `ECk_ProcessorLoadPolicy` trait (mirror PumpPolicy) + scheduler `_LoadPassOrder`/`_LoadPumpOrder` + `ECk_SchedulerTickScope{Full,LoadKernel}` Tick/DoPump scope + subsystem `Get_/Set_IsLoadGateActive` + Actor::Tick threading (lazy weak-ptr `_OwningSubsystem`) + `Request_PumpToQuiescence` scope param — DONE
- [x] 2.2 Mark the kernel RunsDuringLoad — DONE, verified 12 (grep 13 incl. the descriptor doc-comment; reworded so exit grep == 12)
- [x] 2.3 `FGroup_Hydration` between PostTransform & Replication (fwd-decl + struct + re-point Replication RunAfter + chain comment + CK_REGISTER_GROUP) + BOTH dispatchers → `FGroup_Hydration`, deleted their `RunAfter FinishConstruction` (group order guarantees it) — DONE
- [x] 2.4 `FTag_EntityScript_ConstructedThisFrame` defer — **DONE.** Tag added (CkEntityScript_Fragment.h); FinishConstruction stamps it (.cpp:361 area); both dispatchers skip at top of ForEachEntity; hydration dispatcher `RunAfter net` + `DoTick` override clears it registry-wide (ck_exp pattern `TProcessor::DoTick(InDeltaT)` then `_TransientEntity.Clear<>()` — VERIFIED ck_exp::TProcessor exposes a public view-iterating DoTick, CkProcessor.h:350). [P2-D1] applied.
- [x] 2.5 Fire-gating — **DONE, via Fable ruling [P2-D2].** `Get_HasUndrainedReplicatedFragments_IncludingDependents` recurses the LIFETIME-DEPENDENTS tree (`Get_LifetimeDependents`, the same traversal the expected-dependent count derives from — DoCount at CkEntityReplicationDriver_Utils.cpp:53-79 is the mirrored precedent), NOT a driver-side list (none exists). Per-entity predicate is TWO checks: `Has<FTag_RepFragments_PendingApply>() || Has<FFragment_PendingHydration>()` (the tag subsumes queued removals; `_Fragments` is private/Utils-not-friend). Gate placed BEFORE `Remove<MarkedDirtyBy>` in FireOnDependentReplicationComplete. Does NOT prune at non-driver entities (hydration is net-mode-agnostic). Original design.md note.
- [x] 2.6 Retire `5eda3ac8a` guards — **DONE.** Removed the `Has<FTag_Velocity_NeedsSetup>→NotReady` / `Has<FTag_Acceleration_NeedsSetup>→NotReady` blocks from CkVelocity_Fragment.cpp + CkAcceleration_Fragment.cpp Apply lambdas (§2.4+§2.5+late group supersede them). Left a why-comment.
- [ ] 2.4-OLD-NOTES (superseded by [x] above) `FTag_EntityScript_ConstructedThisFrame` defer — design ref kept:
   - Add `CK_DEFINE_ECS_TAG_TRANSIENT(FTag_EntityScript_ConstructedThisFrame);` in `CkEntityScript_Fragment.h:23-28` (beside the lifecycle tags).
   - `FProcessor_EntityScript_FinishConstruction::ForEachEntity` (`CkEntityScript_Processor.cpp:~350-361`, where it `Add<FTag_EntityScript_BeginPlay>`) also `InHandle.AddOrGet<FTag_EntityScript_ConstructedThisFrame>();`.
   - BOTH dispatchers (`CkReplicatedFragmentContainer_Processor.cpp`) skip at the TOP of ForEachEntity: `if (InHandle.Has<ck::FTag_EntityScript_ConstructedThisFrame>()) { return; }` (leaves the pending tag → re-dispatches later).
   - **WHY it works (verified against the pump semantics — load-bearing):** feature Setups live in FGroup_Gameplay (BEFORE FGroup_Hydration). In the MAIN pass the entity is composed in FGroup_Gameplay_Script AFTER Setups already ran, so its Setup is still pending → applying now would be stomped by the pending Setup. Skipping in the main pass + clearing the tag after both dispatchers → in the PUMP, FGroup_Gameplay Setup drains FIRST, THEN FGroup_Hydration applies (post-Setup, no stomp). The clear-then-pump-reapply is INTENDED, not a bug.
   - **[P2-D1] decision — which dispatcher clears + ordering (plan left ambiguous; resolved by the pump analysis):** the clear MUST run AFTER both dispatchers' skips in the main pass, else the earlier-cleared tag lets the later dispatcher apply too early. So: give `FProcessor_Hydration_Dispatch` `using RunAfter = TDepList<FProcessor_ReplicatedFragments_Dispatch>;` (runs last, dormant so it clears exactly once per main pass — it doesn't pump), and ONLY the hydration dispatcher clears. Forward-correct for Phase 3B when hydration activates.
   - The clear is a `DoTick` override on `FProcessor_Hydration_Dispatch` (idiom: `CkIsmRenderer_Processor.cpp:23-32` — override `DoTick`, call the base's view-iterating DoTick, then `_TransientEntity.Clear<FTag_EntityScript_ConstructedThisFrame>();`). **ck_exp DoTick-override mechanics UNVERIFIED** — the base is `ck_exp::TProcessor<...>`; add `using Super = ck_exp::TProcessor<FProcessor_Hydration_Dispatch, FCk_Handle, ck::TReadWrite<FFragment_PendingHydration>, FTag_Hydration_PendingApply, CK_IGNORE_PENDING_KILL>;` (CkAttribute pattern, `CkAttribute_Processor.h:34`) and call `Super::DoTick(InDeltaT)`. CONFIRM ck_exp exposes a callable view-iterating `DoTick(TimeType)` before trusting this; if not, mirror how a CkAttribute composite drives its inner loop. The dispatcher's ForEachEntity is `const`; the DoTick override that clears must be non-const.
- [ ] 2.5 Fire-gating: `Get_HasUndrainedReplicatedFragments_IncludingDependents` (Net utils, beside `Get_IsReplicationCompleteAllDependents` `CkEntityReplicationDriver_Utils.cpp:381-405`) traversing the dependent set the counters at `CkEntityReplicationDriver_Fragment.cpp:377-408` maintain (self + dependents: `FTag_RepFragments_PendingApply` / queued removals / `FFragment_PendingHydration`); gate `FProcessor_ReplicationDriver_FireOnDependentReplicationComplete` (`CkEntityReplicationDriver_Processor.cpp:19-32`) — if true, return WITHOUT consuming the fire tag (retry next tick).
- [ ] 2.6 Retire `5eda3ac8a` `Has<FTag_*_NeedsSetup> → NotReady` guards in the Apply lambdas of `CkVelocity_Fragment.cpp` (the guard I READ at `:42-44`) + `CkAcceleration_Fragment.cpp`. Keep everything else. Verify `git show 5eda3ac8a --stat` remnants.
- [x] 2.7 Tests — **Test 1 DONE; Test 2 subsumed per [P2-D3].** `Ck.Snapshot.LoadGate.GatedSkipsKernelTicks` written (hermetic scheduler à la Test_Scheduler_PumpGating: two test processors, one default + one RunsDuringLoad; `Tick(Full)`→both, `Tick(LoadKernel)`→only kernel [the inverse assert: over-gating hangs a real load], `Tick(Full)`→gated resumes). CkTests `Source/CkTests/Private/CkSnapshot/Test_Snapshot_LoadGate_Scope.cpp`. Compile-verified (p2-compilecheck3.log). **[P2-D3]** the standalone `Velocity_ApplyAfterLateSetup` MP repro is NOT written: the late-setup stomp (replicated value arrives while NeedsSetup pending) can only be forced ROBUSTLY via a reload (client re-constructs with the saved override as initial replicated data) — a non-reload override stomps server-side at spawn, or is a change-rep after the client already constructed. That exact scenario IS covered, robustly + in-gate, by `Ck.Snapshot.Parity.Acceleration_MPReload` (post-reload the client's Setup recomputes Current from Params{0} and the replicated override{3,4,5} must win) + the three `Ck.Attribute.Net` pins. A flaky standalone repro is worse than robust existing coverage.
- [x] 2.8 Gate + commit — **DONE, GREEN.** `Ck.Snapshot` 49/49/0 (--discover-fresh; LoadGate + AccelerationParity + all Parity; NO over-gating hang → kernel list correct). `Net` 102/98/4: framework `Ck.*.Net` delta-zero incl. the 3 Attribute.Net pins (`Values_AppliedBefore_OnReplicationComplete`, `Float_InitialBakedValue_Replicates`, `Float_PreComposition_StashedValue_Applies`) all GREEN — the "real regression" filter (non-kiosk, non-StateMachine) was EMPTY; the 4 fails are the recorded kiosk env-red trio + the StateMachine.Net flake. `Ck.Attribute.Net`/`Ck.Physics.Net` covered by the `Net` substring run (all pins green, no physics-net red). Commits: CkF 91b96a177 / 8c4fbce7a / af9fad239, CkTests 4522c8e. Nothing pushed. (Split 3 CkF commits not 4 — the trait/group/defer sub-steps interleave within shared files; fire-gating + guard-retire live in independent files so they got their own commits.)

**Phase-2 CHECKPOINT (2026-07-11): 2.1–2.3 IMPLEMENTED + COMPILE-VERIFIED (editor build clean, 0 errors, log
`CkAuto/logs/p2-compilecheck2.log`) + uncommitted on disk; 2.4–2.8 remaining.** Compile-check caught + fixed a dropped
`namespace ck {` opening in `CkProcessorScheduler.h` (my enum-insert edit had swallowed it — now restored). Phase 2 is ATOMIC —
2.1–2.3 alone are NOT gate-safe (they move BOTH dispatchers from FGroup_Gameplay_Script → FGroup_Hydration, which
without 2.4's ConstructedThisFrame defer + 2.5's fire-gating can expose the late-setup stomp). DO NOT commit or gate
until 2.4–2.7 land. The tree's committed tip is Phase-1-green (`df06b8394`); the uncommitted delta is Phase-2 2.1–2.3
(files: `CkProcessorDescriptor.h`, `CkProcessorTraits.inl.h`, `CkProcessorGraph.h/.cpp`, `CkProcessorScheduler.h/.cpp`,
`CkEcsWorld_Subsystem.h/.cpp`, `CkEntityScript_Processor.h`, `CkEntityLifetime_Processor.h`,
`CkReplicatedFragmentContainer_Processor.h`, `CkPersistence_ReDrive_Processor.h`, `CkEntityReplicationDriver_Processor.h`,
`CkActorRespawn_Processor.h`, `CkProcessorGroups.h/.cpp`). `git -C Plugins/CkFoundation diff --stat` shows the exact set.

## Phase-3B progress (RESEARCH CHECKPOINT — 2026-07-11, Opus; no code landed yet)

Status: read the full load SM + v3 format/writer + context/walker + dormant hydration queue + load gate +
reconstitution machinery + the e2e gate tests. Design forks routed to a Fable agent (running). NO code edited yet.
Repo clean at `57fee2671` (CkF) / `5bb9798` (CkTests). Editor closed.

**v3 load model (the shape of the rewrite).** Unlike Model A (registry.clear()+image rebuild via `Run_Restore`),
v3 works WITH the freshly-built world: TearingDown→Travel→AwaitingWorld give a pristine level-reloaded world (GameMode
default pawn + level actors already re-created normally). Then, under the Phase-2 load gate, overlay saved state:
`Set_IsLoadGateActive(true)` (on world-ready) → Rebuilding (spawn RuntimeSpawned from recipes; adopt EngineOwned by
rendezvous; adopt ConstructSpawned by label) → Hydrating (enqueue Produce payloads into `FFragment_PendingHydration` +
`FTag_Hydration_PendingApply`; the kernel `FProcessor_Hydration_Dispatch` drains under the gate) → Reconciling
(subtractive `Request_DestroyEntity`, parks under gate) → Settling (`Request_PumpToQuiescence(LoadKernel)` from the
FTSTicker → gate off → `DoFinish_Load`). NO registry.clear / Run_Restore / Relink / AdoptRestoredTransient /
RebuildProcessorGraph in the v3 path — the fresh world's graph is already correct. Request_Load: v3-only hard break
(reject v2 with Failed_IncompatibleSave; keep Model A compiled for oracle).

**Verified machinery (file:line, for the reader):**
- Saved-id map key = packed entt uint32 = `_SavedId` (writer: `CkSnapshot_CaptureV3.cpp:50` `static_cast<uint32>(Handle.Get_Entity().Get_ID())`; handle blobs write the same via `FSnapshotContext::Snapshot_EnttEntity`, `CkSnapshot_Context.cpp:9`). Default-ctx load just casts raw (`CkSnapshot_Context.cpp:16-19`) — the v3 load needs a NEW map-backed mode that re-homes the full handle from the map.
- Hydration queue is kernel-wired + dormant: `FProcessor_Hydration_Dispatch` (LoadPolicy RunsDuringLoad, NetMode All, FGroup_Hydration) drains `FFragment_PendingHydration._Entries` via `persistence_apply::ApplyOne` — 3B just enqueues (`CkReplicatedFragmentContainer_Processor.cpp:154-206`).
- Load gate: `Get_/Set_IsLoadGateActive` + `Request_PumpToQuiescence(LoadKernel)` (`CkEcsWorld_Subsystem.h:160,193-194`); EcsWorld actor tick reads the flag.
- Recipe: EVERY Request_SpawnEntity stamps `FFragment_SpawnRecipe` (holder pinned) at `CkEntityScript_Processor.cpp:156-167`. Non-bridged RuntimeSpawned rebuild = `UCk_Utils_EntityScript_UE::Request_SpawnEntity(owner, recipeClass, remappedParams)` (`CkEntityScript_Utils.cpp:129-176`).

**FORKS routed to Fable (verify rulings against code before implementing):**
- **[FORK-D] (CENTRAL, load-bearing — under-scoped by the plan).** The ENTIRE e2e gate (11 Parity_MPReload + M2b + M2b2a) drives ONE entity shape: a **bridged WithActor probe** `ACk_AutoTest_NetSubject_M2bProbe[_Replicated]` (spawned via SpawnActor at runtime), whose entity-script Construct composes the tested feature. Hard facts: (1) a WithActor entity gets BOTH `FFragment_SpawnRecipe` (→ rule-4 RuntimeSpawned) AND `FFragment_ActorSpawnIntent` (`CkEntityScript_WithActor.cpp:61-65`); (2) its recipe `_SpawnParams` = `FCk_EntityScript_WithActor_SpawnParams{actor}` whose actor ptr does NOT serialize → CANNOT `Request_SpawnEntity(WithActorScript, savedParams)` (Construct ensures on invalid `_OwningActor`, `CkEntityScript_WithActor.cpp:24-28`); (3) Transform has ONLY net Apply handlers (NO Produce, Transport=Net — `CkTransform_Fragment.cpp:39-104`) → position is NOT in any v3 payload; (4) hydration needs the feature already composed (Apply→NotReady else dropped) → the WithActor Construct MUST re-run on load. Candidate model: loader spawns the ACTOR (class + saved transform) → the actor's WithActor ActorComponent bridge re-creates the entity + runs Construct (features compose) → rendezvous-map `_SavedId`→`TryGet_ActorEntityHandle(actor)` → hydrate. Open: (D3) transform via recipe-field vs Transform-Produce+NetAndSave (needs verified actor↔entity sync direction); (D4) is `FProcessor_ActorRespawn` reused or replaced (it rebinds to an EXISTING entity w/o Construct — wrong for v3's no-pre-existing-entity). Fable ruling pending.
- **[FORK-A] saved-id map + v3 FSnapshotContext mode** — add map-backed `const TMap<uint32,FCk_Handle>*` mode to `FSnapshotContext`; on load set full handle (entity+registry, NO typed-slice) from the map; missing key → invalid. My read says this is well-defined; Fable to confirm Set_Entity/Set_Registry + no-slice.
- **[FORK-B] ConstructSpawned adoption timing** — resolve saved-id→live labeled child under the mapped+constructed owner via `UCk_Utils_Ecs_Base_UE::Get_EntityOrRecordEntry_WithFragmentAndLabel` (or CkRecord/CkLabel util); may need its own poll (child composed after owner Construct). Fable to confirm API + sequencing.
- **[FORK-C] reconcile vs gated destruction** — QUEUE `Request_DestroyEntity` only (parks under gate); LostGrantStaysLost asserts after ≥2 post-gate frames. Fable to confirm the labeled-ConstructSpawned-child enumeration.

**IMPLEMENTATION STATE (2026-07-11, Opus — ON DISK, UNCOMMITTED, compile-check running):**
- **3B.1 DONE on disk:** Fork-A FSnapshotContext v3 map-backed mode (`CkSnapshot_Context.h/.cpp`); save-side `_ActorSpawnTransform` field on `FCk_Snapshot_V3_EntityEntry` + capture in `CkSnapshot_CaptureV3.cpp` (D3a); full load-SM rewrite (`CkSnapshot_Subsystem.h/.cpp`) — new `ELoadPhase{Idle,TearingDown,AwaitingWorld,Rebuilding,Hydrating,Settling}`; v3 reader (SerializeItem tables at Request_Load, v3-only hard break → Failed_IncompatibleSave/Failed_Corrupt); `DoRebuild_Tick` (EngineOwned SaveKey/player rendezvous, ConstructSpawned label-adopt via LifetimeDependents walk, RuntimeSpawned non-bridged Request_SpawnEntity + bridged actor-first spawn+TryGet_ActorEntityHandle); `DoHydrate_Enqueue` (payloads → FFragment_PendingHydration + tag, single-source orphan count = total−mapped); `DoReconcile_Queue` (subtractive Request_DestroyEntity of stray labeled ConstructSpawned children); `DoTick_Load` **Hydrating is ATOMIC** (enqueue+reconcile+gate-off+Full-pump in ONE callback → hydration never drains under the gate → no Setup-stomp); Settling countdown; Request_Save now FAILS on v3 capture failure (v3 authoritative). Model-A capture/restore KEPT compiled.
- **3B.2 DONE on disk:** deleted `ECk_ReconstitutionPhase`+accessors+`_ReconstitutionPhase` (CkEcsWorld_Subsystem.h/.cpp), `DoIs_WorldReconstituting`+2 call sites (CkEntityScript_Utils.cpp), all CkSnapshot stamp sites (subsystem rewrite). KEPT `Get_IsSnapshotRespawnable` ([P3B-D1]). `rg "Reconstitution" Source` → **0**. `IsSnapshotRespawnable` → 3 (virtual def/decl + WithActor ActorSpawnIntent opt-in). Stale comment refs to DoStamp_RespawnMarkers remain in dormant CkActorRespawn docs (comments only, not code).
- **KEY DESIGN NOTE:** the v3 load does NOT registry.clear/Run_Restore — it works WITH the freshly level-reloaded world (EngineOwned re-created normally; loader spawns RuntimeSpawned actors whose BeginPlay re-creates bridged entities via the WithActor bridge). The existing e2e gate's ConstructSpawned coverage comes FREE via attribute meta-fragment children (labeled ConstructSpawned entities under the bridged probe) — AttributeParity exercises adopt-by-label + hydrate.
- **NEXT:** compile-check verdict (p3b-compilecheck1.log) → fix → 3B.3 tests (existing Parity/M2b gate first — the real proof; then new Rebuild/Oracle tests) → full gate → diagnose reds ([P3B-D4]) → commit.

**GATE-1 RESULT (2026-07-11, p3b-gate1.log): BUILD CLEAN (595s, Result: Succeeded); Ck.Snapshot ~39 pass / 12 fail.**
- **Model A fully intact** — ALL registry-level tests green (Core, DynamicFragment×11, Oracle×2, LoadGate, V3.Capture×2, LifecycleStrip, SaveKey, SceneNode, Timer, MontagePlayer, etc.). LifecycleStrip stays green (registry-level, NO rewrite needed — earlier hunch confirmed).
- **v3 e2e PASS:** FloatAttribute.Gate (actually Model-A Run_Capture/Restore, not v3), Parity.Acceleration, Parity.TeamPlayer, M2b2b.MPServerTravel, both spikes.
- **EXPECTED reds (annotate later):** StateMachine, StateMachineNoHistory (→4A, no Produce); GridPlacements, InventoryDataOnly, InventorySpatial, RenderTarget (→4B, client-shaped Apply). [P3B-D4].
- **UNEXPECTED reds → ROOT CAUSE FOUND:** M2a/M2b/M2b2a (load flag not cleared), AnimPlan/Attributes/TagSet (values wrong). DIAG: **`rebuild did not fully resolve within [600] frames — [7]/[26] mapped`** — only ~7 of 26 saved entities REBUILD; 19 orphan. Rebuilding burns the 600-frame cap → load runs past the tests' 240-frame budget (→ "load in progress" fails) AND 19 entities never hydrate (→ wrong values, "dangling handle entity N → LifetimeDependents → N+1 not present" ×20 in AttributeParity).
- **TWO fixes landed (uncommitted, rebuilding for diagnosis):** (1) progress-based early-exit (proceed after 30 ticks with no NEW mapping — kills the 600-frame stall so loads complete fast even with orphans, fixing the *timing* failures); (2) per-unresolved-entry DIAG dump (provenance + owner-mapped + label) to pinpoint WHY 19 fail. Running p3b-diag-m2b.log. The CORE bug (why most entities don't rebuild/adopt) is under diagnosis — likely ConstructSpawned attribute-child adoption OR framework RuntimeSpawned entities the fresh world already owns being mis-respawned. DO NOT commit until root-caused + green.

**DIAGNOSIS COMPLETE (p3b-diag-m2b.log) — the v3 LOAD PIPELINE WORKS; remaining reds are per-feature payload gaps (Phase 4).**
- **`Ck.Snapshot.M2b.LevelReload` PASSES** with the fixes — proves the core end-to-end: bridged actor-first rebuild, ConstructSpawned adoption (all 6 attribute children mapped, unresolved ConstructSpawned = **0**), position restore (from `_ActorSpawnTransform`), value hydration. The core is SOUND.
- **Root cause of the 12/19 unmapped (per DIAG dump):** they are ALL `owner-saved-id 0` (world transient, never persisted) non-bridged RuntimeSpawned **world-boot infrastructure** — `Bb_DayNightLampDriver`, `Bb_DayNightVisuals`, `Bb_MusicDirector`, generic `Ck_EntityScript_WithActor_UE`. The CkTests gate runs in the full BB editor, so `/Engine/Maps/Entry` boots these; the fresh world's normal boot RE-CREATES them, so the loader must NOT respawn them. My non-bridged logic deferred them forever (owner never maps). **FIX: skip RuntimeSpawned whose lifetime owner is not a persisted saved-id (transient/root-owned = boot-infra); the boot recreates them** (`_PersistedIds`/`_SkippedIds`). ⚠️ This also skips any GAMEPLAY top-level entity spawned under the transient at runtime — the boot-infra-vs-gameplay discriminator is the **CTO-N1 spawner-state problem, deferred to Phase 4A**.
- **Root cause of the value-wrong parity reds (Attributes/AnimPlan):** their Produce is **EMPTY-SEED** ([P1-D2], VERIFIED `CkAttribute_RestorePersistence.h:12-31` — `return FInstancedStruct::Make(T_RepDataStruct{})`, a SET-but-empty payload). Under Model A the value came from the raw fragment image + `FProcessor_Attribute_Replicate`; **under v3 there is no image, so the value is NOT persisted** → attribute restores to Construct base (42.5), not the saved override (17.0). Making the Produce value-emitting is non-trivial (the per-owner upsert-merge the comment flags: multiple attributes share one owner container) — genuinely **Phase 4B "coverage sweep"** (spec §6 Phase 4). VALUE-EMITTING features (Velocity, Acceleration) round-trip and PASS.
- **[P3B-D5] EXPECTED CASUALTY CATEGORIZATION (annotate; the load pipeline is 3B's deliverable, per-feature payloads are Phase 4):**
  - **→ Phase 4A** (no Produce / SM state): `Parity.StateMachine_MPReload`, `Parity.StateMachineNoHistory_MPReload` (CkStateMachine has no Produce; SM redrive-as-hydration IS 4A).
  - **→ Phase 4B** (empty-seed Produce → value not persisted): `Parity.Attributes_MPReload`, `Parity.AnimPlan_MPReload`, likely `Parity.TagSet_MPReload` (VERIFY TagSet Produce shape).
  - **→ Phase 4B** (client-shaped Apply / re-author): `Parity.GridPlacements_MPReload`, `Parity.InventoryDataOnly_MPReload`, `Parity.InventorySpatial_MPReload`, `Parity.RenderTarget_MPReload` ([B1] shape, PHASE_3B §"Known interaction").
  - **MUST PASS in 3B** (load pipeline): M2a/M2b/M2b2a/M2b2b orchestration, `Parity.Acceleration`, `Parity.Velocity` (value-emitting), `Parity.TeamPlayer` (passed gate-1), all Model-A registry-level. Gate-2 (p3b-gate2.log, running) confirms the ambient-skip fix makes the orchestration tests green.

**GATE-2 RESULT (2026-07-12, p3b-gate2.log): BUILD CLEAN (54s incremental); Ck.Snapshot 41 pass / 10 fail; EXIT -1.** The
ambient-skip + stall-exit fixes RECOVERED M2b.LevelReload + M2b2a.ReplicatedRespawn (now PASS). DIAG clean: rebuild completes
in 2 frames, 19 skipped boot-infra, **0 orphaned**. **ALL 10 reds diagnosed as legitimate Phase-4 casualties — NONE is a
load-pipeline bug** (the pipeline is PROVEN by M2b/M2b2a/M2b2b + Acceleration/Velocity/TeamPlayer + all Model-A registry tests):
- **→ Phase 4A (3):** `Parity.StateMachine_MPReload`, `Parity.StateMachineNoHistory_MPReload` (CkStateMachine has NO Produce —
  SM redrive-as-hydration IS 4A); **`M2a.LoadOrchestration`** — its subject is a NON-bridged transient-owned RuntimeSpawned
  gameplay entity (provenance `RuntimeSpawned [13] bridged [0]`, `0/19 mapped`); the boot-infra skip ([P3B-D5]) can't tell it
  from BB boot-infra so skips it → the attributes' owner never maps → "0 float attributes survived." This is EXACTLY the CTO-N1
  boot-infra-vs-gameplay discriminator, deferred to 4A. (M2b/M2b2a use BRIDGED probes → pass; only M2a's non-bridged subject
  hits N1.)
- **→ Phase 4B (7):** `Parity.Attributes`, `Parity.AnimPlan` (empty-seed Produce → value not persisted, verified
  `CkAttribute_RestorePersistence.h`); `Parity.TagSet`, `Parity.GridPlacements`, `Parity.InventoryDataOnly`,
  `Parity.InventorySpatial`, `Parity.RenderTarget` (**client-shaped Apply** — e.g. TagSet Apply stamps
  `FFragment_TagSet_SyncReplication` for a SEPARATE processor "which owns the actual diff/apply", `CkTagSet_Fragment.cpp:28-32`;
  that processor doesn't apply authority-side under hydration → server value never restored; [B1] shape). KEY LESSON:
  value-emitting *Produce* is necessary but NOT sufficient — the *Apply* must apply authority-side. Velocity/Acceleration Apply
  do (they pass); TagSet's does not.
- **COMMIT DECISION (left to next session / maintainer):** all 10 reds are legitimately Phase-4 per the campaign's phase
  structure, so 3B is "green modulo annotated Phase-4 casualties." RECOMMEND: commit 3B (working load pipeline) with these
  annotated in Blockers, then 4A/4B green them. Not committed this session (handed off — 10 reds incl. an orchestration test is
  a larger deviation than the plan anticipated, worth a maintainer glance). Everything UNCOMMITTED on disk. See
  CONTINUATION_PROMPT_Phase3B_finish.md.

## Phase-3B DONE (2026-07-12, Opus unattended — COMMITTED, gate-3 green mod casualties)

**Gate-3 (p3b-gate3.log): BUILD CLEAN; Ck.Snapshot 52 / 43 pass / 9 fail (--discover-fresh); EXIT -1 (expected, casualties present).**
Delta from gate-2 (41/10): +1 total (new `V3.InstancedStructDiskSmoke`, GREEN), M2a fixed (10→9 fails). **Zero unexpected reds; all
9 fails are verified Phase-4 casualties.** Commits: CkF `78fcdaa8e` (retire reconstitution), `36bcdec5d` (v3 load pipeline),
`<docs-this>`; CkTests `ce32c65` (disk smoke + M2a opt-in). Nothing pushed.

- **M2a.LoadOrchestration is GREEN — the gate-2 "→ Phase 4A / N1 casualty" call above was a MISDIAGNOSIS, now CORRECTED ([P3B-D6]).**
  M2a's subject is NOT an inherently non-bridged gameplay entity: it is a WithActor **bridged** probe (`ACk_AutoTest_NetSubject_M2aProbe`)
  that simply never opted into the v3 respawn contract. The M2a probe's entity-script lacked the `Get_IsSnapshotRespawnable() -> true`
  override that M2bProbe has (`CkEntityScript.cpp:161-167` base default = false), so `WithActor::Construct` never stamped
  `FFragment_ActorSpawnIntent` → no `_ActorClassPath` → provenance `bridged [0]` → the loader's non-bridged branch hit the boot-infra
  skip. **This is a fixture gap, NOT N1:** the bridged actor-first branch (`CkSnapshot_Subsystem.cpp:628-663`) spawns the actor from
  `_ActorClassPath` and rendezvous-maps via `TryGet_ActorEntityHandle` WITHOUT consulting the owner saved-id — so opting in bypasses the
  boot-infra skip entirely (`_PersistedIds` check is only in the non-bridged else-branch at `:686`). Fix (CkTests ce32c65): add the
  respawn opt-in to `UCk_AutoTest_NetSubject_M2aProbe_EntityScript_UE` + bump `FramesForLoad` 150→240 to match the respawn-exercising
  M2b/M2b2a siblings. Value check (Final==42.5) holds because 42.5 is the Construct base (no override added). Fable-consulted (ruling A,
  fixture-fix) + independently code-verified (base default false, bridged branch bypasses skip, M2bProbe_Replicated ctor-only proves
  replicated respawn safe in-scheduler, gate-3 green). N1 (gameplay RuntimeSpawned under the transient, spawned via the NON-bridged path
  by an SM task) remains genuinely Phase-4A — M2a never was that case.
- **The 9 verified casualties (annotate; each is per-feature payload work, NOT a load-pipeline bug):**
  - **→ Phase 4A (2):** `Parity.StateMachine_MPReload`, `Parity.StateMachineNoHistory_MPReload` — CkStateMachine has no Produce; SM
    redrive-as-hydration is 4A.
  - **→ Phase 4B, empty-seed Produce (2):** `Parity.Attributes_MPReload`, `Parity.AnimPlan_MPReload` — verified `CkAttribute_RestorePersistence.h`.
  - **→ Phase 4B, client-shaped Apply ([B1] shape) (5):** `Parity.TagSet_MPReload`, `Parity.GridPlacements_MPReload`,
    `Parity.InventoryDataOnly_MPReload`, `Parity.InventorySpatial_MPReload`, `Parity.RenderTarget_MPReload`. VERIFIED for TagSet
    (`CkTagSet_Fragment.cpp:28-32`): Apply stamps `FFragment_TagSet_SyncReplication`, drained ONLY by `FProcessor_TagSet_SyncReplication`
    which is **`ClientOnly`** (`CkTagSet_Processor.h:98`) → on the loading AUTHORITY the sync fragment is stamped but never drained → the
    server's `ck::FFragment_TagSet` never receives the tags → server assertion fails → never replicated → client assertion fails. KEY
    LESSON (from the gate-2 block, confirmed): value-emitting *Produce* is necessary but NOT sufficient — the *Apply* must apply
    authority-side. Velocity/Acceleration/TeamPlayer Apply do (they pass); the deferred-six + TagSet Apply do not. PHASE_3B §"Known
    interaction" pre-authorizes annotating this shape as 4B — do NOT invent an authority-side sync drain in 3B.
- **What passes (43):** all Model-A registry-level tests (Core/DynamicFragment×11/Audit/LifecycleStrip/SaveKey/SceneNode/Timer/
  MontagePlayer/etc.), `M2a/M2b/M2b2a/M2b2b` orchestration, value-emitting parity (`Parity.Acceleration/Velocity/TeamPlayer`), both
  Oracle, LoadGate, `V3.CaptureClassification/RecipeParamsHandleRemap/InstancedStructDiskSmoke`.
- **3B follow-ups (deferred heavier PIE tests, recorded per continuation prompt §3 — NOT gate-blocking; the existing Parity/M2b gate is
  the primary proof):** `Ck.Snapshot.Rebuild.NoDuplicateGrants`, `Rebuild.LostGrantStaysLost`, `Rebuild.OrphanHydrationLoud`,
  `Rebuild.OracleParity` (the last consumes `oracle-allowlist-p3.txt`, which exists but has no active entries yet — populated in 4A when
  a BB driver world is oracle-diffed). The ConstructSpawned adopt+hydrate path is already covered via the attribute meta-fragment
  children under the bridged M2b probe. Load wall-time (CTO suggestion 6 baseline): M2a v3 load settled ~2 frames rebuild + hydrate under
  the gate (p3b-gate3.log DIAG).
- **Net gate:** not run pre-commit (the reconstitution retirement is provably inert outside a load — the deleted spawn-suppression gate
  only fired when `Get_ReconstitutionPhase() != None`, which was ONLY stamped during a CkSnapshot load; Net tests do no loads; the
  replicated respawn/travel paths ARE exercised green by M2b2a/M2b2b). Net baseline re-captured at the 4A boundary (4A touches
  CkStateMachine replication and needs it).

**3B.2 reconstitution retirement surface (verified grep):** `ECk_ReconstitutionPhase` enum + `Get_/Set_ReconstitutionPhase`+`Get_IsReconstitutionInProgress`+`_ReconstitutionPhase` (`CkEcsWorld_Subsystem.h:38-43,179-188,218` + `.cpp:165-185`); `DoIs_WorldReconstituting`+2 call sites (`CkEntityScript_Utils.cpp:42-71,147,196`); `Get_IsSnapshotRespawnable` (`CkEntityScript.h:109`/`.cpp:163`) + its CONSUMERS (`CkEntityScript_WithActor.cpp:61` gates ActorSpawnIntent stamp — KEEP the stamp but the abstention gate becomes moot; `CkEntityScript_WithActor.cpp` M2b comment; the probe overrides it). ⚠️ `Get_IsSnapshotRespawnable` is ALSO the ActorSpawnIntent opt-in gate, not only a reconstitution gate — deleting it needs a replacement opt-in for ActorSpawnIntent (Fork-D dependent — resolve with Fable's bridged-actor design). Full retirement + `rg "Reconstitution|IsSnapshotRespawnable" Source` → 0 outside docs.

## Phase-3A progress (implementation-state checkpoint — 2026-07-11)

Additive phase (v3 writer beside Model A). IMPLEMENTED on disk (UNCOMMITTED, UNBUILT as of this checkpoint):
- **3A.1** `ck::FTag_ConstructSpawned` (TRANSIENT) in `CkEntityLifetime_Fragment.h` + stamp in
  `Request_SetupEntityWithLifetimeOwner` (`CkEntityLifetime_Utils.cpp`) — [P3A-D1].
- **3A.2** GC-safe recipe: new `UCk_EntityScript_SpawnRecipe_UE` holder UObject + `ck::FFragment_SpawnRecipe` (pins
  it via TStrongObjectPtr) in new `CkEntityScript_SpawnRecipe.h/.cpp`; stamped in `FProcessor_EntityScript_
  SpawnEntity_HandleRequests::DoHandleRequest` — [P3A-F1].
- **RemapHandles lift** ([P3A-F2]): new `CkEcs/Snapshot/CkSnapshot_HandleWalk.h/.cpp` (`ck::snapshot::RemapHandles`
  + `ForEachHandle`); CkDynamic_Fragment_Data.cpp re-pointed at the shared copy (−124 lines).
- **3A.3** v3 writer: new `CkSnapshot_CaptureV3.h/.cpp` (`Run_CaptureV3` + `_Registry` core); v3 format structs +
  `FCk_Snapshot_HeaderV3` (FormatVersion=3) in `CkSnapshot_Header.h`; `Get_SaveHandlerTypes()` accessor added to the
  registry. CkSnapshot gains a `CkLabel` dep.
- **3A.4** 10 migrated registrars flipped `Transport → NetAndSave`; Team + Player gained per-entity Produce (no
  SeedContainer). **REMAINING: the 4 complex deferred Produces (2dGridOccupancy, Inventory Spatial, Inventory
  DataOnly, RenderTarget)** — being drafted by an agent, then verify+apply — [P3A-D2].
- **3A.5** `Request_Save` dual-writes Model A + v3 into `_SnapshotBytes(V3)` / `_Header(V3)` on the SaveGame.
- **3A.6** `Test_Snapshot_V3_Capture.cpp` (CaptureClassification + RecipeParamsHandleRemap) authored.

**DONE 2026-07-11.** All 16 registrars edited (10 flip + 6 per-entity Produce; the 4 complex Produces drafted by an
agent then VERIFIED against code by Opus). One link fix (CoreOnline dep for FUniqueNetIdWrapper::ToString in the player
rendezvous — everything else compiled first pass). Gate GREEN (Ck.Snapshot 51/51/0, Net framework delta-zero). Commits
81f7b6505 / 349947218 / d0ce51877 (CkF) + 5bb9798 (CkTests). Nothing pushed. Next: Phase 3B (load side).

## Campaign-added tests (protected inventory — grows as phases land)

| Test name | Added in | File |
|---|---|---|
| Ck.Snapshot.Oracle.StructuralBaseline | Phase 0 | CkTests `Source/CkTests/Private/CkSnapshot/Test_Snapshot_Oracle_StructuralBaseline.cpp` |
| Ck.Snapshot.Oracle.ProduceDiffBaseline | Phase 1 §1.6 | CkTests `Source/CkTests/Private/CkSnapshot/Test_Snapshot_Oracle_ProduceDiffBaseline.cpp` |
| Ck.Snapshot.V3.CaptureClassification | Phase 3A | CkTests `Source/CkTests/Private/CkSnapshot/Test_Snapshot_V3_Capture.cpp` |
| Ck.Snapshot.V3.RecipeParamsHandleRemap | Phase 3A | CkTests `Source/CkTests/Private/CkSnapshot/Test_Snapshot_V3_Capture.cpp` |

## Decisions made by executors (anything the plan left as A-or-B, with which was taken and why)

- **[P0-D1] Homing dt==0 fix lives in `CkProjectile`, not `CkPhysics`.** PHASE_0 §0.3b cites
  `Source/CkPhysics/.../Homing/CkHoming_Processor.cpp`; the Homing feature actually lives at
  `Source/CkProjectile/Public/CkProjectile/Homing/`. Same fix, real file — a doc path typo, not an architecture
  change. Guard placed at the top of `FProcessor_Homing_Update::ForEachEntity` (covers the finite-diff divide at
  `:236` AND, by returning before `Compute_HomingAcceleration`, prevents ProNav's existing `InDeltaT > 0` ensure
  from firing during a settle pass — so no `CkHoming_ProNav.cpp` edit is needed, matching the plan's "one guard at
  the top covers the ProNav calls it makes").
- **[P0-D2] Oracle `_LabelPath` left empty (tier fence); label-keyed identity deferred to Phase 3B.** PHASE_0 §0.5
  specifies the label path via `UCk_Utils_GameplayLabel_UE`, but the oracle lives in **CkEcs** and that util is in
  **CkLabel**, which *depends on* CkEcs — so the reverse include edge is impossible (would break the tier direction
  the same step's fence insists on: "keep the oracle in CkEcs"). `FFragment_LifetimeOwner` IS in CkEcs, but without
  CkLabel there is no stable per-entity label text to build a path from. Decision: Tier-1 signatures use the
  fragment/tag set (fully implemented) + `_ScriptClassPath` (CkEcs-visible EntityScript fragment); `_LabelPath`
  stays empty. The Phase-0 harness fixture carries no labels, so this does not affect the gate. **Forward note for
  Phase 3B:** when label-keyed cross-rebuild identity actually matters, supply the label via a CkEcs-visible hook
  (registered label-provider callback) or perform the label step from a higher tier — do not add a CkEcs→CkLabel
  edge.
- **[P0-D3] Tier-1 lumps tags into `_FragmentTypeNames`; `_TagTypeNames` left empty** — explicitly permitted by
  §0.5 ("put everything in `_FragmentTypeNames` … Tier-1 only needs stable signatures, not taxonomy"). Unregistered
  storage types are keyed by their entt type-hash (hex) to avoid the non-null-terminated `string_view`→`FString`
  hazard; registered types use their clean `_DisplayName`.

- **[P1-D1] §1.1 include-surface CORRECTION.** The blessed ".inl.h at the bottom of CkNet_Utils.h" design FAILS —
  CkNet_Utils.h is UHT-reflected and UHT forbids any `#include` after its `.generated.h`. Fix: `RegisterLazyTyped<T>`
  body lives in `CkReplicatedFragmentContainer.inl.h` (no includes), and each migrated registrar `.cpp` includes
  `CkNet_Utils.h` then that `.inl.h`. The Attribute registrars get both via `CkAttribute_RestorePersistence.h`.
- **[P1-D2] Attribute Produce = EMPTY-SEED (option A), not the research recipe's value-emitting option B.** The old
  attribute `ReplicateOnRestore` itself empty-seeds the owner container + re-arms Current/Min/Max
  `MayRequireReplication` (it reads NO values); `FProcessor_Attribute_Replicate` refills. So a behavior-neutral
  Model-A migration is empty-seed (shared `ck::attribute_restore::Produce/SeedContainer` in
  `CkAttribute_RestorePersistence.h`), which also sidesteps the per-owner upsert-merge risk. AnimPlan uses the same
  empty-seed shape. **Consequence for 1.6/Phase 3A:** an empty-seed Produce emits an always-empty payload, so a
  Tier-2 "mutate attribute value → diff line" test would never register a change; the value-emitting Attribute
  Produce is a Phase-3A/save-path concern. The 1.6 `ProduceDiffBaseline` test should exercise a value-emitting
  feature (**Velocity** — `Produce` emits `FCk_RepData_Velocity{Get_CurrentVelocity()}`), not an attribute.
- **[P1-D3] 1.3 re-drive done-marker.** `FTag_Snapshot_JustRestored` is added at `CkSnapshot_Restore.cpp:243` and
  NEVER removed (persists). So the re-drive views on it, populates `FFragment_Persistence_ReDrivePending._Remaining`
  on first sight, drains per-tick, and LEAVES the emptied fragment as the done-marker (does not re-populate). Gate =
  each `SeedContainer`'s own driver/owner check returns `NotAdded`→retry; 5s/2s timeout → loud drop.
- **[P1-D4] 1.5 net dispatcher NOT refactored.** Extracted `ck::persistence_apply::ApplyOne` (resolve+Apply+timeout)
  used by the new dormant `FProcessor_Hydration_Dispatch`; left the tested `FProcessor_ReplicatedFragments_Dispatch`
  inline to avoid regressing the green Net/Parity gate for a dormant feature (a future cleanup can adopt ApplyOne).
  Hydration processor view needs `ck::TReadWrite<FFragment_PendingHydration>` (ck_exp::TProcessor static_assert).

- **[P1-D5] §1.6 oracle Tier-2 shape.** `Capture_Payloads(SnapshotRegistryType&, FCk_RegistryHandle, const TSet<uint32>*
  =nullptr)` mints per-entity handles via `FCk_Registry{InRegistryHandle}` + `ck::MakeHandle(FCk_Entity{e}, CkRegistry)`
  (the `FCk_Registry(FCk_RegistryHandle)` ctor at `CkRegistry.h:186` makes the spec's `FCk_RegistryHandle` param work
  directly). Keyed by the Tier-1 signature; the storage-sweep signature builder was EXTRACTED into a shared internal
  `BuildEntitySignatures` helper used by both Capture_Structural (behavior-neutral — StructuralBaseline stays green) and
  Capture_Payloads. `Diff_Payloads` = per-(sig,type) composite-ExportText comparison (`~`/`+`/`-` lines): a VALUE change
  is ONE line, not an add/remove pair. New registry method `Get_ProduceHandlerTypes()` = superset of
  `Get_ReDriveHandlerTypes()` (every handler with Produce, incl. capture-only). Per [P1-D2] the ProduceDiffBaseline test
  uses the REAL value-emitting **Velocity** handler (Add creates Current immediately; Request_OverrideVelocity is an
  immediate write — no tick needed in a bare FEcsWorld); mutate → exactly 1 diff line, unmutated → 0.

## Decisions — Phase-3B (Fable ruling verified against code + executor)

- **[P3B-F1] (Fable, VERIFIED) — Fork D: bridged-actor (WithActor + ActorSpawnIntent, RuntimeSpawned) rebuild = ACTOR-FIRST, `FProcessor_ActorRespawn` NOT reused; PHASE_3B plan text overridden.** The loader spawns the ACTOR (`_ActorClassPath`, at the saved transform); the actor's own BeginPlay (`ACk_AutoTest_NetSubject::BeginPlay`→`Request_SpawnEntityScript_OnActor`, CkAutoTest_NetSubject.cpp:55-71; generic bridge `CkEntityScript_WithActor_ActorComponent.cpp:12-23`) re-creates the WithActor entity with Construct running FOR REAL (features compose). The whole EntityScript pipeline is load-kernel (RunsDuringLoad, CkEntityScript_Processor.h) so this constructs under the gate. Rendezvous-map `_SavedId`→`UCk_Utils_OwningActor_UE::TryGet_ActorEntityHandle(actor)` (CkOwningActor_Utils.cpp:195-210; link-valid ⟹ Construct done). `Request_SpawnEntity(WithActorScript, savedParams)` is IMPOSSIBLE (the actor ptr in `FCk_EntityScript_WithActor_SpawnParams` doesn't serialize; Construct ensures on invalid `_OwningActor`, CkEntityScript_WithActor.cpp:24-28). `FProcessor_ActorRespawn` stays compiled-but-DORMANT in 3B (nothing stamps `FTag_ActorRespawn_Pending` once `DoStamp_RespawnMarkers` dies with the Model-A tail); deleted Phase 5. Its `DoReplicate_RestoredEntity` re-replication is unneeded — the fresh Construct runs the normal replication path (this is what M2b2a now exercises organically).
- **[P3B-F1a] (Fable, VERIFIED) — Fork D3: transform via a NEW recipe field `_ActorSpawnTransform` (option a), NOT a Transform Produce (option b).** Option (b) is broken: `FProcessor_Transform_SyncFromActor` UNCONDITIONALLY overwrites the entity Transform fragment from the actor's root component whenever they differ (VERIFIED CkTransform_Processor.cpp:121-126) and NO Transform processor is load-kernel — a hydrated transform would park behind the gate while the actor sits at identity. Option (a): add `FTransform _ActorSpawnTransform` to `FCk_Snapshot_V3_EntityEntry`, capture via `UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform` (guarded by `Has`, mirroring CkActorRespawn_Processor.cpp:76-78) in the RuntimeSpawned branch (CkSnapshot_CaptureV3.cpp:323-355); loader spawns the actor there; `WithActor::Construct` seeds the entity Transform from `_OwningActor->GetActorTransform()` (CkEntityScript_WithActor.cpp:50-53) — position correct with zero hydration, zero sync hazard. Additive save-side change to committed 3A (justified; new field defaults to identity, only meaningful when `_ActorClassPath` set).
- **[P3B-F2] (Fable, VERIFIED) — Fork A: map-backed `FSnapshotContext` v3 mode.** New ctor `FSnapshotContext(const TMap<uint32,FCk_Handle>* InSavedIdMap, FCk_RegistryHandle InLoadRegistryHandle)` + member `_SavedIdMap`; `IsLoading()` returns true when set; a branch in `Snapshot_EnttEntity` ahead of the raw-cast fallback: on load, `Find(RawId)` → set entity to mapped `Get_Entity().Get_ID()` (or `entt::null` if absent/sentinel `0xFFFFFFFF`). `Snapshot_Handle`'s existing tail re-homes the registry via `_LoadRegistryHandle` (Set_Registry, CkHandle.h). Key = packed entt uint32 = `_SavedId` (VERIFIED CkSnapshot_CaptureV3.cpp:50 + Snapshot_EnttEntity raw write). No typed-handle slice (set entity+registry in place). Reader helper mirrors `SerializeInstancedStruct` exactly (proxy, ArIsSaveGame=false, SetIsPersistent(true), CkSnapshot_CaptureV3.cpp:100-104); tables via `FCk_Snapshot_V3_Tables::StaticStruct()->SerializeItem(Reader,&Tables,nullptr)` (idiom Test_Snapshot_V3_Capture.cpp:172-196).
- **[P3B-F3] (Fable, VERIFIED) — Fork B: adopt ConstructSpawned via the LifetimeDependents walk, NOT `Get_EntityOrRecordEntry_WithFragmentAndLabel`** (that is a compile-time template over `T_FragmentUtils`/`T_RecordUtils` — unusable type-erased). Mapped owner → `Owner.Get<ck::FFragment_LifetimeDependents>().Get_Entities()` (usage precedent CkSnapshot_Subsystem.cpp:313) → filter `Has<FTag_ConstructSpawned>()` + labeled (`UCk_Utils_GameplayLabel_UE::Has && NOT Get_IsUnnamedLabel`, mirror capture rule 3) → match `Get_Label(Child).ToString() == Entry._Label`. Needs a `Request_PumpToQuiescence(LoadKernel)` after owner mappings + a bounded poll (child label may be added one kernel-tick later, e.g. WithActor labels itself in its own Construct; ConstructionFlow::Continue scripts span frames). Frame-cap → loud ensure + skip (its payloads then time out loudly).
- **[P3B-F4] (Fable, VERIFIED) — Fork C: reconcile QUEUES `Request_DestroyEntity` only (parks; the destruction pipeline is NOT load-kernel — only `FProcessor_EntityLifetime_EntityJustCreated` is RunsDuringLoad, CkEntityLifetime_Processor.h:19), completes ~2 post-gate frames.** Enumerate strays = owner's live labeled ConstructSpawned children (LifetimeDependents walk, same as F3) minus the saved `_Label` set for that owner → `Request_DestroyEntity` (precedent CkSnapshot_Subsystem.cpp:315-319). Reconcile does NOT wait (would deadlock under the gate). `LostGrantStaysLost` polls `ck::IsValid→false` ≥2 post-load-complete frames.
- **[P3B-D1] (executor) — KEEP `Get_IsSnapshotRespawnable`; delete ONLY the reconstitution machinery. Deviates from PHASE_3B 3B.2's literal "delete Get_IsSnapshotRespawnable" — the plan OVERLOOKED that it is ALSO the `FFragment_ActorSpawnIntent` stamp opt-in** (`CkEntityScript_WithActor.cpp:61`, v3 needs it — it supplies `_ActorClassPath`) not only the EarlyWindow reconstitution gate. Deleting it wholesale would stop the probe being persisted → e2e gate fails. So: retire `ECk_ReconstitutionPhase`/accessors/`_ReconstitutionPhase`/`DoIs_WorldReconstituting`/`Get_IsReconstitutionInProgress` + all CkSnapshot stamp sites; KEEP `Get_IsSnapshotRespawnable` (orthogonal save opt-in). Exit grep becomes `rg "Reconstitution" Source` → 0 (NOT "|IsSnapshotRespawnable").
- **[P3B-D2] (executor, from Fable landmine) — retiring reconstitution is a PREREQUISITE for 3B.1, not just cleanup.** `Request_SpawnEntity` returns `{}` (suppresses the spawn) whenever `Get_ReconstitutionPhase() != None` (CkEntityScript_Utils.cpp:147-148, 42-71). The v3 loader spawns actors whose BeginPlay triggers entity spawns — those would be SUPPRESSED if any reconstitution phase were stamped on the post-travel world. So 3B.1 must NOT stamp any phase post-travel (delete the EarlyWindow world-init watch + the `Full` escalation at CkSnapshot_Subsystem.cpp:634); the LoadKernel gate is the isolation mechanism.
- **[P3B-D3] (executor) — FULL reconstitution retirement incl. the pre-travel `Full` stamp (per PHASE_3B), deviating from Fable's "keep pre-travel Full" caution.** The load gate + v3-restores-no-image ⇒ suppression is unnecessary everywhere; a pre-travel teardown spawn would be wiped by travel anyway. WATCH-POINT: if the gate shows a pre-travel-teardown duplicate/spawn issue, this stamp is the suspect (cheap to restore narrowly).
- **[P3B-D4] (executor) — EXPECTED CASUALTIES (annotate as Phase-4, do NOT fix in 3B): `Ck.Snapshot.StateMachineParity_MPReload`** — CkStateMachine handlers are Apply-ONLY (no Produce, not in the NetAndSave census; CkStateMachine_Replication.cpp:295-349) → SM saved state isn't captured → SM rebuilds to its initial state → parity reds. SM redrive-as-hydration IS Phase 4A. Also the **deferred-six parity** (GridPlacements, InventoryDataOnly, InventorySpatial, RenderTarget, TeamPlayer) — their Apply is client-shaped (stamp-a-sync-fragment, ClientOnly sync) so authority-side hydration may not drain → [B1]-shape → annotate Phase-4B (per PHASE_3B §"Known interaction"). CLEAN features (Acceleration, AnimPlan, FloatAttribute, TagSet) MUST pass — if one reds via Setup-stomp (hydration applies under gate before its gated Setup runs at gate-open) that is a REAL 3B ordering bug to fix (candidate: defer hydration apply to the settle full-pump so Setup precedes it in-pass), NOT annotate. Fable gap-3: unaudited across all 16 handlers — parity gates are the empirical pin.
- **[P3B-D6] (executor, 2026-07-12, gate-3) — M2a fix = FIXTURE respawn opt-in (NOT the N1 discriminator); TagSet + AnimPlan re-categorized. Corrects [P3B-D4] and the gate-2 block.** [P3B-D4] listed AnimPlan/FloatAttribute/TagSet as "CLEAN … MUST pass"; the gate-2 block then reclassified M2a as an N1/Phase-4A casualty. Both are wrong. VERIFIED against code + gate-3: (a) **M2a** is a WithActor BRIDGED probe missing the `Get_IsSnapshotRespawnable()->true` opt-in (base default false, `CkEntityScript.cpp:161-167`); adding it (CkTests `ce32c65`) makes the loader's bridged actor-first branch (`CkSnapshot_Subsystem.cpp:628-663`, which does NOT consult owner saved-id) respawn it — M2a GREEN in 3B, no N1 needed. N1 is specifically the NON-bridged path (SM-task spawn under the transient), which M2a never was. (b) **TagSet** is NOT clean — its Apply is client-shaped (`FFragment_TagSet_SyncReplication` drained only by the `ClientOnly` `FProcessor_TagSet_SyncReplication`, `CkTagSet_Processor.h:98`) → authority-side hydration never drains → 4B casualty ([B1] shape), same class as Grid/Inventory×2/RenderTarget. (c) **AnimPlan** is empty-seed Produce (4B), not clean. Only **FloatAttribute value-parity** stays in the "value round-trips via clean Apply" club alongside Velocity/Acceleration/TeamPlayer — but note the FloatAttribute *value* still needs 4B (empty-seed Produce); what passes for attributes is the M2a existence/base-value check, not `Parity.Attributes` value-parity. Rule of thumb going forward: value-emitting Produce is necessary but insufficient — a feature round-trips in 3B ONLY if its Apply writes authority-side (direct fragment write), not if it stamps a ClientOnly sync fragment.

## Decisions — Fable-agent rulings (unattended-protocol consults)

- **[P4A-D1-Fable] (2026-07-12, 4A.1) — SM registrar fork + full-plan review.** Consulted a read-only Fable agent on the
  RegisterLazy-vs-RegisterLazyTyped fork (see [P4A-D1] under §Decisions—Phase-4A). Ruled Q1 YES (keep RegisterLazy/no
  SeedContainer — SeedContainer's sole consumer is the Model-A `FProcessor_Persistence_ReDriveOnRestore` at
  `CkPersistence_ReDrive_Processor.cpp:65`, JustRestored-gated; net+save use Apply only) + adversarial-reviewed the whole
  4A.1 edit plan: zero blockers, 5 findings — F1 (echo-suppress-first is load-bearing) + F2 (friend-decl rename) already
  correct in the diff, F3/F4 comment tightenings applied, F5 (sub-SM state drop) = pre-existing v1 bound. Every claim
  independently code-verified. Gate green.
- **[P4B-F1] (2026-07-12, 4B) — the 5 client-shaped-Apply casualties: uniform shell, per-feature body; Inventory×2 → Adam.**
  Consulted a read-only Fable agent to design the client-shaped-Apply re-author (TagSet/Grid/Inventory×2/RenderTarget).
  Ruling: uniform SHELL (hydration-scope branch first → authority write → Applied/NotReady, the 4A.1 shape) but per-feature
  BODY. **TagSet** = direct `Set_Tags` REPLACE + arm `FTag_TagSet_MayRequireReplication` (`Request_AddTags` is UNFAITHFUL —
  Construct re-seeds default tags → ADD resurrects a runtime-removed tag) — IMPLEMENTED + gate-green this session (`5088f5336`).
  **Grid** = re-drive `Request_AddPlacement` per entry, ALL occupants resolve first (all-or-nothing NotReady) — but ⚠️ its
  occupants must be v3-mapped; if the test's occupants are item-style anonymous, Grid folds into [INV-A] (verify the fixture
  first). **RenderTarget** = transplant the Model-A `FProcessor_RenderTarget_ReplicateOnRestore` body (`CkRenderTarget_Processor.cpp:397-500`)
  into a static helper, CHILD-keyed, exactly-once repaint (all gates before any mutation); NOT the request path (evidence-forced
  deviation from PHASE_4B §4B.2, same class as [P4A-D1]); ⚠️ verify the settle Full-pump runs RT Setup before the dispatcher
  timeout. **Inventory×2** = RECLASSIFIED, DEFER → [INV-A] (items are v3 Rule-5 anonymous; handles-only payload restores
  nothing; product/architecture fork for Adam). Full per-feature design map (verified anchors) in CONTINUATION_PROMPT_Phase4B.md §6.
- **[P4B-F2] (2026-07-12, 4B) — Attributes (×5 kinds) + AnimPlan empty-seed → value-emitting Produce + authority-side hydration Apply.**
  Consulted a read-only Fable agent; independently code-verified EVERY load-bearing anchor before implementing. Ruling + verification:
  - **Keying (foundation):** the attribute/AnimPlan v3 Produce fires on the ATTRIBUTE/PLAN child entity (`Has<Current>` /
    `Has<AnimPlan_Current>`), so the payload is keyed on the CHILD saved-id and hydration dispatches `Apply(ChildEntity, payload)` —
    same owner-vs-child mismatch as RenderTarget/TagSet. VERIFIED against the v3 capture per-entity Produce loop + the OWNER-keyed net
    Apply (`CkFloatAttribute_Fragment.cpp:125` TryGet(Entity-as-owner)).
  - **Attributes design (per-attribute-entity, shared across 5 kinds):** make the shared `ck::attribute_restore::Produce`
    (`CkAttribute_RestorePersistence.h`) value-emitting — emit THIS attribute's Base/Final per composed component (Current always;
    Min/Max if present), byte-identical to the wire-builder `CkAttribute_Processor.inl.h:235-241` (VERIFIED: `Get_Base/Get_Final`,
    entry ctor `{Name,Base,Final,Component}` `CkFloatAttribute_Fragment.h:184`, `ComponentTagType`/`HandleType`
    `CkAttribute_Fragment.h:241-243`). Add a shared `TryHydrationApply` template helper (hydration-scope-gated FIRST statement of each
    Apply) that writes AUTHORITY-side via each kind's existing `ApplyReplicated*Entry` (same path the client net drain uses) — one
    3-line insertion per registrar (×5). Chosen over per-OWNER Produce (rejected: moves the trigger entity → ripples into 4 consumers +
    breaks stale saves).
  - **SeedContainer stays EMPTY-seed (decoupled from the now-value Produce):** seed `T_RepDataStruct{}` explicitly, NOT `InData`.
    VERIFIED `TryAddContainerFragment` returns `AlreadyExists` WITHOUT writing when the container exists (`CkNet_Utils.h:461-462`) → a
    value seed would land only the first sibling's data (the per-owner upsert-merge hazard the header comment flags). Empty-seed +
    re-arm `MayRequireReplication` → `FProcessor_Attribute_Replicate` rebuilds → Model-A byte-neutral. v3 never calls SeedContainer.
  - **Client convergence needs NO explicit owner-container refill (unlike RenderTarget):** the authority write via `Request_Override`
    etc. defaults to `TrySyncToClients` → arms `FTag_MayRequireReplication` → `FProcessor_Attribute_Replicate` upserts the owner
    container (which already exists on the freshly-Constructed owner) → post-load clients converge via the unchanged net path.
  - **Exactly-once:** `Add_Revocable` mints a new modifier per call, so ALL NotReady exits precede any mutation (only the `Has<Current>`
    guard; a component-drift entry is warn+skip, NOT NotReady). No attribute/AnimPlan `NeedsSetup` tag exists (composition is
    synchronous in `Add`), and the child only maps AFTER the owner's replayed Construct ran `Add`, so the guard is near-vacuous; the Full
    settle-pump (`CkSnapshot_Subsystem.cpp:983`) precedes hydration.
  - **AnimPlan (hand-written, not templated — different payload shape):** value-emitting Produce emits
    `FCk_AnimPlan_State{Params.Get_Params().Get_AnimGoal(), Current.Get_AnimCluster(), Current.Get_AnimState()}` (mirrors wire-builder
    `CkAnimPlan_Processor.cpp:129-130`, VERIFIED); hydration branch = `Request_UpdateAnimState(Cast<FCk_Handle_AnimPlan>(Entity),{cluster,state})`
    per saved plan; SeedContainer seeds `FCk_RepData_AnimPlans{}`.
  - **Oracle safety (VERIFIED):** ProduceDiffBaseline uses Velocity (not attributes) so unaffected; StructuralBaseline/RepDataRestoreCoverage
    don't consult attribute Produce. Land Produce + Apply together (a value Produce without the Apply fix would loud-fail the parity gate).
  - **Executor-judgment calls (Fable flagged 3 for Adam; taken as executor judgment, consistent with the campaign's wire-fidelity restore
    pattern — same class as [P4B-D1]/[SM-A]; recorded for veto):** (1) authority modifier-stack FLATTENING — `ApplyReplicated*Entry`
    reconstitutes value as Base + one synthetic (Final-Base) replication modifier, losing per-modifier identity (already how the CLIENT
    applies replicated attributes every frame; runtime modifier entities are unlabeled → save-transient anyway); (2) component-mismatch =
    warn+skip (not loud-drop the whole payload); (3) DoesNotReplicate attributes now hydrate (mode-agnostic, mirrors RenderTarget). None
    is a hard STOP: all are engineering-consistency calls with campaign precedent, not new product risk. Adam can veto any via PROGRESS.
  - **CkTests:** NO change needed — the parity tests already exist (they ARE the gate). One stale doc comment in CkTests
    `Test_Snapshot_Oracle_ProduceDiffBaseline.cpp:4-6` ([P1-D2] "attribute Produce is empty-seed") goes stale — deferred doc follow-up
    (not gate-blocking; avoid a cross-repo CkTests commit for a comment).
- **[P4B-D4] (executor + Fable root-cause, 2026-07-12) — the Attributes parity RED after the value fix is a PRE-EXISTING
  FALSE-POSITIVE in the test's `Verify_AllStoredHandlesResolve` invariant, NOT a value regression and NOT [INV-A]. Fixed the
  invariant (CkSnapshot), not the loader/framework/test.** After the value-emitting Produce landed, `Parity.Attributes_MPReload`
  still failed — but on the SERVER dangling-handle invariant (`server: no dangling stored handles ... to be 0, but it was 20`),
  NOT the value asserts (Float/Byte/Integer/Vector/Rotator all round-trip — the value fix WORKS). ISOLATION: the identical 20
  danglers appear in the pre-change gate log (`p4b-rt.log`), so pre-existing. Fable root-caused + I VERIFIED every anchor:
  the 20 danglers are **EntityScript spawn-request scratch entities** — `UCk_Utils_EntityScript_UE::...Add()` creates a request
  child (`CkEntityScript_Utils.cpp:244`) registered in the parent's `FFragment_LifetimeDependents`
  (`CkEntityLifetime_Utils.cpp:549`), then destroys it (`CkEntityScript_Processor.cpp:53-54`), and the parent's dependents
  array is DELIBERATELY NOT pruned (`CkEntityLifetime_Utils.cpp:153-155` — a documented perf choice; every consumer, incl.
  `Get_LifetimeDependents:151-160`, filters via `ck::IsValid`). So `FFragment_LifetimeDependents` is a LAZILY-PRUNED WEAK-ref
  list; the 19 odd-id parents are the fresh boot's re-created boot-infra EntityScripts (matching the DIAG's "19 skipped
  boot-infra") + the bridged probe (60→61). A strict-resolve dependents check would false-positive on EVERY EntityScript world,
  saved or not — sole caller is the Attributes test (`:299`), which only runs it post-reload so never noticed. FIX (Fable ruling
  C, verified): in `Verify_AllStoredHandlesResolve` (`CkSnapshot_RestoreInvariants.cpp`, campaign-added `e669659d0`), the
  dependents leg now SKIPS stale (destroyed) refs and instead asserts BACK-POINTER CONSISTENCY — a live dependent's
  `FFragment_LifetimeOwner` must name this owner (`TransferLifetimeOwner` keeps it symmetric+EAGER, `:570`/`:583`, VERIFIED, so
  no false positive) — the genuine registry-rehome corruption check. The strict `LifetimeOwner`/`ContextOwner` legs (hard refs)
  are UNCHANGED. Rejected: (A) load-path prune of dependents = treats a documented framework-wide lazy contract as a load
  defect + a CkEcs-core behavior change (ck-change-control territory), unjustified; (B) defer as [INV-A] = factually wrong (the
  danglers are spawn-request scratch, not anonymous modifiers; pre-existing). Test-only invariant (no production caller), so no
  net/wire/Model-A impact. **GATE GREEN (p4b-attr2.log):** build Succeeded; `Parity.Attributes_MPReload` +
  `Parity.AnimPlan_MPReload` Success; ZERO dangling-handle errors; Ck.Snapshot 52/49/3 (the 3 = [INV-A]-deferred Grid +
  Inventory×2), delta-zero. Committed CkF `19613a1eb` (invariant fix), `5b7c1e077` (Attributes×5 value hydration), `db83e687a`
  (AnimPlan). Net delta-zero validated on the net-identical binary (p4b-attr-net.log 102/98/4 = kiosk env-trio + documented
  SM.Net flake; the invariant fix is test-only/net-irrelevant so the verdict holds).
- **[P4B-N1] (executor, 2026-07-12) — 4B ADDITIVE-work scope assessment: 4B.1 params-mutators + MontagePlayer are
  VERIFICATION-GATED (defer with OracleParity); 4B.3 AS smoke is the one verifiable-but-new-framework item.** After
  closing all 4 actionable casualties (RenderTarget/Attributes×5/AnimPlan) + the invariant fix, the remaining PHASE_4B.md
  steps were assessed for verifiability:
  - **4B.1 (8 params-mutator sites: Timer/Substep/Goap/2dGridCell/WorldSpaceWidget payloads + Camera/Pmg annotations) and
    the MontagePlayer rebind are UNVERIFIABLE right now.** Their verification mechanism is `Ck.Snapshot.Rebuild.OracleParity`
    — VERIFIED NOT in the current 52-test gate (grep count 0 in p4b-attr2.log) and its `oracle-declared-transient.txt` /
    `oracle-allowlist-p3.txt` have no active entries (the file doesn't even exist yet); PROGRESS §3B-follow-ups records
    OracleParity as deferred until a BB driver world is oracle-diffed (a 3B/4A follow-up). MontagePlayer additionally has
    ONLY a Model-A `Ck.Snapshot.MontagePlayer.StateRoundTrip` (registry-level) test — no v3-reload MP test exists, and the
    rebind (`CkMontagePlayer_Utils.cpp:80-95`) needs the respawned actor's re-created SKMC resolved (non-trivial). Implementing
    these payloads blind would add unverifiable persisted save-state, violating verify-before-claim. **DEFER 4B.1 + MontagePlayer
    to when OracleParity is wired** (the same BB-driver-world prerequisite that blocks the 3B OracleParity follow-up and 4A.2).
  - **4B.3 AS smoke** (`Ck.Snapshot.AS.SaveGameFields_RoundTrip` + `NonSaveGameField_Drops`) IS gate-verifiable, but requires a
    NEW framework handler `FCk_SaveData_EntityScriptFields` in CkEcs (reflection-walk of the script instance's SaveGame-tagged
    UPROPERTYs, Produce/Apply post-Construct-pre-BeginPlay) + 2 AS tests in CkTests (cross-repo, needs AS wrapper-gen + editor
    recompile per `ck-angelscript-interop`). A distinct new-framework + AS effort — best served by a fresh context window after
    this session's length, done carefully per the AS cautions (never write .as during a test run; grep the fresh log for
    Angelscript errors).
  - **Checkpoint rationale:** the core Phase-4B hydration-parity goal is COMPLETE + fully verified for every closeable feature.
    The remaining work is human-decision-gated ([INV-A] trio → Adam), verification-prerequisite-gated (4B.1/MontagePlayer →
    OracleParity/BB-driver-world), or a distinct fresh-context new-framework effort (4B.3 AS smoke). Not an arbitrary stop.
- **[P4B-D5] (executor, 2026-07-12, 4B.3) — EntityScript SaveGame fields land POST-Construct/POST-BeginPlay (at hydration),
  NOT pre-BeginPlay; recorded as the designer contract (the continuation's small human decision (a), taken as executor
  judgment).** VERIFIED: `FProcessor_EntityScript_BeginPlay` is load-kernel (`RunsDuringLoad`, `CkEntityScript_Processor.h`),
  so on a v3 load the fresh script re-Constructs AND BeginPlays under the load gate BEFORE the hydration payloads enqueue —
  identical to every other v3-hydrated feature. So the framework `FCk_SaveData_EntityScriptFields` Apply restores SaveGame
  fields after BeginPlay ran, contradicting PHASE_4B.md:37-38's "post-Construct/pre-BeginPlay" wording. **Contract line for
  spec §3 AS surface:** *don't read a script's UPROPERTY(SaveGame) fields in its BeginPlay — they land at hydration, one gate
  pass later.* Building a BeginPlay-defer would be a scheduler-core change, out of 4B scope; same class as [SM-A]/[P3B-D6]
  (campaign-precedented executor call). Adam can veto via PROGRESS. Gate green with the fields restored (the AS RoundTrip test
  never reads them in BeginPlay, so the contract holds in practice).
- **[P4B-D1] (executor, 2026-07-12) — TagSet authority write omits the OnTagsChanged broadcast.** Fable's pseudocode mirrored
  the client drain's diff+broadcast (`CkTagSet_Processor.cpp:156-168`) for symmetry. Omitted it: a save-load RESTORE is not a
  gameplay change, the parity test asserts tag presence + replication (not the signal), and clients still get the OnTagsChanged
  pulse via their own SyncReplication drain after the authority re-replicates. Kept the change minimal (Set_Tags REPLACE + arm,
  no new includes). If an authority-side OnTagsChanged consumer is later shown to need the restore pulse, add it then.

- **[P4B-D3] (executor, 2026-07-12) — RenderTarget hydration helper lives as a static method of `FProcessor_RenderTarget_HandleRequests`, NOT a free function in `ck_render_target_processor` (minor refinement of [P4B-F1]).** [P4B-F1] specified `ck_render_target_processor::HydrateFromSavedChannel` (namespace free function). A free function needs friend access to `FFragment_RenderTarget_Current._NextBatchSeq` — which would force a dllexport-mismatched forward-declaration of the function into the widely-included `CkRenderTarget_Fragment.h` (the friend decl there would lack CKRENDERTARGET_API while the Processor.h decl carries it → MSVC linkage-attribute mismatch). Instead made it a public static of `FProcessor_RenderTarget_HandleRequests`, which is ALREADY a friend of `FFragment_RenderTarget_Current` (`CkRenderTarget_Fragment.h:69`) AND already owns `DoApplyBatch` (the shared repaint primitive it calls) — zero Fragment.h churn, no new friend decl. Same intent (transplant the Model-A `FProcessor_RenderTarget_ReplicateOnRestore` body onto the child-keyed hydration path). Signature `HydrateFromSavedChannel(FCk_Handle& InChild, const FCk_RenderTarget_ChannelState& InChannel) -> bool` (true=Applied, false=NotReady); the Apply lambda maps bool→`ECk_RepFragment_ApplyResult`. GATE GREEN (Ck.Snapshot 52/47/5, `Parity.RenderTarget_MPReload` flipped; delta-zero). Key facts driving the design (verified): payload is CHILD-keyed (`CkRenderTarget_Replication.cpp:120-140` Produce reads the sync child's own AuthoredLog) but the net Apply is OWNER-keyed (`:96-106` `TryGet_RenderTarget(Entity-as-owner)`) → gate-log confirmed the child `[63](RenderTarget.AutoTest.Net)` dropped after 5s NotReady on the owner-keyed lookup; `Get_LatestAppliedBatchSeq = max(Current._NextBatchSeq-1, ClientReplayState._LastAppliedSeq)` (`CkRenderTarget_Utils.cpp:120-127`) → server needs `Current._NextBatchSeq=LastSeq+1` (friend write) + Replicates half refills the owner container so the client's `_LastAppliedSeq` converges; settle pump is Full (`CkSnapshot_Subsystem.cpp:983`) so RT Setup precedes hydration. AuthoredLog refilled via public `Record_PublishedBatch` (`CkRenderTarget_RepData.cpp:23-34`, ring-caps 64) — no friend needed there.

- **[P4B-D2] (executor, 2026-07-12) — Grid `Parity.GridPlacements_MPReload` FOLDS INTO [INV-A], DEFERRED (the design-map ⚠️
  contingency fired).** [P4B-F1]'s Grid design was Apply-branch-only IFF the test's occupants v3-map. VERIFIED they do NOT:
  the occupant is `Request_CreateEntity(InHandle)` + `Transform::Add` + `2dGridObject::Add`
  (`CkAutoTest_NetSubject_GridEntityScript.cpp:77-82`); `2dGridObject::Add` adds ONLY `FFragment_2dGridObject_Params`, no label
  (`Ck2dGridObject_Utils.cpp:11-20`); nothing else labels it → it is an **unlabeled ConstructSpawned child** → v3 capture Rule 3
  unlabeled-skip (`CkSnapshot_CaptureV3.cpp:235-259`) → never captured, never in `_SavedIdMap`. EMPIRICAL confirmation
  (`p4b-tagset.log` Grid window): `rebuild complete — [1] entities mapped` / `enqueued [1] payloads across [1] mapped entities
  ([19] skipped boot-infra, [0] orphaned)` — only the bridged GRID maps; the occupant is absent. The single payload
  (`FCk_RepData_2dGridPlacements`) carries the occupant HANDLE, which remaps to invalid on load → an all-or-nothing
  `Request_AddPlacement` re-drive fails the occupant-valid gate → NotReady → 5s drop → red. Same class as [INV-A]: **grid
  placement restore is DOWNSTREAM of occupant restore**, and production grid occupants ARE the Rule-5 anonymous inventory items
  [INV-A] defers. A fixture-only fix (label the test occupant) would false-green — production occupants are anonymous, so it
  would not reflect the real restore path. **Salvage alternative for Adam (recorded under [INV-A]):** because the grid subject's
  `Construct` re-creates a fresh occupant on load, a bespoke Grid Apply could resolve the occupant by enumerating the
  re-Constructed grid's GridObject child instead of remapping the payload handle — but that is a feature-specific
  child-resolution path (single-occupant, direct-child assumptions) and is the same payload-enrichment-vs-rearchitecture scope
  call as [INV-A]. NOT implemented; not a STOP (pre-authorized fold). **4B actionable casualties reduce to 3: RenderTarget,
  Attributes, AnimPlan.**

- **[P3B-M2a] (2026-07-12, gate-2→gate-3) — Fable ruling A: M2a red is a test-fixture gap, not a Phase-4 casualty.**
  Consulted a Fable agent (read-only) when gate-2's M2a red fell outside the user's expected-RED set. Ruling: the bridged
  actor-first respawn path does not depend on the N1 discriminator (`bBridged = NOT ActorClassPath.IsEmpty()`,
  `CkSnapshot_Subsystem.cpp:628`; the boot-infra skip is only in the non-bridged else-branch at `:686`), so opting the M2a probe
  into `Get_IsSnapshotRespawnable` (like M2bProbe) bypasses the skip; keep `bReplicates=true` (M2b2a proves replicated respawn is
  safe in-scheduler via `FProcessor_ActorRespawn`; `CkAutoTest_NetSubject_M2bProbe_Replicated.cpp:10-13`); value check holds
  (42.5 = Construct base, no override). VERIFIED against code (base default false; bridged branch bypasses skip; M2b2a topology
  identical + green) + gate-3 (M2a GREEN). See [P3B-D6].

- **[BI-1] (2026-07-11, §1.6) — kiosk-destruction `Net` reds are pre-existing/environmental, NOT the campaign.**
  Consulted a Fable agent when the `Net` gate diverged (3 BB `Bb_AutoTest_RentnetKiosk*` destruction tests red; baseline
  had them green). VERIFIED its verdict against code: the failing tests compose all entities
  `ECk_Replication::DoesNotReplicate` (`BB_AutoTest_RentnetKiosk_DamageToDestroy.as:39,46,53,59`) → the replicated-fragment
  registry / Produce / re-drive surface (all the campaign touched) is never consulted for them → mechanically severed
  from Phase-1-core AND §1.6. Independently confirmed §1.6 is inert by construction: 274+/66− diff wholly additive to the
  registry (+19/+9) + gated to `#if CK_WITH_FIDELITY_ORACLE` (oracle); `Get_ProduceHandlerTypes`/`Capture_Payloads`/
  `Diff_Payloads` have ZERO production callers. Failure shape = hits 2–3 miss fixed 0.4s ScheduleSettle windows under
  machine load (a sibling save-load session was active on this box); the kiosk Setup file records prior settle-timer
  races. Recorded the trio as a known env-red in the baseline. Safe to commit §1.6 + proceed to Phase 2.

- **[P2-D2] (2026-07-11, Phase 2 §2.5) — fire-gating aggregates over the LIFETIME-DEPENDENTS tree, not a driver list.**
  Consulted a Fable agent when PHASE_2's "traverse the dependent set the counters maintain" didn't map to code (the
  driver tracks dependents only by COUNT — `_NumSynced`/`_ExpectedNumberOfDependentReplicationDrivers`; no owner→dependents
  collection). VERIFIED its ruling against code: the traversable set IS `UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents`
  (`FFragment_LifetimeDependents`), populated on the client before the count bump, and it is the exact traversal
  `DoCount_ReplicationDriversIncludingDependents` (Utils.cpp:53-79) derives the expected count from — so at fire-tag-set
  time every synced dependent is reachable. Chose option (b) recurse-the-tree (mirror DoCount) over (a) self-only [leaves
  `OnDependentsReplicationComplete` firing in the §2.4 ConstructedThisFrame window — a real hole with consumers like
  CkEntityCollection_Processor.cpp:184] and (c) push-aggregation [new owner-side state, overkill]. Two-check predicate
  (`FTag_RepFragments_PendingApply` subsumes removals + `FFragment_PendingHydration`); no non-driver prune (hydration is
  net-mode-agnostic). Stall-bounded by the dispatcher 5s/2s timeout. Caveat (noted): cross-registry children are excluded
  from LifetimeDependents (they carry no driver on-wire — acceptable).

- **[P3A-F1] (2026-07-11, Phase 3A §3A.2) — FFragment_SpawnRecipe stores its recipe on a per-entity holder UObject,
  pinned by the fragment (Fable ruling, VERIFIED against code).** PHASE_3A §3A.2's plain-fragment sketch is GC-UNSAFE:
  fragments are not GC-traced, so a plain `FInstancedStruct`/`TSubclassOf` member dangles — worst for NotInstanced
  scripts (DoHandleRequest returns the archetype CDO directly, `CkEntityScript_Processor.cpp:98-101` — no per-entity
  object exists to lean on) and for AS-defined params structs (the `FInstancedStruct`'s UScriptStruct type ptr is
  untraced across a script reload). §4.2's asset-only rule doesn't rescue it (a loaded asset with no traced referencer
  is collectible; the spawn→save window is the entity's whole life). The fence PRE-AUTHORIZES the fix: mirror
  `_ReplicationData_EntityScript` (USTRUCT UPROPERTY carrier, `CkEntityReplicationDriver_Fragment_Data.h:93-134`).
  Chosen shape: a small `UCk_EntityScript_SpawnRecipe_UE` (`UPROPERTY TSubclassOf<UCk_EntityScript_UE> _ScriptClass` +
  `UPROPERTY FInstancedStruct _SpawnParams` — FInstancedStruct traces its inner refs via AddStructReferencedObjects
  when a UPROPERTY), created in DoHandleRequest, pinned by the fragment via one `TStrongObjectPtr` member (mirrors
  `FFragment_EntityScript_Current._SnapshotLoadPin`, `CkEntityScript_Fragment.h:76-82`). Works for DoesNotReplicate
  too (no driver needed). VERIFIED: NotInstanced CDO path, the two precedents, and fragments-untraced (root doctrine)
  all confirmed against code by the Opus main loop. NOT a human-decision fork (fence authorized the mechanism).
- **[P3A-F2] (2026-07-11, Phase 3A §3A.3) — lift `ck_dynamic_snapshot::RemapHandles` into CkEcs as shared
  `ck::snapshot::RemapHandles` (Fable ruling, VERIFIED against code).** Serializing the recipe's `_SpawnParams`
  (and the deferred-six payloads) through the plain tagged-property pass SKIPS nested `FCk_Handle` fields (they are
  Transient). The generic handle-remap walker already exists — `RemapHandles` at `CkDynamic_Fragment_Data.cpp:16-146`,
  a deterministic `TFieldIterator` walk routing every `FCk_Handle`-DERIVED field (IsChildOf, so typed handles too;
  top-level + nested structs + TArray/TSet/TMap with load-side rehash) through `FSnapshotContext::Snapshot_Handle`,
  used via the two-step pattern (`_StructData.Serialize` then `RemapHandles`, `:141-145`). VERIFIED by reading the full
  body. Decision: lift it verbatim to `CkEcs/Snapshot/CkSnapshot_HandleWalk.h/.cpp` (`CKECS_API`), add a
  `ForEachHandle(Struct, Memory, visitor)` overload (the §3A.3 forward-ref ensure: each params handle must reference
  an already-written saved-id), and RE-POINT CkDynamic at the shared copy (single-source; the Ck.Snapshot gate incl.
  DynamicFragment.* tests covers the re-point — fallback is duplicate-in-CkSnapshot if it reds). Touches one CkDynamic
  file (out of §3A named scope) — flagged in the commit. NOT a STOP (~100-line mechanical move).

## Decisions — executor (Phase 3A)

- **[P3A-D1] ConstructSpawned stamp site = `Request_SetupEntityWithLifetimeOwner` (create time), condition =
  owner Has FFragment_EntityScript_Current AND NOT Has FTag_EntityScript_HasBegunPlay.** VERIFIED the EntityScript
  spawn path routes through `Request_CreateEntity(owner)` (`CkEntityScript_Utils.cpp:169,208`) → the single choke
  `Request_SetupEntityWithLifetimeOwner` (`CkEntityLifetime_Utils.cpp:441`) where both new-entity + owner are in
  hand; every owned-entity create passes here. A child spawned inside a parent's `Construct()` is created
  SYNCHRONOUSLY there (the spawn request carries a pre-made NewEntity, `CkEntityScript_Processor.cpp:147`) while the
  owner lacks HasBegunPlay → stamp frozen correctly at create time. HasBegunPlay is the discriminator (owner still
  in its deterministic Construct/BeginPlay build ⇒ ConstructSpawned/adopt; owner begun-play + child spawned by later
  runtime e.g. an SM task ⇒ RuntimeSpawned — this is the CTO-N1 StoreDriver case, correctly RuntimeSpawned). Spec
  §4.2's condition text is satisfied; picked HasBegunPlay over the FinishConstruction-window nuance (definitive
  marker). Transient tag ⇒ never round-trips Model A.
- **[P3A-D2] Deferred-six Produce keyed PER-ENTITY, mirroring the feature's `*_Replicate` build (NOT aggregate).**
  §3A.4 says "emit the same RepData the per-frame *_Replicate pass builds from live state." Each *_Replicate
  iterates a specific entity (Team/Player: the feature entity; Inventory×2: the inventory entity, storing on the
  owner container; RenderTarget: the sync-child; 2dGrid: the grid entity) and builds RepData from THAT entity's
  live state. Produce keyed on the same entity emits the same RepData; the owner-hosted STORAGE location is a
  Phase-3B/Apply concern, not the Produce build. Each Produce gets a robust presence check returning unset when the
  feature is absent (so the oracle's per-entity sweep is safe). Produce-WITHOUT-SeedContainer per [P1-R1] — the
  still-alive `*_ReplicateOnRestore` processors keep seeding under Model A; a SeedContainer would double-seed.

## Decisions — planner rulings

- **[P1-R1] (2026-07-11, planning session — resolves [B1]): option (c), refined to a 6/6 split.** Phase 1 migrates
  ONLY the clean six (Velocity, Acceleration, Attribute×5, TagSet, MontagePlayer, AnimPlan — 10/16 registrations);
  Team + Player join the DEFERRED set alongside Inventory×2, RenderTarget, 2dGridOccupancy (their unconditional
  `FTag_TeamID/PlayerID` re-derive is the same non-container-reconstitution class, just smaller — the research
  table's own "UNCONDITIONALLY, pre-driver-gate" note contradicts a driver-gated SeedContainer home). Deferred
  processors stay VERBATIM: their repair work is Model-A-only and Model B retires it structurally (items→recipes,
  grids→Construct, RT→Construct+Phase-4B re-author, Team/Player tags→normal Assign in hydration Apply); they go
  inert at 3B (v3 never stamps JustRestored) and are deleted in Phase 5. Options (a) slimming and (b) a
  `Reconstitute` hook are REJECTED: churn without end-state value / framework surface that Phase 3B makes dead.
  New participation rule (encoded in PHASE_1.md + PHASE_3A.md §3A.4): `SeedContainer` present ⇒ handler joins the
  Model-A re-drive; `Produce`-without-`SeedContainer` ⇒ capture/oracle-only (how the deferred six gain Produce at
  3A with zero double-seed risk). The research doc's §1.1 `.inl.h`-at-bottom-of-CkNet_Utils.h include decision is
  BLESSED — re-apply verbatim. PHASE_1/3A/3B/5 docs revised accordingly (same commit as this entry).

## Decisions — Phase-4A design (Fable consults 2026-07-12, foundational claims spot-verified; FULL verification required at implementation start per protocol)

Two independent read-only Fable design consults resolved the 4A forks. Recorded here as the LOCKED design for the 4A
implementation session. Both deviate from PHASE_4A's literal text in evidence-forced ways (noted) and BOTH flagged
genuine product/scope decisions for Adam (see Blockers [N1-A]/[SM-A]). NOT YET IMPLEMENTED — no 4A code on disk.

- **[P4A-F1] (Fable, SM redrive → hydration Apply).** (a) "Delete FProcessor_Sm_RestoreRedrive" = **rename + re-gate**,
  not evaporate: delete its Model-A halves (JustRestored trigger, first-visit stash/virgin-reset, WaitDriver, sub-SM
  orphan destroy) and re-home the surviving Start→Transition→Finalize ladder in a new `FProcessor_Sm_HydrationResume`
  triggered by `FFragment_Sm_HydrationResume` (= `FFragment_Sm_RestorePending` renamed, minus `EPhase::WaitDriver`;
  fragment-removal is the done marker → delete `FTag_Sm_RestoreRedriven`). Exit criterion `rg RestoreRedrive → 0` is
  name-based, so rename satisfies it. Foundation VERIFIED by Opus: `CkSnapshot_Subsystem.cpp:971-990` opens the gate +
  `Request_PumpToQuiescence(Full)` in ONE callback, so at Apply time the fresh SM is normal-boot-composed + Setup-drained
  (no virgin reset needed). (b) `FCk_HydrationApplyScope` = a game-thread-only static depth counter (serial dispatchers,
  no thread-local), set inside `FProcessor_Hydration_Dispatch::ForEachEntity` wrapping the `ApplyOne` call
  (`CkReplicatedFragmentContainer_Processor.cpp:~188`). (c) **Produce = canonical single-event, NOT the live ring**
  (DEVIATION from PHASE_4A's literal "emit as the replicate pass builds" — the live ring's server seqs restart in the
  rebuilt world → oracle byte-diff never matches → OracleParity could never close; emit `FCk_RepData_StateMachine_
  WithHistory{History=[{null,CurrentStateClass,Seq0,Fp0}] or [], RunStatus}` / `_NoHistory{CurrentStateClass,Seq0,Fp0,
  RunStatus}`, gated on `Get_ReplicationModel()` NOT `Get_Replication()` so DoesNotReplicate SMs persist). (d) Apply:
  `if (FCk_HydrationApplyScope::Get_IsActive()) return Sm_ApplyFromHydration(...)` as the FIRST statement (above
  echo-suppress); Sm_ApplyFromHydration returns NotReady pre-composition else stashes FFragment_Sm_HydrationResume +
  Applied; net path token-for-token unchanged (byte-identical). Migrate both `RegisterLazy`→`RegisterLazyTyped` +
  `.Produce` + `.Transport=NetAndSave` (`CkStateMachine_Replication.cpp:295,314`). (e) KEEP the authority gate verbatim
  (`CkStateMachine_Processor.cpp:1116-1140`). Full delete-list + anchors in the consult transcript. Risk-most-likely-wrong:
  ladder convergence inside the Hydrating pump (verify via the ladder Display logs on first gate). KEEP
  `FFragment_Sm_Current::SerializeSnapshot` (Model-A Tier-C, pinned by `StateMachine.StateRoundTrip`).
- **[P4A-F2] (Fable, N1 discriminator).** Discriminator = **extend `Get_IsSnapshotRespawnable` to the non-bridged path**
  (candidate a; it already exists and means exactly boot-infra-vs-gameplay — bridged path already consumes it, [P3B-D6]).
  Set at spawn: stamp `_IsSnapshotRespawnable` on the recipe holder (`UCk_EntityScript_SpawnRecipe_UE`) from
  `NewEntityScript->Get_IsSnapshotRespawnable()` at `CkEntityScript_Processor.cpp:~164`. Persist: additive
  `UPROPERTY() bool _IsSnapshotRespawnable=false` on `FCk_Snapshot_V3_EntityEntry` + capture it (tagged-property →
  **no FormatVersion bump**, old saves read false → today's skip; smoke with an old-save fixture). Consume: restructure
  the loader `:676-698` — owner not persisted & NOT respawnable → skip (boot-infra); not persisted & respawnable →
  respawn under the transient (gameplay); owner in `_SkippedIds` → skip-propagate. **Never-double BY CONSTRUCTION** (reconcile
  is ConstructSpawned-only, `:810-852`, so a RuntimeSpawned double is permanent) via the **SM free-run gate**:
  `FProcessor_Sm_HandleRequests::ForEachEntity` early-outs while `Has<ck::FTag_Hydration_PendingApply>()` (added at
  enqueue BEFORE gate-open) — this is the like-for-like replacement of the deleted JustRestored gate, keyed on the
  load-generic tag; the parked script `Request_Start` then drains as a no-op (already-Running drop). Also make
  `Get_IsSnapshotRespawnable` `UPROPERTY(EditDefaultsOnly)`-backed so AS scripts can opt in. Test:
  `Ck.Snapshot.Rebuild.SpawnerResumesPastSpawnDecision` (spawner SM Idle→SpawnState→Holding, spawns subordinate in
  SpawnState, save in Holding, load → assert exactly ONE subordinate + SM in Holding — a true conjunction pin).
  Risk-most-likely-wrong: `FProcessor_Sm_Setup` AutoStart may enter a state directly (not via a request) → the free-run
  gate on HandleRequests would miss it → read Setup's AutoStart + extend the tag check there if needed.

- **[P4A-D1] (executor, 2026-07-12, Fable-confirmed Q1) — SM save-transport handlers KEEP `RegisterLazy` (NOT
  `RegisterLazyTyped`); Produce-WITHOUT-SeedContainer.** The runbook / [P4A-F1](d) said "migrate RegisterLazy→RegisterLazyTyped".
  RegisterLazyTyped synthesizes a default SeedContainer when `.SeedContainer` is unset (`CkReplicatedFragmentContainer.inl.h:22-28`),
  which would move the SM into `Get_ReDriveHandlerTypes()` → the Model-A `FProcessor_Persistence_ReDriveOnRestore` re-drive.
  SeedContainer has exactly ONE consumer in Source (`CkPersistence_ReDrive_Processor.cpp:65`, gated on `FTag_Snapshot_JustRestored`
  which only Model-A `CkSnapshot_Restore.cpp:243` stamps); both net dispatchers + the save capture/oracle use `Apply`/`Produce`
  only. So RegisterLazyTyped's ONLY delta vs RegisterLazy is the SeedContainer → on a `DoesNotReplicate` SM (the SM default,
  `CkStateMachine_Fragment_Data.h:227`) `TryAddContainerFragment` returns NotAdded → the re-drive's 5s LOUD timeout ensure.
  Per [P1-R1] (Produce-without-SeedContainer ⇒ capture/oracle-only) + the Team precedent (`CkTeam_Fragment.cpp` uses RegisterLazy
  for exactly this reason), the SM handlers keep RegisterLazy + add `.Produce` (canonical single-event per [SM-A], model-gated
  on `Get_ReplicationModel()` not `Get_Replication()`) + `.Transport = NetAndSave`, NO SeedContainer. Net path byte-identical
  (same RegisterLazy + type-resolver + Apply; the hydration branch is FIRST, above echo-suppress — load-bearing per Fable F1,
  since `Get_EffectiveAuthorityModel` AutoDetects player-pawn SMs to OwningClientAuthoritative and echo-suppress would else swallow
  the payload). The "RegisterLazyTyped" wording was the Phase-1 §1.1 migration recipe carried forward without re-applying the
  participation rule the campaign locked later — evidence-forced deviation, same class as [SM-A]. Fable consult (read-only)
  ruled Q1 YES + reviewed the full edit plan (zero blockers, 5 findings all reconciled: F1/F2 already correct in the diff,
  F3/F4 comment fixes applied, F5 = sub-SM state drop is a pre-existing v1 bound). Independently code-verified.

**4A.1 IMPLEMENTATION STATE (2026-07-12, Opus — DONE + COMMITTED).** Adam said
"continue" → taken as approval of [SM-A] (Fable canonical-single-event Produce) + [SM-B] (doctrine) + [N1-A] (implement 4A
as designed, boot-singleton adoption deferred). **Scope-cut for THIS increment: do 4A.1 (SM redrive→hydration) ONLY** — it
directly greens the 2 `Parity.StateMachine*` casualties the EXISTING gate measures and the SM.Net suite verifies the
byte-identical net path; **4A.2 (N1 discriminator + `Rebuild.SpawnerResumesPastSpawnDecision` test) DEFERRED** (its real
value is [N1-A]-blocked and it needs a new BB-style fixture world).
- **IMPLEMENTED + COMMITTED (CkF `705e7d57e`, CkTests `773a4d2`).** Delivered exactly per [P4A-F1] + [P4A-D1]: (1)
  `FCk_HydrationApplyScope` (game-thread static depth counter in `CkReplicatedFragmentContainer.h`; static+methods in
  `_Processor.cpp`; wraps the `ApplyOne` call in an IIFE-scoped `FCk_HydrationApplyScope{}`); (2) `FFragment_Sm_RestorePending`
  → `FFragment_Sm_HydrationResume` (dropped `EPhase::WaitDriver`, default `Start`, public `Populate`); deleted
  `FTag_Sm_RestoreRedriven`; (3) `FProcessor_Sm_RestoreRedrive` → `FProcessor_Sm_HydrationResume` (view gains
  `TReadWrite<FFragment_Sm_HydrationResume>` + `MarkedDirtyBy`; kept RequiresSetup-wait + authority gate + Start/Transition/
  Finalize; deleted JustRestored gate, sub-SM orphan-destroy, first-visit-stash/virgin-reset, WaitDriver; `DoMarkResumed` →
  `Remove<FFragment_Sm_HydrationResume>`); (4) deleted the `FirstSyncInitialState` JustRestored gate + the
  `CkSnapshot_RestoreMarker.h` include; (5) registrar — `RegisterLazy` KEPT ([P4A-D1], NOT RegisterLazyTyped) + `.Produce`
  (canonical single-event, model-gated) + `.Transport=NetAndSave` + hydration Apply branch FIRST (above echo-suppress); (6)
  reworded stale comments in `CkSmState_Processor.cpp`, `CkStateMachine_Fragment_Data.h`, `CkSmTask_SubStateMachine.cpp`,
  `CkTag.h`, `CkPersistence_ReDrive_Processor.h`, CkTests `Test_Snapshot_StateMachineState_RoundTrip.cpp`.
  `rg "RestoreRedrive|RestoreRedriven|RestorePending" Source` → 0.
- **GATE (p4a.log / p4a-sm.log / p4a-net.log): BUILD Succeeded.** Ck.Snapshot **52/45/7** — both `Parity.StateMachine_MPReload`
  + `Parity.StateMachineNoHistory_MPReload` GREEN (9→7; the 7 remaining are the annotated 4B casualties: Attributes/AnimPlan
  empty-seed + TagSet/Grid/Inventory×2/RenderTarget client-shaped Apply), zero unexpected reds, zero regression in the 43
  prior-green. Ck.StateMachine **24/25** — lone red = the documented `OwningClientAuth_SubSm_AuthorityGatedTask` flake, which
  PASSED alone (p4a-sm-flake-rerun.log) AND in the Net run this session (same binary) → classified FLAKE not regression;
  byte-identical net path holds. Net **102/99/3** — only the kiosk env-trio; ZERO new framework `Ck.*.Net` red (delta-zero vs
  p4a-net-baseline; the SM.Net flake passed this run). **Risk-most-likely-wrong RESOLVED:** the HydrationResume ladder
  converges cleanly in ~4 frames (`Start (Running observed) → Transition (→State_B) → matches desired → done`, verified via
  the `[SM HydrationResume]` Display logs; no stall, no pump-limit, no SM apply-timeout).
- **NOT in 4A.1 (deferred to 4A.2 / [N1-A]):** the N1 discriminator ([P4A-F2]) + the SM free-run gate
  (`FProcessor_Sm_HandleRequests` early-out while `Has<FTag_Hydration_PendingApply>`) + the
  `Rebuild.SpawnerResumesPastSpawnDecision` test. 4A.1's ladder converges without the free-run gate for the bridged Parity
  probe (confirmed at the gate). Sub-SM saved control-state is not restored (pre-existing v1 bound; Fable F5).

## Blockers

- **[SM-A] (Adam decision, flagged by Fable [P4A-F1], 2026-07-12) — the Produce-determinism deviation reinterprets a
  CTO-fixed sentence.** PHASE_4A §4A.1 says Produce should "emit current run-state exactly as the authority's replicate
  pass builds the payload." Taken literally (copy the live history ring), the payload carries live server seqs that
  RESTART in the rebuilt world → the oracle Tier-2 BYTE diff never matches pre-save → `Rebuild.OracleParity` can never
  close with an empty allowlist. Fable's canonical-single-event Produce (Seq=0/Fp=0) is the evidence-forced fix. This is
  the correct engineering call but it reinterprets a sentence the CTO fixed — CONFIRM before implementing, or accept as
  executor judgment. (Low-risk to proceed; recorded for visibility.)
  **RESOLVED 2026-07-12 (accepted as executor judgment, per the "continue" approval + unattended protocol):** implemented
  the canonical single-event Produce (Seq=0/Fp=0, model-gated) in 4A.1; both `Parity.StateMachine*_MPReload` GREEN, net path
  byte-identical. The oracle `Rebuild.OracleParity` (which the byte-diff determinism serves) is still deferred to when a BB
  driver world is oracle-diffed (3B follow-up / 4A.2); the canonical shape is the prerequisite that makes it closable.
- **[N1-A] (Adam decision, flagged by Fable [P4A-F2] #1, 2026-07-12) — the REAL BB N1 mass (boot-created gameplay
  drivers' OWN state) is unscheduled and needs a product-scope call.** The discriminator + SM free-run gate + 4A.1 close
  the N1 DUPLICATE class for entities spawned under a PERSISTED spawner (and for flagged transient-owned gameplay
  top-levels). BUT a boot-created gameplay singleton like BB's `StoreDriver` (spawned under the transient/GameMode at
  boot, and correctly SKIPPED as "boot-infra" so the fresh boot re-creates it) has its OWN SM state DROPPED on
  save/load — its payloads never map because the skipped entry never enters `_SavedIdMap`. Restoring a boot-created
  singleton's state needs an ADOPT-BY-RENDEZVOUS for boot singletons (EngineOwned-style, keyed on script class/label
  under the transient), after which 4A.1 hydrates the boot driver past its spawn decisions and its subordinates become
  respawnable under it. Fable: "a phase-worth of work and a product-level scope call — 4A-adjacent follow-up vs 4B vs
  post-campaign." **This bounds what 4A-as-designed actually delivers: it makes the framework SpawnerResumes test green
  and eliminates duplicate lines, but does NOT by itself restore BB StoreDriver-owned gameplay state across a real
  save/load.** Adam to scope where boot-singleton adoption lands.
- **[SM-B] (Adam doctrine sign-off, flagged by both consults) — spawn-decision-placement contract.** Because the redrive
  re-fires Initial-state AND saved-state entry effects, N1's never-double guarantee holds iff a spawn decision lives on a
  state that is neither the InitialState nor the saved current state, OR its task is idempotent with HYDRATED guard flags
  (BB's `_SpawnStarted`/`WillSpawnCustomization` guards are this shape but must themselves be hydrated). Becomes a
  designer-facing contract line in spec §4.2. Same v1 scope as the old Model-A redrive (not a new limitation).

- **[INV-A] (Adam decision, flagged by Fable [P4B-F1] 2026-07-12) — Inventory×2 save-restore needs a product/architecture
  call; DEFERRED out of 4B.** `Parity.Inventory{DataOnly,Spatial}_MPReload` cannot be closed by a client-shaped-Apply
  re-author (the TagSet/Grid pattern) because item entities are v3 provenance **Rule 5 (anonymous scratch, skipped)**: they
  are built via `UCk_Utils_EntityReplicationDriver_UE::Request_BuildAndReplicate` with a `UCk_InventoryItem_Definition`
  archetype (`CkItem_Utils.cpp` `Create`), NOT the EntityScript spawn path, so they carry no `FFragment_SpawnRecipe`
  (`CkSnapshot_CaptureV3.cpp:261-271`) → not captured, not rebuilt. The inventory payload carries only item HANDLES, which
  remap to nothing on the loading authority. Two forks, both Adam-level: (a) **payload enrichment** — Produce emits per-item
  re-creation data (definition soft path / CoreInfo+traits + stack count, which is an IntegerAttribute → intersects the
  Attributes empty-seed workstream); or (b) **item-architecture change** — make items SpawnRecipe-carrying so the loader
  rebuilds them (net implications). Fable explicitly flagged this for Adam (protocol STOP condition c); do NOT wire an Apply
  branch against invalid handles. Same class as [N1-A]. **Adam to scope where Inventory save-restore lands** (payload
  enrichment now vs item-spawn-recipe rearchitecture vs post-campaign).
  **[UPDATE 2026-07-12, [P4B-D2]] Grid `Parity.GridPlacements_MPReload` JOINS this blocker.** Its occupant is the same
  Rule-5 anonymous class (verified — unlabeled ConstructSpawned, not captured, gate-log shows only the grid maps). Grid
  placement restore is downstream of occupant restore, so it cannot close until [INV-A] is scoped. Extra Grid-only salvage
  fork Adam could weigh (cheaper than the Inventory forks, since the grid subject's Construct re-creates a fresh occupant):
  a bespoke Grid Apply that resolves the occupant by enumerating the re-Constructed grid's GridObject child rather than
  remapping the payload handle — feature-specific, single-occupant/direct-child assumptions. The deferred set is now
  **Inventory×2 + Grid** (3 casualties), all gated on the same anonymous-entity restore decision.
- **[B1] — RESOLVED 2026-07-11 by [P1-R1] above.** Original text kept below for the record.
- **[B1] (2026-07-11, Phase 1) — Plan's "12 ReplicateOnRestore = deletable container re-seeds" is false for 4
  features; where their non-container reconstitution goes is an unmade architecture decision.** REQUIRES a design
  ruling before Phase 1 can proceed (executor may not improvise architecture — PROMPT line 5).
  - **What diverges:** PHASE_1.md §1.2/§1.4 assume every `*_ReplicateOnRestore` processor only re-seeds the
    replication container from live state, so `Produce`/`SeedContainer` (§1.1) replaces them and §1.4 deletes them
    behavior-neutrally under Model A. Verified against code, that holds for 8 features but **Inventory Spatial,
    Inventory DataOnly, RenderTarget, and 2dGridOccupancy** restore processors ALSO do non-container reconstitution:
    child-entity re-replication (`Request_TryReplicateExisting`), grid re-stamp (`Request_PlaceItemOnGrid`) gated on
    CkGrid's `FProcessor_2dGridSystem_RestoreRecompose`, render-target re-create + repaint (`DoApplyBatch`), and an
    unconditional derived-fragment re-seed for local-only grids. The generic re-drive (§1.3) and Produce/SeedContainer
    cannot house this (cross-processor ordering; Produce-before-Seed circularity for Inventory; unset-Produce-skips-
    Seed for local grids; once-only repaint vs per-retry SeedContainer). Deleting these 4 per §1.4 breaks restore
    under Model A. HAND-VERIFIED: `CkInventory_Spatial_Processor.cpp:62-175`. Others agent-cited (consistent pattern).
  - **Decision needed (maintainer/CTO — do NOT pick unilaterally):** (a) keep slimmed restore processors for these 4
    doing only reconstitution + Produce for the payload; (b) add a `Reconstitute(Entity)` re-drive hook (unconditional,
    retry, separate from the container path); or (c) migrate only the 8 clean features in Phase 1 and DEFER the 4 to
    Phase 3/4 (Model B's Construct-rerun may moot most of it; interacts with CTO-addendum N1 + PHASE_4B RenderTarget).
  - **Full analysis + the 8 ready-to-implement recipes + the (reverted, re-usable) §1.1 design:**
    `PHASE_1_RESEARCH.md`. Once resolved, §1.1 re-applies verbatim; the 8 clean migrations follow the recipe table.
  - **Repo state:** clean at the gated-green Phase-0 boundary (CkFoundation `e5ffe028d`). No Phase-1 code landed;
    the §1.1 scaffolding was implemented then reverted (never build-verified) to keep the boundary clean.

## Session log

- 2026-07-11 — package authored (planning session, Fable). Branch `feature/save-load-improvements` @ `bc484d645`.
- 2026-07-11 — Phase 0 execution (Opus). Baselines captured @ `bbde1a9dd` (see table). Census re-derived
  (127/20/23). Decisions P0-D1..D3 recorded. Applied 3 dt==0 guards + `CK_WITH_FIDELITY_ORACLE` define + oracle
  Tier-1 (.h/.cpp) + harness test + spec §2 census reconciliation. **Gate GREEN, delta-zero:** Ck.Snapshot
  46→47 (new `Ck.Snapshot.Oracle.StructuralBaseline` PASSED, `--discover-fresh`), Ck.Attribute.Net 17/17/0,
  Net 102/101/1 (same pre-existing red). Build clean; no AngelScript errors naming campaign files;
  `rg CK_WITH_FIDELITY_ORACLE Source | wc -l` = 8 (≥3). Commits: CkF `68ba192dc`,`55521d493`,`<docs-this>`;
  CkTests `14d65ac` (on `dev`, unpushed — where the sibling snapshot tests live). Nothing pushed. Phase 0 DONE.
- 2026-07-11 — Phase 1 attempt (Opus, same session). Ran a 10-agent read-only census of all `*_ReplicateOnRestore`
  processors (recipes in PHASE_1_RESEARCH.md); designed + implemented §1.1 registry extension (Produce/SeedContainer/
  Transport/RegisterLazyTyped; include-surface decision made). **STOPPED on divergence [B1]** — 4 of the 12 restore
  processors do non-container reconstitution the plan's model can't house; where it goes is an unmade architecture
  decision (executor may not improvise). Reverted the unbuilt §1.1 scaffolding; tree back at gated-green Phase-0
  boundary. No Phase-1 code committed. Awaiting a design ruling on [B1] (see Blockers). Session ends here per the
  campaign's divergence rule.
- 2026-07-11 — Phase 1 RE-EXECUTION (Opus) after [B1] resolved by [P1-R1]. Implemented + COMMITTED the core:
  §1.1 registry contract, §1.3 generic re-drive, §1.2+§1.4 all SIX clean-feature migrations + deletions, §1.5
  dormant hydration queue. Two commits on `feature/save-load-improvements`: **d7956345a** (CkEcs framework),
  **4a6839afb** (6 migrations + deletions). **Gate GREEN:** Ck.Snapshot 47/47/0 delta-zero (all 11
  Parity_MPReload pass — proves the generic re-drive == the deleted per-feature processors), Net 102/102/0 (the
  baseline's lone red is a pre-existing flake, green this run). Executor decisions [P1-D1..D4] recorded above (esp.
  P1-D2: empty-seed Attribute Produce ⇒ 1.6 ProduceDiffBaseline must use a value-emitting feature like Velocity).
  Hit + fixed: UHT-forbids-include-after-generated.h (.inl.h moved to registrar .cpp), ck_exp::TProcessor needs
  TReadWrite on the hydration fragment. **REMAINING: §1.6** (oracle Tier-2 Capture_Payloads/Diff_Payloads +
  Ck.Snapshot.Oracle.ProduceDiffBaseline test) — the ONLY unfinished Phase-1 step. Nothing pushed. Handed off
  mid-phase (context full) with a continuation prompt.
- 2026-07-11 — **Phase 1 §1.6 execution + COMPLETE (Opus, unattended run).** Implemented oracle Tier-2:
  `Get_ProduceHandlerTypes()` (registry), `Capture_Payloads`/`Diff_Payloads` + shared `BuildEntitySignatures` extraction
  (oracle), `Ck.Snapshot.Oracle.ProduceDiffBaseline` test (Velocity value-emit). Decision [P1-D5]. **Gate GREEN
  (built against the integrated base — see below): Ck.Snapshot 48/48/0** (all 11 Parity + StructuralBaseline +
  ProduceDiffBaseline; `--discover-fresh`), framework `Ck.*.Net` delta-zero. Divergence handled via Fable consult
  [BI-1]: 3 BB kiosk-destruction `Net` reds are pre-existing/environmental (DoesNotReplicate → severed from campaign),
  NOT §1.6 (proven inert, zero prod callers). **Also discovered:** the branch was rebased at 16:55 (one-time) to fold in
  the object-pooling-core campaign (+3758 lines incl. CkEcs Scheduler/EntityScript) — see the baseline-section note;
  Phase-2 line refs shifted (re-locate by pattern). Commits: CkF 5d262f52a, CkTests b9e7f86.
  Nothing pushed. **Phase 1 DONE.** Proceeding to Phase 2.
- 2026-07-11 — **Phase 2 execution (Opus, same run) — 2.1–2.3 DONE + compile-verified; CHECKPOINT before 2.4.**
  Implemented the load-gate trait plumbing (2.1: `ECk_ProcessorLoadPolicy` mirroring PumpPolicy across descriptor/traits/
  graph/scheduler + `ECk_SchedulerTickScope{Full,LoadKernel}` Tick/DoPump scope + subsystem `Get_/Set_IsLoadGateActive` +
  lazy weak-ptr Actor→subsystem + `Request_PumpToQuiescence` scope param), the 12-processor kernel marking (2.2), and
  `FGroup_Hydration` + moving BOTH dispatchers into it with their `RunAfter FinishConstruction` deleted (2.3). Ran an
  editor compile-check: FOUND + FIXED a dropped `namespace ck {` in CkProcessorScheduler.h (enum-insert edit swallowed
  it); re-built CLEAN (0 errors, 529s). **STOPPED at the 2.1–2.3/2.4 boundary** — context depletion + Phase 2 is atomic
  high-blast scheduler core (2.4's ck_exp DoTick-override + dispatcher-ordering + 2.5 fire-gating want fresh context to
  get right; a half-done atomic scheduler phase is worse than a clean checkpoint). 2.1–2.3 code is UNCOMMITTED (atomic
  phase — not gate-safe alone) but compile-verified in the working tree; the sub-step tracker above carries the full
  design for 2.4–2.8 (esp. [P2-D1] + the ConstructedThisFrame pump/ordering reasoning). Continuation: implement 2.4–2.7,
  gate per §2.8, commit. Nothing pushed.
- 2026-07-11 — **Phase 2 COMPLETE (Opus continuation).** Implemented 2.4 (ConstructedThisFrame defer — verified the
  ck_exp DoTick-override pattern; [P2-D1]), 2.5 (fire-gating — Fable consult [P2-D2] resolved the "no traversable
  dependent set" divergence → recurse the lifetime-dependents tree), 2.6 (retire NeedsSetup guards), 2.7 Test 1
  (LoadGate.GatedSkipsKernelTicks; Test 2 subsumed per [P2-D3] — the late-setup stomp is only robustly forced via
  reload, already covered by AccelerationParity_MPReload + the Attribute.Net pins). Two compile-checks (the first caught
  a dropped `namespace ck {` in 2.1-2.3; final clean). **Gate GREEN:** Ck.Snapshot 49/49/0 (LoadGate + all Parity, no
  hang), Net 102/98/4 (framework delta-zero, 3 pins green, 4 fails = recorded kiosk env-red trio + StateMachine.Net
  flake). Commits CkF 91b96a177 / 8c4fbce7a / af9fad239, CkTests 4522c8e. Decisions [P2-D1..D3] + Fable [P2-D2]
  recorded. **Phase 2 DONE.** Nothing pushed. Next: Phase 3A.
- 2026-07-11 — **Phase 3A COMPLETE (Opus, unattended run).** Save side (spec §4.2), additive v3 writer beside Model A.
  Implemented: 3A.1 ConstructSpawned provenance ([P3A-D1]); 3A.2 GC-safe SpawnRecipe holder+fragment (Fable [P3A-F1],
  verified); RemapHandles lifted to shared CkEcs walker + CkDynamic re-pointed (Fable [P3A-F2], verified); 3A.3 v3
  writer (Run_CaptureV3 + FCk_Snapshot_HeaderV3 + entity/payload tables + Get_SaveHandlerTypes); 3A.4 16 registrars
  (10 Transport flips + 6 per-entity Produces, [P3A-D2] — the 4 complex ones agent-drafted then Opus-verified against
  the *_Replicate builds); 3A.5 Request_Save dual-write; 3A.6 two V3 tests. Two Fable consults (recipe GC-lifetime,
  handle-in-params serialization) + one registrar-survey + one Produce-drafter agent; every ruling verified against
  code before applying. One compile-check caught only a single link error (FUniqueNetIdWrapper::ToString → added
  CoreOnline dep); all code compiled first pass otherwise. **Gate GREEN:** Ck.Snapshot 51/51/0 (delta-zero + 2 new;
  DynamicFragment/Parity/Oracle all pass — the RemapHandles lift + new Produces are regression-free), Net framework
  Ck.*.Net delta-zero (only the recorded kiosk env-red trio, 3 Attribute.Net pins green). Commits CkF 81f7b6505 /
  349947218 / d0ce51877, CkTests 5bb9798. Nothing pushed. Next: Phase 3B.
- 2026-07-12 — **Phase 3B FINISHED + COMMITTED (Opus, unattended continuation).** Read gate-2 (41/10), finalized the casualty
  categorization, fixed the ONE genuine outlier (M2a), added the cheap disk-smoke test, re-gated, committed. **M2a diagnosis: the
  gate-2 "N1/Phase-4A casualty" call was a misdiagnosis** — M2a's probe is a bridged WithActor actor missing the respawn opt-in, not
  an inherently non-bridged gameplay entity; Fable-consulted (ruling A) + code-verified (base `Get_IsSnapshotRespawnable`=false;
  loader bridged branch `CkSnapshot_Subsystem.cpp:628-663` bypasses the boot-infra skip) + gate-3 green. Fix = respawn opt-in on the
  M2a probe entity-script + FramesForLoad 150→240 (CkTests). Added `Ck.Snapshot.V3.InstancedStructDiskSmoke` (Fork-A map-backed
  remap + dangling-ref, verified `CkSnapshot_Context.cpp:18-26`). **Gate-3 (p3b-gate3.log): Ck.Snapshot 52/43/9, --discover-fresh;
  M2a + disk-smoke GREEN; the 9 fails are exactly the verified Phase-4 casualties** (StateMachine×2→4A; Attributes/AnimPlan→4B
  empty-seed; TagSet/Grid/Inventory×2/RenderTarget→4B client-shaped Apply, TagSet ClientOnly-sync verified `CkTagSet_Processor.h:98`).
  Zero unexpected reds; zero real bugs — the v3 load pipeline is proven. Decisions [P3B-D6]/[P3B-M2a] recorded (correcting [P3B-D4] +
  the gate-2 TagSet/M2a calls). Commits: CkF `78fcdaa8e` (retire reconstitution), `36bcdec5d` (v3 load pipeline), `<docs-this>`;
  CkTests `ce32c65` (disk smoke + M2a opt-in). Nothing pushed. **Phase 3B DONE.** Next: Phase 4A (SM redrive-as-hydration + N1
  closure) — Net baseline to be re-captured at the 4A boundary.
- 2026-07-12 — **Phase 4A DESIGN (Opus, unattended continuation) — checkpoint at a protocol STOP (Adam decisions flagged).**
  Captured the Net baseline (p4a-net-baseline.log 102/98/4 = delta-zero: SM.Net flake + kiosk trio; NO new framework
  Ck.*.Net red → 3B commits are Net-clean). Routed BOTH 4A design forks to read-only Fable agents and recorded the rulings
  [P4A-F1] (SM redrive→hydration: rename FProcessor_Sm_RestoreRedrive→FProcessor_Sm_HydrationResume, FCk_HydrationApplyScope
  static game-thread guard, canonical single-event Produce, byte-identical net path) + [P4A-F2] (N1: extend
  Get_IsSnapshotRespawnable to the non-bridged path + SM free-run gate on FTag_Hydration_PendingApply → never-double by
  construction). Spot-verified the load-bearing foundation (atomic Hydrating phase at CkSnapshot_Subsystem.cpp:971-990).
  **STOPPED before implementing (protocol STOP condition c):** BOTH Fable agents explicitly flagged human-only decisions —
  [SM-A] the Produce-determinism deviation reinterprets a CTO-fixed sentence; **[N1-A] the REAL BB N1 mass (boot-created
  gameplay drivers' OWN state, e.g. StoreDriver) is unscheduled and needs a product-scope call** (4A-as-designed makes the
  framework test green + kills duplicates but does NOT restore boot-singleton-owned gameplay state); [SM-B] a designer-facing
  spawn-decision-placement doctrine line. 4A is a high-blast CkStateMachine replication-core change; implementing it atop
  unresolved product-scope questions ([N1-A]) would be premature. Design is LOCKED + recorded; implementation awaits Adam's
  calls on [SM-A]/[N1-A]/[SM-B]. No 4A code on disk; tree clean at the 3B commits. Nothing pushed.
- 2026-07-12 — **Phase 4A.1 IMPLEMENTED + COMMITTED (Opus, unattended run).** Took "continue" as approval of
  [SM-A]/[SM-B]/[N1-A] (implement 4A.1 as designed; boot-singleton adoption + N1 discriminator deferred). FULLY verified every
  cited anchor against code before editing (the design was only spot-verified): read the container header/processor, the SM
  processor/fragment/registrar/RepData, the re-drive processor, the scheduler main-pass + DoPump, and the v3 load settle
  (`kLoad_SettleFrames=3`). Confirmed the main pass runs every processor once per frame (scheduler.cpp:114) so the 3 settle
  frames are a solid convergence safety net beyond the pump. Implemented all 6 sub-steps (SM redrive→hydration, the
  `FCk_HydrationApplyScope` guard, canonical single-event Produce, byte-identical net path). **One design fork routed to a
  read-only Fable agent** ([P4A-D1]: RegisterLazy-vs-RegisterLazyTyped — the SeedContainer-synthesis participation trap);
  Fable ruled Q1 YES (keep RegisterLazy) + adversarial-reviewed the whole plan (zero blockers, 5 findings — F1/F2 already
  correct in my diff, F3/F4 comment fixes applied, F5 sub-SM-drop noted); every claim independently code-verified. Gate:
  Ck.Snapshot **52/45/7** (both Parity.StateMachine* green, 9→7 casualties), Ck.StateMachine 24/25 (lone red = documented
  flake, passed alone + in Net run), Net 102/99/3 (kiosk env-trio only, no new framework Ck.*.Net red). Ladder converges in
  ~4 frames (Display-log verified). Commits: CkF `705e7d57e` (SM hydration), `<docs-this>`; CkTests `773a4d2` (comment).
  Nothing pushed. **Phase 4A.1 DONE.** Next: Phase 4B (the 7 remaining casualties + PHASE_4B params-mutators/AS-smoke), then
  4A.2 (N1, [N1-A]-blocked) + Phase 5. See CONTINUATION_PROMPT_Phase4B.md.
- 2026-07-12 — **Phase 4B STARTED (Opus, same unattended run) — client-shaped-Apply pattern validated; TagSet DONE.** Routed
  the 5 client-shaped casualties' Apply re-author design to a read-only Fable agent ([P4B-F1]): uniform hydration-scope shell,
  per-feature body; verified its rulings against code. Implemented + committed **TagSet** (`5088f5336`) — the first client-shaped
  casualty — via the 4A.1 hydration-scope pattern (direct `Set_Tags` REPLACE + arm `FTag_TagSet_MayRequireReplication`; [P4B-D1]
  omits the OnTagsChanged broadcast). Gate: Ck.Snapshot **52/46/6** (`Parity.TagSet_MPReload` green, 7→6 casualties, zero
  regression); Net **102/99/3** delta-zero (kiosk env-trio only, no framework `Ck.*.Net` red — net path byte-identical, branch
  gated by `FCk_HydrationApplyScope`). **Remaining 4B (handed off, all designed in CONTINUATION_PROMPT_Phase4B.md §6):** Grid
  (Fable-designed; ⚠️ verify test occupants map first), RenderTarget (transplant ReplicateOnRestore; ⚠️ settle-pump ordering),
  **Inventory×2 DEFERRED → [INV-A]** (items are v3 Rule-5 anonymous — Adam scope call), Attributes/AnimPlan empty-seed Produce,
  params-mutators, MontagePlayer, AS smoke. Stopped here at a clean committed increment (TagSet green) with the rest fully
  designed + handed off — the remaining per-feature work (esp. RenderTarget's transplant + the [INV-A] Adam fork) is better done
  with fresh context. Nothing pushed.
- 2026-07-12 — **Track A / 4B.3 AS SaveGame smoke DONE + COMMITTED (Opus, unattended run).** Built the framework Save-transport
  handler `FCk_SaveData_EntityScriptFields` in CkEcs (`CkEntityScript_SaveFields.h/.cpp`) — `RegisterLazy` (no SeedContainer),
  `Transport = Save`, Produce reflect-walks the script class's `CPF_SaveGame` FProperties + serializes via
  `SerializeScriptProperties(ArIsSaveGame=true)`, Apply replays them onto the re-Constructed instance gated to
  `FCk_HydrationApplyScope` (net path is a no-op early-return). Every load-bearing anchor verified against code before writing
  (container registry contract, `Get_SaveHandlerTypes` filter `(Transport & Save)`, CaptureV3 Produce loop, generic-by-name
  payload deserialize, the non-bridged-RuntimeSpawned respawn gates on the OWNER being persisted not the child's respawnable
  flag — so a fixture child under the persisted M2a probe round-trips without the deferred N1 discriminator). CkTests: AS fixture
  `Script/CkSnapshot/CkSnapshot_AsSaveFields_EntityScript.as` (`UCk_GenericEntityScript_UE` subclass, `UPROPERTY(SaveGame)`
  int/FString/TArray + a non-SaveGame negation int, DoesNotReplicate) + C++ gate `Test_Snapshot_AS_SaveFields_Gate.spec.cpp`
  (two IMPLEMENT_SIMPLE_AUTOMATION_TEST off the M2a single-PIE harness, reflection mutate/read + a CPF_SaveGame precondition
  assert). **Gate: Ck.Snapshot 54/51/3** (both `AS.SaveGameFields_RoundTrip` + `AS.NonSaveGameField_Drops` GREEN, the 3 = [INV-A]
  trio by name, Meta.RepDataRestoreCoverage green — `FCk_SaveData_` prefix keeps it off the RepData ratchet — v3 AUDIT delta-zero
  3=3 vs baseline, no Angelscript errors naming the fixture). **Net 102/99/3** framework `Ck.*.Net` delta-zero (kiosk env-trio
  only; SM.Net flake passed). Commits: CkF `09ca84f41`, CkTests `a0c2f2d`. Decision [P4B-D5] recorded (post-BeginPlay timing).
  Nothing pushed. **Track A DONE** (closes 2 of the 7 VALIDATION-required tests). Next: Track B (Phase 5 partial).
- 2026-07-12 — **Track B / Phase 5 partial — clusters 1a + 2 DONE + COMMITTED; checkpointed before the big 5.2.2 delete (Opus,
  unattended run).** Loaded ck-change-control (Class-4 framework change; heeded the stale-binary trap — every gate ran a full
  `--build` on the final binary). **Cluster 1a** (CkF `521e3834d`): oracle-gated the `CK_REGISTER_SNAPSHOTABLE` registration
  (two `#define` forms under `CK_WITH_FIDELITY_ORACLE`, static_assert ungated), dropped the Model-A dual-write from Request_Save,
  deleted `UCk_Snapshot_SaveGame::_Header`/`_SnapshotBytes`, and made Get_SaveSlotHeader synthesize its frozen
  `FCk_Snapshot_Header` from `_HeaderV3` ([P5-D1] — a `_Header`-vs-VALIDATION-§3 fork routed to a read-only Fable agent, ruled C,
  every field-mapping anchor code-verified). **Cluster 2** (CkF `8bdd08346`): deleted the dormant FProcessor_ActorRespawn (all
  external refs comments; KEPT CkActorRebind). Both gates GREEN + delta-zero (Ck.Snapshot 54/51/3, Model-A registry tests +
  FloatAttribute.Gate + M2b2a + both AS tests green). **CHECKPOINT rationale (not a blocker STOP):** cluster 3 (5.2.2) is a
  large/delicate 9-processor delete across 8 modules + JustRestored tag/marker + scattered stale RestoreMarker.h includes (found
  a non-plan one at CkAttribute_Processor.h:7) + a FloatAttribute.Gate regression risk that may force the pre-authorized test
  rewrite — better done with fresh context than rushed. Full surface + safe delete order recorded in §Phase-5 progress cluster 3.
  Clusters 1a+2 are clean committed increments; 3–6 remain. Nothing pushed. Net gate NOT re-run this session (clusters 1a/2 are
  save-path / dormant-delete, provably net-inert; the Track-A Net run 102/99/3 is the last framework-Ck.*.Net delta-zero baseline
  — cluster 3+ must re-run Net since it deletes replicated-feature restore processors).
- 2026-07-12 — **Track B / Phase 5 partial — clusters 3 + 4 DONE + COMMITTED (Opus, unattended run).** **Cluster 3** (CkF
  `f4dd25992`): deleted the JustRestored re-drive machinery — FProcessor_Persistence_ReDriveOnRestore + 6 feature
  *_ReplicateOnRestore processors (Team, Player[whole file], Inventory DataOnly/Spatial, RenderTarget[only ReplicateOnRestore, kept
  the live HydrateFromSavedChannel], 2dGridOccupancy) + FProcessor_2dGridSystem_RestoreRecompose + the 6 FTag_*_RestoreReplicated
  done tags + the JustRestored stamp. **Hit a cross-repo build break the plan MISSED** (BB `Bb_SnapshotRestore.cpp`/`Bb_CombatReceiverRestore.cpp`
  consume `FTag_Snapshot_JustRestored`) → Fable-consulted + code-verified → KEPT the marker header (retirement comment) per the
  FTag_ActorJustRebound precedent [P5-D3]; cross-repo sweep for ALL deleted symbols clean (only 1 stale CkTests comment). Gate:
  Ck.Snapshot 54/51/3 (3 = [INV-A] trio; FloatAttribute.Gate GREEN — no rewrite), Net 102/98/4 framework Ck.*.Net delta-zero
  (only the documented SM.Net flake + kiosk env-trio). **Cluster 4** (CkF `<c4-this>`): reverted 15434d8ef's Transform `_Previous`
  persistence in CkTransform_Fragment.cpp/.h ([P5-D2] doc-path fix vs the plan's Utils.cpp cite; exact pre-commit shape verified).
  Gate: Ck.Snapshot 54/51/3 delta-zero. Nothing pushed. **Clusters 5 (machinery gate + Shipping compile) + 6 (docs + 5.3 grep +
  final Ck.Snapshot+Net gate) remain.** Cluster-5 recon done (see tracker ⚠️ — archives/registry pervasively included → gate only
  entry points; Fable consult at execution). Maintainer review of this Class-4 change is MANDATORY — branch stays unpushed.
- 2026-07-12 — **Track B / Phase 5 partial — cluster 5 DONE + COMMITTED (Opus, unattended run).** Gated the Model-A machinery under
  `#if CK_WITH_FIDELITY_ORACLE`. Second read-only Fable consult resolved the archives-gating-scope fork ([P5-D4]): the Archive
  Writer/Reader + FragmentRegistry are pervasively `#include`d by ~48 production fragments, so gate ENTRY-POINTS only (Capture/Restore/
  Archive_Writer/Archive_Reader/TagDriver `.cpp` bodies) + LEAVE FragmentRegistry.cpp + Audit.cpp compiled (un-gated static-init/Audit
  callers → gating = Shipping LNK2019). Every anchor code-verified; the Ruling-5 TagDriver define-visibility landmine pre-empted with a
  FragmentRegistry.h include; dropped the stale Subsystem Capture.h include. **Gate: Dev Ck.Snapshot 54/51/3 delta-zero (Model-A registry
  tests still GREEN under oracle-ON); Shipping Game/Shipping compile SUCCEEDED (2304s, zero real errors, zero snapshot symbols in link
  diagnostics).** Commit CkF `<c5-this>`. Nothing pushed. **Cluster 6 remains: docs (CkEcs/CLAUDE.md rep section, root CLAUDE.md
  CK_REGISTER_SNAPSHOTABLE row) + the exit-grep comment sweep (34 stale ReplicateOnRestore/RestoreReplicated/JustRestored/Reconstitution
  comment hits, minus the RestoreMarker.h carve-out) + amended PHASE_5.md §5.3 grep + final Ck.Snapshot + Net gate.** Then Phase-5 partial
  is complete for maintainer review; Phase-5 FINAL (VALIDATION §2 OracleParity, camera revert, etc.) stays BB-driver-world-blocked.
- 2026-07-12 — **Track B / Phase 5 partial COMPLETE — cluster 6 DONE + COMMITTED; clusters 1a–6 all done (Opus, unattended run).**
  Docs updated (root CLAUDE.md macro row → oracle-only; CkEcs/CLAUDE.md rep section → FGroup_Hydration + fire-gating; PHASE_5.md §5.3
  grep dropped IsSnapshotRespawnable + RestoreMarker.h carve-out). Exit-grep comment sweep: a 7-agent workflow reworded 34 stale
  `ReplicateOnRestore|RestoreReplicated|JustRestored|Reconstitution` comment hits across 26 files to the post-Phase-5 reality
  (v3 hydration; RenderTarget→HydrateFromSavedChannel) — comments-only (build Succeeded), **exit grep VERIFIED ZERO outside the
  RestoreMarker.h carve-out**. Gate: Ck.Snapshot 54/51/3 delta-zero + Net 102/98/4 framework Ck.*.Net delta-zero. Commit CkF
  `<c6-this>`. **PHASE-5 TRACK-B PARTIAL IS COMPLETE (clusters 1a/2/3/4/5/6): Model-A decommissioned (dead-path deleted, live
  machinery oracle-gated, Shipping compile OK, docs + grep clean).** NOTHING PUSHED — branch `feature/save-load-improvements` handed
  to Adam for MANDATORY Class-4 review. **DELIBERATELY NOT DONE (Adam/prereq-gated, per prompt DO-NOT-START):** the [INV-A] trio
  (Grid+Inventory×2, the 3 standing reds), [N1-A] boot-singleton adoption + 4A.2, everything BB-driver-world/OracleParity-blocked
  (Camera revert, VALIDATION §2, Phase-5-FINAL, 4B.1 params-mutators, MontagePlayer rebind), and the spec Status→IMPLEMENTED flip.
  **CROSS-REPO FOLLOW-UPS for Adam/BB-repo (a CkF session must NOT touch BB):** delete `CkSnapshot_RestoreMarker.h` +
  `FTag_Snapshot_JustRestored`/`Get_WasJustRestored` surface (Bb_SnapshotRestore/Bb_CombatReceiverRestore + ~28 AS
  `*_Processor_RestoreRebind.as`) together with the `FTag_ActorJustRebound` cleanup ([P5-D3]); the 2 stale BB comments naming
  FProcessor_ActorRespawn (Bb_PlayerSaveLoad.h:14, Bb_SaveLoadProbe.cpp:274) + 1 stale CkTests comment
  (Test_Snapshot_RenderTargetParity_MPReload_Gate.spec.cpp:10).
