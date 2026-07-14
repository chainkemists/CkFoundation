# PROGRESS — saveload-v3-parity

Living doc. The EXECUTOR updates this at every phase boundary and whenever reality diverges from a
phase doc. Planner: Fable session 2026-07-13 (design review + log triage). Branch:
`feature/save-load-improvements` (CkFoundation submodule; CkTests changes ride its current branch).

## Baseline (recorded Phase 0, 2026-07-14, against committed pre-edit binary)

- CkFoundation HEAD: `c3dfa6398b6f4730ef79b69f6a7e282a16b230bd` (branch feature/save-load-improvements; 82 ahead of origin/dev, clean tree)
- CkTests HEAD: `8ebc9c11d41850f70b94db326907c7f1b67cd9eb` (same branch, clean tree)
- Ck.Snapshot: **24 pass / 0 fail** (pattern `Ck.Snapshot`, 24 total, 5m5s, exit 0). Planner expected 27/2 — the "2 fail" are `Bb.Snapshot.*` (BB project, NOT matched by `Ck.Snapshot` and out of scope). Framework Snapshot core is cleanly green; no divergence.
- Ck Net: **90 pass / 0 fail** (pattern `Ck.Net`, 90 total, 13m38s, exit 0). See Deviations — the literal phase-doc pattern `Ck.*.Net` matched ZERO (toolbox uses substring tokens, not glob). `Ck.Net` over-matches ~11 non-Net CkTests AutoTests; harmless, delta measured against the identical pattern.
- Both baselines all-Success → no pre-existing reds to attribute a regression to.
- Orphan histogram from BB_QuickSave repro (Phase 0 [EDITOR-VERIFY], Adam): __PENDING__ (executor proceeds without it per PHASE_0 step 5)

## >>> STOPPED at end of Phase 5 (2026-07-14, per Adam) — resume at Phase 6 <<<

- Phases 0–5 COMPLETE + committed LOCALLY (unpushed) on `feature/save-load-improvements` in BOTH submodules.
- **Current gate state (the delta baseline for Phase 6): Ck.Snapshot 35/35, Ck.Net 90/90, build green.** Save format = **v4**.
- Trees clean in CkFoundation + CkTests. Nothing pushed (per package scope).
- Commit tips: CkFoundation `993f6323` (5B), CkTests `4d55ec34` (5B). Full per-phase hashes in the table below.
- Phase 6 (renames: registry/result-enum/hydration vocab + T_Policy Option-A deletion) and Phase 7 (VALIDATION) REMAIN. Phase 6 is mechanical/rebase-hostile — do it in one focused session; the T_Policy INERT surfaces were left in place by Phase 2 (comments only) precisely for Phase 6 to delete.
- Open Adam-decision / follow-up items are in "Follow-ups discovered" + [3.1-D1] below. The [EDITOR-VERIFY] items (BB_QuickSave orphan histogram; actor-branch transform restore; AS-defined dynamic fragments; several MP-reload harness-novelty gates) are listed per-phase and feed VALIDATION.md.

## Phase status

| Phase | Status | Commit(s) | Gate result vs baseline | Notes |
|---|---|---|---|---|
| 0 — orphan diagnostics | DONE | `8c7cd5c80` | build green; Snapshot 24/24 (delta-0), Net 90/90 (delta-0) on fresh binary | LoadReport `FCk_Snapshot_OrphanRecord[]` + `DoHydrate_Enqueue` per-orphan walk w/ 6 reason buckets; file-local `DoProvenance_ToString` (enum has no fmt formatter). Exit grep: exactly 1 `v3 load ORPHAN`. [EDITOR-VERIFY] BB_QuickSave histogram still Adam's (executor proceeded per PHASE_0 §5). |
| 1 — handler contract | DONE | `1dc8e444a` | build green; Snapshot 24/24, Net 90/90 (delta-0) | 26 files. FHandler +HydrationApply(required), Transport enum + HydrationApplyScope class DELETED, Get_SaveHandlerTypes=Produce&&HydrationApply sorted by path, registration ensure. SM Processor.h scope ref was COMMENT-only (no Fable needed). §1.5 ensure verified by inspection (inferred-not-run). FOLLOW-UP(Phase 2): 5 attribute *_Processor.h comments still say `TryHydrationApply`. |
| 2 — comments + doc | DONE | CkF `f214415ef` | build green; Snapshot 26/26 | Batch A. 24 comment fixes + CkSnapshot/Claude.md (recipe + §5 lazily-composed exception) + Source/CLAUDE.md. Claude.md force-added (gitignored *.md → now tracked). Net skipped for Batch A (justified: comment-only + unreplicated features; Phase 7 backstops). |
| 3.1 — EntityTag | DONE | CkF `7adf00b24`, CkTests `a28cb58` | Snapshot 26/26 (incl. EntityTag_MPReload two-cycle pin) | Fable fix applied: composite `FCk_Request_EntityTag_RestoreSet` (Fragment_Data struct + variant + DoApply_RestoreSet diff-at-drain + Utils Request_RestoreSet). HydrationApply enqueues 1 request, NO live read (merge bug gone) — verified. Doc §5 + anti-pattern#1 amend added. Test rebuilt w/ Construct-seeding probes (UCk_AutoTest_EntityTagSeed_EntityScript + 2 probe actors) → two-cycle count-exactness pin + resurrection + EnTT presence. save-only; payload `FCk_SaveData_EntityTags` (parallel FName/int32 arrays). DEVIATION [3.1-D1]: HydrationApply COMPOSES-BY-ADDING (no NotReady/Has<Current> gate) — EntityTag Current is lazily composed by Add + auto-removed at zero, NOT re-composed by Construct on load, so a NotReady gate would time out & drop the payload. Subagent argues safe (post-construction, unreplicated, HydrationApply is sole restore channel). Pending Fable review (batched w/ 3.2). Test `Ck.Snapshot.Parity.EntityTag_MPReload` (M2b single-world reload harness); +CkEntityTag to CkTests.Build.cs. Fidelity: FName tags only (not gameplay-tag view); cleared-to-empty-over-Construct-seed won't round-trip (inherent to no-empty-state model). |
| 3.2 — Timer | DONE | CkF `3ed679720`, CkTests `aec0e65` | Snapshot 26/26 (incl. Timer_MPReload) | save-only; payload `FCk_SaveData_Timer`{_Elapsed(FCk_Time), _CountDirection, _RunState=ECk_Timer_State{Paused,Running}}. Follows recipe (NotReady gated on FTag_Timer_NeedsSetup — VALID, Timer child re-composed via ConstructSpawned adoption). Jump is RELATIVE→delta-from-current. Done-timer NO re-fire (never Request_Complete; lands at goal via Jump→OnTimerJump/Update only, Paused). Test `Ck.Snapshot.Parity.Timer_MPReload` (2-client seamless harness, asserts server-side). Most-likely-wrong: timer-child rebuild identity mapping (test surfaces it if gapped). |
| 3.3 — EntityCollection | DONE | CkF `229d316b`, CkTests `378f59e` | Snapshot 29/29, Net 90/90 (EntityCollection.Net green) | idiom chosen: **implicit re-arm via Request_AddEntities** (→ MayRequireReplication). Owner-keyed, 3 NotReady gates before mutation, net Apply byte-identical, handle-remap verified (WalkHandles recurses array→struct→array→handle). ADD-only (collections compose empty in Construct + runtime-added members = correct real usage). LIMITATION (documented, for Adam): construct-seeded MEMBERS can't REPLACE-restore (no drain-time primitive; composite Request_RestoreSet like EntityTag would fix — out of scope, non-occurring case). Moved EntityCollections kDeferred→kCovered in coverage-ratchet meta-test. Test `Ck.Snapshot.Parity.EntityCollection_MPReload`. |
| 3.4 — SM overrides | DONE | CkF `6d4345c5`, CkTests `648e015` | Snapshot 29/29, Net 90/90 (StateMachine.Net green) | `FCk_Sm_SavedStateOverride`{_OverrideStateClass, _CachedStatesToOverride} (matched ACTUAL FEntry) on BOTH RepData structs; wire builder never fills it (confirmed). ORDERING PROOF (not a Blocker): override Request_AddOverrideState enqueued in HydrationApply BEFORE the resume record exists → ahead of resume's restore Request_Transition in the SM FIFO → drains before Get_ResolvedStateClass (DoEnterState) consults it. Deviation (correct): re-add folded INSIDE Sm_StashHydrationResume AFTER its Has<Current> NotReady guard (single-shot, no retry-stacking). Test `Ck.Snapshot.SmStateOverride_Reload` (single-world OpenLevel; AS override-state classes). [EDITOR-VERIFY] harness novelty: single-world OpenLevel SM respawn unproven — gate confirms; fallback = seamless SM parity harness. |
| 3.5 — Refill | DONE | CkF `5cd1d3d9`, CkTests `6e5d527` | Snapshot 29/29 (AttributeRefill_MPReload) | PROCEEDED (not deferred). Refill = own LABELED ConstructSpawned child (adoption confirmed CkEntityLifetime_Utils.cpp:466-474). Captures run-state (Running/Paused) only — fill-rate already persists via value handler. New shared `CkAttribute_RefillPersistence.h` + Float/Integer registrars (save-only). Unreplicated run-state → no re-arm. Test `Ck.Snapshot.Parity.AttributeRefill_MPReload` (dedicated probe, zero blast radius). Wire structs untouched. |
| 4 — G1 Transform | DONE | CkF `0be6ef9e`, CkTests `7ac25f0` | Snapshot 30/30 (Transform_MPReload), Net 90/90 (delta-0) | **format version = 4** (bump 3→4; pre-bump v3 saves rejected loudly at Request_Load — was already loud, no hardening needed). _SavedWorldTransform column + DoApply_SavedTransforms at DoHydrate top (before payload loop; Phase 0 orphan walk intact, verified :898 vs :989). Atomic world-space Request_SetTransform (pure-ECS) / SetActorTransform TeleportPhysics (actor) / bridged-skip. Actor-branch = [EDITOR-VERIFY] (headless map lacks placed content; bridged-actor restore separately covered). No net path. |
| 5A — G17 provenance | DONE | CkF `aea622d4`, CkTests `d282763` | Snapshot 35/35, Net 90/90 (delta-0) | NO Blocker — construction is SYNCHRONOUS (Construct→DoConstruct inline; proven same synchronicity as EntityScript stamp). Bracket = ON_SCOPE_EXIT around the ConstructionInfos loop (EntityReplicationDriver_Utils.cpp:213-229). FTag_DefinitionBuild_InProgress + extended stamp condition. Census shift: NONE (fixtures stamp manually or use non-stackable items; no orphan risk). Test `Ck.Snapshot.Parity.InventoryStackCount_MPReload` (count 7 vs default 1). Caveat: TAG_TRANSIENT is a doc-only alias post-Model-A-purge; non-persistence rests on the scope guard (holds). Limitation: only synchronously-composed children stamped (async deferral misses — same as EntityScript stamp; Stackable is synchronous). |
| 5B — G2 dynamic | DONE | CkF `993f6323`, CkTests `4d55ec34` | Snapshot 35/35, Net 90/90 (delta-0) | **5B.1 = RAW variant** (both ends use FObjectAndNameAsStringProxyArchive → nested FInstancedStruct stringifies + gets 5B.0 remap free; no blob). 5B.0 WalkHandles: appended FInstancedStruct case ahead of handle-struct test (existing branches untouched, InRehashAfterKeyVisit threaded). Handler wrapper `FCk_SaveData_DynamicFragments` in CkDynamic_Fragment.cpp (Produce=Get_AllFragments; HydrationApply=AddOrGet_Fragment_TypeUnsafe+OnRepNotify+re-arm once+drift-skip-Warning; no net Apply; fallback byte-identical). Tests: HandleWalk.{InstancedStructNested,TombstoneUntouched} + DynamicFragment.{HandleRemapRoundTrip,HydrateDriftSkip}. [EDITOR-VERIFY] AS-defined dynamic types. |
| 6A — vocab rename | ABSORBED by saveload-v3-ergonomics Phase 2 (2026-07-14) | | | executed there — do NOT run here |
| 6B — T_Policy delete | ABSORBED by saveload-v3-ergonomics Phase 2 (2026-07-14) | | | executed there — do NOT run here; site count found: ____ (in ergonomics PROGRESS) |
| 7 — VALIDATION | NOT STARTED | | | |

## Blockers (STOP-and-record; do not improvise past these)

_(none yet)_

Format per entry: date / phase+step / what was expected / what was observed (verbatim error or
grep output) / what you did NOT do / question for the planner or Adam.

## Deviations from plan (executed differently than written, with reason)

- **2026-07-14 / Phase 0 baseline / Net pattern.** PHASE_0's literal command `--test-pattern "Ck.*.Net"`
  matched ZERO tests ("No tests matched — nothing to run", exit 1). Cause: the toolbox `--test-pattern`
  splits on `.` and requires each token to be a case-insensitive SUBSTRING of the test path — there is
  no glob, so the literal `*` token matches nothing (VERIFIED from `UnrealToolbox --help`). Used
  `--test-pattern "Ck.Net"` (tokens `Ck`+`Net`) instead, which matched 90 tests (all `Ck.*.Net.*` plus
  ~11 non-Net CkTests AutoTests the substring `Net`/`Ck` incidentally catches). Same pattern will be used
  for the post-build delta, so delta-zero remains valid. Intent (measure the Net baseline) preserved; no
  redesign. The prior campaign's "103" is a different count basis (likely client+server world pairs).
- **2026-07-14 / directive change.** Adam instructed (mid-Phase-0) to run unattended through ALL phases
  rather than one-per-session, invoking a Fable-model agent to design solutions when a real issue
  surfaces, and compacting between phases. Divergences are still recorded here, but the run does NOT stop
  at them. Push remains out of scope (commits local).

## [3.1-D1] Fable verdict (2026-07-14) — EntityTag HydrationApply MERGE bug + sanctioned pattern

Fable review (model=fable) of the EntityTag compose-in-HydrationApply deviation: **CHANGE** (principle sound, impl buggy).
- BUG: shipped `HydrationApply` clears-live-then-re-adds via deferred `Request_TryRemove`/`Add`. But those drain in `FProcessor_EntityTag_HandleRequests` which is `GatedDuringLoad` (no LoadPolicy), while Construct/BeginPlay (`RunsDuringLoad`) may seed the same tags via the same deferred requests. At HydrationApply time the seeds are ENQUEUED-but-invisible to the live "clear" read → FIFO drains `[seeds…, saved adds…]` → MERGE not REPLACE → **monotonic count inflation per save/load cycle**, resting on an unpinned pump tie-break. The M2b test can't catch it (probe seeds no tags).
- FIX (implementing): new composite request `FCk_Request_EntityTag_RestoreSet{TagNames,Counts}` handled by `FProcessor_EntityTag_HandleRequests`; drain-time `DoApply` diffs live→saved set, fixes `Set_StoragePresence` per delta, fires only net Added/Removed. FIFO guarantees it lands AFTER construct seeds → order-independent + idempotent.
- CODIFY: `CkSnapshot/Claude.md` gets §5 "Lazily-composed, data-defined features (reconstitute-by-request)" + anti-pattern #1 amendment (exception). 5 preconditions: data-IS-existence; HydrationApply-only (never net Apply); REPLACE rides the request FIFO (never read-live-then-clear — post-construction ≠ post-construction-effects under the gated load kernel); idempotent; absence-ambiguous (unset can't tell never-had from all-removed → construct-seeds resurrect).
- TEST must pin: count-exactness across TWO save/load cycles + a Construct-seeded-tag scenario; counted multi-add; per-tag EnTT `ForEach_Entity` presence; readable at OnLoadComplete; resurrection semantics; gameplay-tag degradation (`Request_TryRemove_UsingGameplayTag` silently no-ops post-load).
- Also: fix inaccurate registrar comment ("Add composes Current, fires signals" — Add only enqueues). Adjacent smell (flagged, separate): `FProcessor_EntityTag_HandleRequests` is ungrouped — worth `using Group = FGroup_Gameplay;` independently.

## Follow-ups discovered (do NOT act on these in-package)

- [3.4] SM non-authority hydration ensure-edge: `Sm_ReinstallSavedOverrides` uses public deferred `Request_AddOverrideState`, which `FProcessor_Sm_HandleRequests` ensure-drops on a non-authority machine. A `Replicates`+`OwningClientAuthoritative` SM hydrated on a machine that isn't its owning client (dedi server / listen host viewing another player) would fire an ensure where the base resume merely no-ops. Confined to that already-unsupported config; not exercised by tests (authority-only) or the Net gate (HydrationApply runs only on the load path). Clean fix if it ever matters: carry saved overrides in FFragment_Sm_HydrationResume + re-add from the resume processor's authority-gated Start phase.
- [5B] Dynamic-fragment payload doesn't carry the per-type replication flag → a dynamic fragment added with `Replicates` before save restores LOCAL-ONLY unless construct re-registers it replicated (the re-arm is gated on that). Prescribed shape (Model-A also captured value-only). Fidelity gap for Adam if per-type replicated dynamic fragments must survive as replicated.
- [3.3] EntityCollection construct-seeded MEMBERS can't REPLACE-restore (ADD-only; no drain-time restore primitive). Non-occurring case (collections compose empty + runtime-added members). Composite Request_RestoreSet (EntityTag pattern) would fix if ever needed.
- BB-side: debugger tab + CombatReceiver cache handles across load → ensure storm (pre-existing,
  out of scope; needs a BB session).
- BB-side: 3 capture-audit unlabeled-child-with-payload warnings (incl. Bb_AchievementDriver child
  under owner 31) → label or accept as transient.
- **[found 2026-07-13, planner session] The ENTIRE BB restore-rebind fleet is dead code under v3.**
  28 AS processors (`rg -n "Get_WasJustRestored" Script/` in BB: CombatReceiver, StoreDriver,
  DayCycle, Economy, Vendor, Door, Trashcan, BoomBox, ChangeablePoster, CosmeticOwnership,
  Collectibles, EmployeeManager, FlyerRecipient/Stand, LootableInventory, MovieMixing, OpenSign,
  Presentation, QuickUseSelector, RentalManager, RetailGondola, StoreCustomization/Expansion/
  Inventory/Ledger/Naming, + CombatReceiver Setup's refill-seed gate) all gate on
  `ck::FTag_Snapshot_JustRestored`, which `CkSnapshot_RestoreMarker.h` itself documents as legacy —
  **never stamped by the v3 load path**. Every gate is permanently false: no post-load signal
  rebinds, no cached-handle refreshes (e.g. `FBb_Fragment_CombatReceiver_State`'s cached attribute
  handles → the debugger tombstone storm; the debugger itself was hardened 2026-07-13, BB
  Script/Debugger). Deciding the v3 replacement (stamp a real per-entity restored/adopted marker
  from the load path vs. per-feature validity-triggered self-heal vs. "rebuilt entities re-bind via
  Construct so most of the fleet should be DELETED") intersects Phase 0's orphan histogram — the
  planner must rule on it before or during Phase 3. Framework side: `CkSnapshot_RestoreMarker.h`
  deletion is already flagged in that header as a joint BB+CkF task.

## The one claim most likely to be wrong (update as phases land)

Planner's opener: that the 64 orphans decompose into known buckets (cascade / label-miss / known
gaps). If Phase 0's histogram shows a dominant `savekey-miss`/`player-miss` or `unresolved-other`
bucket, the parity phases are aimed at the wrong target — STOP and re-plan before Phase 3.
