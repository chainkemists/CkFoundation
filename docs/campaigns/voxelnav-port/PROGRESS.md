# PROGRESS — voxelnav-port

> **The living tracker. Trust this file over any session memory or summary.**
> Update at every ruled decision, accepted unit, and session close — never batched.

## Status board

| Phase | State | Evidence |
|---|---|---|
| 0 — Scaffold | **CLOSED 2026-08-04** — all exit criteria met | Exit evidence block below; commits d401a54aa/13e30ab1f/49da3aeb5 (CkFoundation), CkTests + superproject commits per session log |
| 1 — Octree + voxelize | **CLOSED 2026-08-04** — gate: full suite delta-zero (981/975/6) + Ck.VoxelNav 20/20 + Ck.Jolt 48/48, zero contaminated, hygiene clean | commits 2c8aec16c/05e146a4a (CkFoundation), deea9fb1/5a6c65c9 (CkTests) |
| 2 — Pathfinding | **CLOSED 2026-08-04** — gate: full suite delta-zero (981/975/6) + Ck.VoxelNav 38/38 + Ck.Jolt.Query 4/4; CkAStar untouched | commits 489eb4bc3/69d689466 (CkFoundation), 3da53af6/12c32a84 (CkTests) |
| 3 — Chunks & dynamics | **OPEN** — wave 1 (3B dynamic occluders, audit-first) dispatched | PHASE_3.md |
| 3 — Chunks & dynamics | not started | — |
| 4 — Consumers | not started | — |
| 5 — Perf (merging) | not started | — |
| 6 — Deferred pool | closed by default; items open by decision entry only | — |

## Baseline (campaign open)

- Branch: `feature/ckvoxelnav-port` @ `c11766760` (== origin/dev tip 2026-08-03), UNMODIFIED
  Source/. Invocation: `--build --test --parallel 1 --no-nullrhi` (single-shot, no --config).
- **Total: 981 | Passed: 974 | Failed: 7 | Skipped: 0 | Contaminated: 0 | Duration: 15m24s (tests)**
- Pre-existing failing names (delta-zero means: exactly these 7, no more, no fewer):
  1. `Ck_AutoTest_PathNetworkFollower_RoutePrefersNetwork` (goal-exactness assert)
  2. `Ck_AutoTest_PathNetworkFollower_ComponentTransferUsesDisconnectedIslands`
  3. `Ck_AutoTest_PathNetworkFollower_DesiredNavmeshClearanceMovesInward` (fixture: no north navmesh boundary)
  4. `Ck_AutoTest_PathNetworkFollower_LocalShortcutUsesSameComponentGap`
  5. `Ck_AutoTest_PathNetworkFollower_ProjectsRibbonWaypointWithinNavQueryExtent` (same fixture issue)
  6. `Ck_AutoTest_PathNetworkFollower_TuningReplansSameGoal`
  7. `Ck_AutoTest_CkJolt_ChaosParity_KinematicPlatformCarry` — editor DIED mid-test (exit 0x3),
     toolbox-marked Failed, run resumed in a fresh lane. Instability, not an assertion red; watch
     whether it recurs at phase gates.
  Also noted: the resumed lane's editor exited 0xFF AFTER completing all tests (results kept).
- All 7 are foreign workstreams (PathNetwork, Jolt parity) — none touched by this campaign.

## Done

- 2026-08-03: Campaign opened. Branch created; sibling detached work preserved
  (`backup/detached-asset-exporter-20260803` = `fcf3da7bd`, unpushed local merge, not ours).
  Doc set authored (PROMPT, PHASE_0, VALIDATION, this file). Research record: 7 dossiers under
  `research/` (5 from the port sweep, 2 from the spatial-structure follow-up).
- 2026-08-03: `research/phase1-port-map.md` (1,381 lines, Opus, orchestrator-reviewed) — full type
  translation, function-level port map (PORT/PORT+FIX/SEAM/DROP per function), 14-stage resumable
  voxelizer design, AStar adapter + volume entity designs. Found SIX verified upstream Nav3D bugs
  (free-space Morton stuffed into 6-bit SubNodeIndex; backwards NavNodeRef unpack ctor; cube-vs-edge
  bounds check in FindNeighbourInDirection; GetRandomPoint one-past-last layer index; 8x-duplicated
  blocked-parent propagation; full-cube RasterizeLayer scan) — all fixed in the map. Behavior change
  to expect at Phase 1 exit: occupancy decided by collision shapes (Jolt), not LOD0 render tris —
  baked results differ from Nav3D by design.

## PHASE 0 EXIT EVIDENCE (closed 2026-08-04)

1. Build: green (`Result: Succeeded`, integration build + relink build).
2. Boundary full suite (post-[C-D16], final artifact): **981 / 974 / 7 / 0 contaminated, 12m06s** —
   six known PathNetworkFollower reds + `Ck_AutoTest_PathNetworkFollower_FallsBackToNavigation`
   (arrival-distance 180cm), which passed 1/1 SOLO immediately after → flake-observed in a foreign
   known-fragile family, non-blocking per [C-D18(b)]. (A second suite sample was started and then
   deliberately stopped under [C-D18] — evidence already sufficient.) Pre-fix suite samples: 2×
   delta-zero (975/6 and 974/7-with-crash-flake, baseline-identical).
3. Targeted C++ patterns ([C-D17]): `Ck.VoxelNav.Volume.Scaffold.*` 2/2 green;
   `Ck.Jolt.Query.BoxOccupancy` 1/1 green post-[C-D16], zero net-driver noise.
4. Hygiene: `rg -l "Jolt/" Source/CkVoxelNav` = 0; libmorton allowlist + Used-by landed; CkJolt
   Claude.md updated; uplugin + Source/CLAUDE.md rows landed; MIT attribution + LICENSE.Nav3D.txt.
5. Bonus root-cause fix [C-D16] verified healed: pre-existing `Ck.Jolt.Body.Lifecycle` 0/3 → 3/3.

## In-flight

- Phase 3 wave 2 (3A chunked volumes + cross-chunk pathfinding) — opus, dispatched.

- **3B ACCEPTED 2026-08-04** (opus, self-gated 46/46, zero ensures): AUDIT verdict — upstream #39
  is NINE defects (D1 fatal: layer-0 loses Morton sort AND index identity on RemoveAtSwap/Add with
  no re-link — first movement corrupts every lookup; D2 fatal: contiguous-8-children invariant
  broken by one-at-a-time adds; D3: occupancy OR-accumulated, never cleared — permanent occupied
  trails; D4-D9: dead scans, no lattice clamp, volume fan-out, unbudgeted tick, inert occluder
  registry, leaked leaves). Ported design makes D1/D2/D3/D9 UNREPRESENTABLE: occupancy re-derived
  (never accumulated), structure DERIVED from the blocked-cell set by re-running the bake's own
  structural stages, repair assembles a NEW octree and swaps (C-D7 preserved) — ratified. Oracle
  test: repaired octree field-for-field equals a full rebake; locality pinned (67 probes vs 4360,
  untouched leaves bit-identical). Deviations 2-8 accepted (incl. the self-review catch:
  Request_Build abandons an in-flight repair — publish-slot race closed).
- **[C-D22]** (3B deviation 1 — capability finding): v1 voxelization sees STATIC-domain bodies
  only ([C-D14] filter + CkJolt's motion-type→domain mapping). KINEMATIC bodies are invisible to
  bake AND repair — the occluder feature today covers static-body teleports and appearing/vanishing
  geometry, NOT moving kinematic platforms. Fix = the deferred composed Domain filter in CkJolt;
  assigned to the deferred pool, trigger: first game needing moving platforms to block flying nav.

- **Wave 2 (2D+2E) ACCEPTED 2026-08-04** (opus, self-gated 38/38, zero ensures): refinement =
  greedy visibility pruning (octree raycast, endpoints kept, unprovable spans kept — always
  traversable, never longer) + CatmullRom smoothing with BOTH segment and point re-validation
  (a curve bulging outside the baked volume passes a segment-only check — the point test is the
  soundness fix). Zigzag pinned: raw 22 wp / 967.32uu → pruned 2 wp / 571.66uu (fixture designed
  tie-break-invariant). TWO MORE upstream bugs fixed (tally 11): smoothing's virtual endpoints
  used `2*(P0−P1)` (a direction as a control point — drags spans toward world origin; correct
  `2*P0−P1`) and upstream's smoothed path silently DROPPED its final waypoint (`Index < Num-2`
  emission). PIE end-to-end test plans a collision-free route across the baked scene. All 9
  deviations accepted. CkAStar verified untouched (empty diff). Commits: CkFoundation `69d689466`,
  CkTests `12c32a84`.

- **2C ACCEPTED 2026-08-04** (opus, self-gated): `Ck.VoxelNav.Path` 8/8; combined `Ck.VoxelNav`
  34/34 on the final artifact (orchestrator re-verified the log). FPathGraph satisfies
  `astar::AStarGraph` (static_assert), synchronous capped search per [C-D21], iteration knob
  landed. Deviations 1-6, 8 accepted — notably: `Stale` DERIVED at the read boundary from epoch
  drift (never stored — rebuilds don't walk paths); node-size compensation folded into Cost (only
  CkAStar-compatible seam; documented inadmissible; default off); unified
  `ECk_VoxelNav_PathSearchOutcome`. Deviations 7 (PIE success-path test gap) + 9 (module doc Path
  section) assigned to wave 2. Recorded gotcha (m): hermetic single-processor scheduler tests must
  add the processor's Group + FGroup_DestructionPipeline descriptors to the same list or graph
  building fails. Commits: CkFoundation `489eb4bc3`, CkTests `3da53af6`.

- **2A+2B ACCEPTED 2026-08-04** (opus, self-gated green): CkJolt session grew
  `Get_IsSegmentBlocked` (any-hit CastRay, static filters, [C-D14]-clean) — `Ck.Jolt.Query` 4/4
  incl. 3 new segment assertions; octree ray-marcher ported to `Octree/CkVoxelNav_Octree_Raycast.*`
  (538 lines) with `Ck.VoxelNav.Raycast` 6/6. TWO MORE upstream bugs found+fixed (campaign total
  NINE): (8) `GetFirstNodeIndex` compared FAR t-values where Revelles requires MIDpoints —
  over-visits children, can report a farther hit as closest; (9) impact point un-mirrored twice
  (local origin mirrored at :165, world result re-mirrored at :223-238) — pinned by
  reverse-direction assertions. Also: leaf scan now keeps the CLOSEST of all 64 sub-node hits
  (upstream returned first-in-Morton-order = arbitrary distance); double-precision parametric math;
  the octree-vs-physics segment-test doctrine documented in three places. Deviations 3-11 accepted.
  Fixture note for later authors: `TryGet_NodeAddressFromPosition` falls through to
  nearest-free on fully-occluded leaves — never returns the occluded leaf's own address.

- **Wave 2 ACCEPTED + GATE GREEN 2026-08-04: 20/20 (35s), zero ensures.** Path to green: 3
  orchestrator inline compile fixes (stub CK_PROPERTY_GET→hand getter — the 1A-documented trap;
  stub CKVOXELNAV_API removed — header-only, never exported; Build.cs +DeveloperSettings), then an
  opus debug agent root-caused the 4 functional reds: (1) the anti-hang guard compared STAGE
  identity instead of a (stage+cursors) footprint, so the first zero-probe layer-walking step after
  RasterizeSubNodes read as a stall and failed every real bake (empty-volume pass was the tell);
  fixed with FStepFootprint comparison, guard intent preserved. (2) PIE test setup created its
  owner via bare Request_CreateEntity(registry) → no LifetimeOwner → no world → StartBuild failed
  before any state existed; fixed via Request_CreateEntity_TransientOwner (proof: framework ensure
  text + Get_WorldForEntity impl). Plus Remove→Try_Remove on may-be-absent tags (first-build
  ensures). NO assertions weakened — hand-computed probe cascade reproduces exactly (hermetic 5328
  = 64+144+80×64; PIE 280 = 64+24+3×64, 18 slices). Commits: CkFoundation `05e146a4a`, CkTests
  `5a6c65c9`.
- Adjacent finding (l): the PIE bake test's three box entities are still bare-registry-created —
  harmless for CkJolt today; will bite any future world-resolving feature composed onto them.

- **Wave 2 (1C+1D+1E) implemented, spot-check + gate pending** (opus): Build.h/.cpp state machine,
  backend-driven stages, Volume/ rewrite (BuiltOctree fragment + epoch + TSharedPtr<const FOctree>,
  Request_Build/CancelBuild with full completion contract, 5 processors with [C-D11] placement
  verbatim, EndPlay cancellation), settings knobs, 5 new tests + scaffold update. All 11 deviations
  accepted. Module CLAUDE.md gained a "parts that look wrong" section — good doc practice.
- **[C-D20]** (wave-2 STOP item) `Stage_RasterizeLayer`/`Stage_BuildNeighbourLinks` budget at
  one-LAYER-per-slice (wave-1 signatures kept; 13 green tests pinned them). Known limitation: a
  large octree's neighbor-link layer is an unbounded single-tick spike during BAKE. Accepted for
  Phase 1 (load-time bake, small scenes); threading cursor+budget through both stages is PHASE 5
  scope (the landed shapes already take FRasterizeScratch&/return FRasterizeStageResult, so the
  change is additive).
- **Repo trap discovered (wave 2)**: CkFoundation `.gitignore:49` is `*.md` — every NEW module doc
  is silently ignored (existing module docs predate the rule). `Source/CkVoxelNav/CLAUDE.md`
  force-added by orchestrator. Follow-up (k): the .gitignore rule deserves maintainer review —
  it will eat the next module's doc too.

- **WAVE 1 GATE GREEN 2026-08-04: 15/15 (13 Octree + 2 Scaffold), 34s.** One compile fix applied
  inline by orchestrator: MSVC C4273 — friend redeclarations of the three `Request_*Octree`
  functions needed `CKVOXELNAV_API` to match the decorated declarations. The libmorton
  unity-blob risk did NOT materialize. Committed: CkFoundation `2c8aec16c` (Octree+Backend),
  CkTests `deea9fb1` (octree tests). 1A + 1B acceptance FINAL.

- **1A octree core — ACCEPTED 2026-08-04** (opus, compile pending wave-1 gate): 8 files / 3,008
  lines under Octree/ + 669-line Test_VoxelNav_Octree.cpp (13 hermetic tests). All six mandated
  upstream fixes implemented (evidence table in the unit report) PLUS a SEVENTH found+fixed:
  `GetFreeNodesFromNodeAddress` recursed with a Morton CODE where a node INDEX is required
  (`VND:2217-2224`, hidden behind bug #4 since GetRandomPoint is the only caller). Ten deviations
  accepted — notably CK_PROPERTY_GET→hand getters (macro requires CK_GENERATED_BODY/ThisType),
  friends→named mutators with `TSharedPtr<const FOctree>` carrying C-D7 immutability,
  `Request_MarkOctreeBuilt` publish switch, exponent>10 overflow guard (upstream latent bug).
  O2 ruling: `Get_ParentMortonAtLayer` implemented with the octree's own coarser=higher-index
  convention (upstream's version has zero callers and inverted direction) — contract block + test
  pin it. O3: upstream's multi-level neighbor-fallback bug ported FAITHFULLY behind a CK_ENSURE
  so it surfaces rather than being silently patched.

- **1B geometry backend — ACCEPTED 2026-08-04** (opus, compile pending wave-1 build): 4 files /
  275 lines under Backend/. Interface = exactly the two calls §3's stages consume
  (Get_IsBoxOccupied, Get_BodiesInBox); Jolt impl over the Phase 0 session (lazy exact-extent
  probe memoization — port map's ctor-time wording was self-inconsistent, deviation adopted);
  header-only stub double for 1C. All 5 narrowing deviations accepted (no SegmentBlocked — no
  Phase 1 consumer AND unimplementable over the current CkJolt surface; no BodyBounds — callers
  deleted; no empty BackendParams struct — clearance arrives pre-inflated per contract; lazy
  probes; _Stub suffix). Hygiene-gate regex corrected in PHASE_1.md per its flag (`<Jolt/` form).
  1D note: `Get_IsValid()` lives on the CONCRETE Jolt backend; the build processor must gate on it.
- **[C-D19]** Phase 2 prerequisite (from 1B's analysis): CkJolt's session gains
  `Get_IsSegmentBlocked(From, To) -> bool` (any-hit CastRay, static-domain filter, no channel, no
  attribution — [C-D14]-consistent) BEFORE the refinement pass unit; the backend interface then
  grows the matching pure virtual (6-line change). Phase 2's opening unit owns this.

- **[C-D16] VERIFIED HEALED 2026-08-04**: post-fix solo runs — `Ck.Jolt.Query.BoxOccupancy` **1/1
  green, zero EnableListenServer/NetDriverCreateFailure hits in the log** (was 0/1 twice);
  `Ck.Jolt.Body.Lifecycle` **3/3 green** (was 0/3). The occupancy stack (session resolution,
  narrowphase in/out, broadphase AABox + body-id attribution) is proven end-to-end in PIE. The
  pre-fix full suite also reconfirmed delta-zero (981/975/6, the six known PathNetworkFollower
  reds, 0 contaminated) — so the phase's plugin-code is regression-free independent of the
  uproject repair.

- Relink chronology: touch + `--build --test-pattern VoxelNav --discover-fresh` → **2/2 VoxelNav
  scaffold tests discovered and GREEN** (36s). Then `Ck.Jolt.Query.BoxOccupancy` solo → FAILED 2/2
  runs, NOT on its own assertions (latent chain completed; body-added wait met) but on an attributed
  engine ensure during PIE start: `EnableListenServer` → `NetDriverCreateFailure`.
  **Discriminator (decisive): the pre-existing exemplar `Ck.Jolt.Body.Lifecycle.*` — green in the
  full suite — fails 0/3 SOLO with the identical ensure.** Mechanism = PIE-multi-client fixture
  fails its listen-server bind in a fresh solo editor session on this config; passes inside the
  full-suite session. Our test inherits a pre-existing solo-run limitation; it is not the defect.
  Follow-up (h): the solo-run listen-server failure of the PIE fixture deserves a root-cause pass
  by the CkTests owner (diagnosis agent's mechanism report to be attached).

- **PHASE 0 FULL-SUITE GATE: DELTA-ZERO ACHIEVED 2026-08-04** (orchestrator-run, single-shot
  `--build --generate --test --parallel 1 --no-nullrhi`): **Total: 981 | Passed: 975 | Failed: 6 |
  Contaminated: 0 | 12m42s.** The 6 reds are exactly baseline reds 1–6 (PathNetworkFollower);
  baseline red 7 (`Ck_AutoTest_CkJolt_ChaosParity_KinematicPlatformCarry`, the mid-test editor
  crash) PASSED this run — instability confirmed, not a deterministic red. No new failures from
  Phase 0 code. Editor's end-of-run 0xFF exit recurred (pre-existing, results kept, same as
  baseline). Remaining for phase close: the 3 new tests discovered + green.

- **0D tests — implemented, spot-check + gate pending** (opus): CkTests branch
  `feature/ckvoxelnav-port` created at `e5bb948b` (verified no ref move, clean tree + 3 paths).
  187-line PIE session-layer occupancy test (exemplar Test_JoltBody_Lifecycle.spec.cpp; static
  JoltBody at (0,0,20000), named wait conditions, 3 assertion stages) + 90-line hermetic FEcsWorld
  scaffold test (exemplar Test_JoltBody_OwnershipExclusivity) + CkTests.Build.cs +CkVoxelNav dep.
  Deviations 1-5 all accepted (session-layer choice was spec-sanctioned; invalid-owner rejection
  test is non-negotiable #3 compliance; .cpp suffix matches JoltBake siblings).

- **Integration build GREEN 2026-08-03** (`Result: Succeeded`, 733s, orchestrator-run): 0A+0B+0C
  compile clean first try. (First attempt failed on vendored Jolt `PhysicsUpdateContext.cpp` at 0s
  with no diagnostics — XGE agent flake, confirmed by clean retry of the identical invocation.)
  0B and 0C acceptance FINAL. Committed: `d401a54aa` (libmorton) → `13e30ab1f` (CkJolt occupancy)
  → `49da3aeb5` (CkVoxelNav skeleton) → `1726f6f0d` (docs).
- **[C-D15]** CkTests campaign branch bases on its CURRENT detached HEAD `e5bb948b` (clean tree),
  NOT origin/dev — the 981-test baseline was captured against e5bb948b; moving CkTests would change
  the suite population and void delta-zero. (CkTests local dev is 83 behind origin — same sibling
  drift pattern as CkFoundation; pairing against origin/dev is a ship-time question, not Phase 0's.)

- **0B implemented, spot-checked, compile pending** (opus): 4 new files (Occupancy_Utils JPH layer
  479 lines total, Occupancy_Session JPH-free pimpl layer), +32 additive lines in
  CkJoltCollisionLayerTable.h (StaticOccupancyFilter + StaticBroadPhaseQueryFilter), CkJolt
  Claude.md section. Session header verified JPH-free by orchestrator grep. Judgment calls 1-4
  accepted. Acceptance finalizes when the integration build is green.
- **[C-D14]** (0B's OQ-3 flag) v1 occupancy filtering is STATIC-DOMAIN ONLY. `_QueryChannel` is
  removed from the Phase 1 backend params (amends the port map's §6/OQ-3 adoption under [C-D12]);
  a composed Domain+Channel filter waits for a named consumer needing per-channel nav-blocking.
  Noted, accepted: body-id 0 is a theoretical validity-sentinel hole (uint8 sequence wrap after
  255 reuses of body index 0) — documented, not guarded.

## Accepted units

- **0A libmorton vendor — ACCEPTED 2026-08-03** (opus). 10 files byte-identical to
  F:\Nav3D-2.0 copy (cmp+sha256 by executor; count/entry/rule re-verified by orchestrator).
  build.cs include path line 20; Claude.md table row + rule 6 (allowlist: CkVoxelNav sole direct
  consumer). Small gap assigned to integration pass: CkThirdParty Claude.md "Used by:" header line
  should gain `CkVoxelNav (libmorton)` once the module exists.
- **0C CkVoxelNav skeleton — ACCEPTED 2026-08-03** (opus). 13 files / 373 lines, exemplar-mapped
  per file (CkTimer quartet + CkPathNetwork Add ritual); uplugin parses (118 modules, CkVoxelNav
  present); Source/CLAUDE.md rows landed; zero Jolt includes (orchestrator re-grepped). Reported
  deviations 1,3,4,5,6,7 accepted as-is (no _Fragment_Data.cpp — nothing to hold; NeedsBuild-consuming
  no-op Setup; root-doctrine validation form; corrected CkHandle_TypeSafe.h casing; no cast-conv BP
  nodes yet; FGroup_Gameplay_TimeDelta on the placeholder Setup). Compile deferred to the phase gate.
- **Orchestrator integration pass 2026-08-03:** copied Nav3D LICENSE →
  `docs/campaigns/voxelnav-port/LICENSE.Nav3D.txt` (0C's attribution was dangling); added
  `CkVoxelNav (libmorton)` to CkThirdParty Claude.md Used-by line (0A's gap); applied [C-D13]
  category rename in CkVoxelNavVolume_Utils.h.

## Follow-ups discovered (foreign code — NOT this campaign's scope)

- (f) `UCk_Utils_PathNetwork_UE::Add` uses the inline `CK_ENSURE_IF_NOT(...) { return {}; }` form —
  its early-out compiles away under `CK_DISABLE_ENSURE_CHECKS`, letting an invalid owner reach
  `Request_CreateEntity` (violates non-negotiable #3's separate-branch rule). One-line fix for the
  PathNetwork owner.
- (g) `CkTimer_Fragment_Data.h` includes `CkHandle_Typesafe.h` (wrong casing; on-disk file is
  `CkHandle_TypeSafe.h`) — resolves only on case-insensitive filesystems while CkTimer is
  Mac/Linux-whitelisted.
- (h) RESOLVED by [C-D16]'s diagnosis — the "solo-run PIE fixture limitation" was actually the
  project-wide missing-online-plugins misconfiguration; full mechanism recorded there.
- (i) `bSuppressLogWarnings`/`bSuppressLogErrors` are STATIC members of `FAutomationTestBase`;
  both Jolt test files (and now BoxOccupancy, by mimicry) set `bSuppressLogWarnings = true` inside
  RunTest, leaking process-wide to every later test in the run. CkTests owner should scope it.
- (j) `ck_net_automation_common::Override_PlaySettings` forces `PIE_ListenServer` even for
  NumClients=1 (`CkNetAutomation_Common.cpp:134`); harness "works" pre-fix only because
  `GetNetMode()` reports NM_ListenServer off the `?Listen` URL despite the absent driver. Worth a
  deliberate pass by the CkTests owner post-[C-D16].

## Known foreign dirt (never stage, never revert without a decision)

- 76 × `Content/CkUsf/GeneratedLooks/M_CkUsf_Look_*.uasset` modified — known CkUsf test-lane
  on-disk churn (matches standing memory; almost certainly our own baseline run's side effect).
  Phase 0's "working tree clean" entry criterion is interpreted as: clean apart from campaign work
  and this enumerated churn.
- Historical note (resolved): first baseline launch failed — this worktree's CkAuto toolbox
  binaries were unsmudged LFS pointers; healed via `git -C CkAuto lfs pull`.

## Decisions

- **[C-D1]** Module name `CkVoxelNav` (ns `ck::voxelnav`). Zero token collision in the 122-dir
  census; median length; names the representation (CkAStar/CkGrid precedent). Runner-up CkVolumeNav.
- **[C-D2]** Campaign branch bases on origin/dev tip `c11766760`, not the superproject's recorded
  gitlink `7f4cc524d` (an ancestor of it) and not the detached `fcf3da7bd` (unpushed sibling merge,
  preserved as backup branch). Superproject pointer bump happens at ship time.
- **[C-D3]** Search runs on CkAStar via an `astar::AStarGraph` SVO adapter + post-search refinement
  (visibility pruning + 3D funnel). Nav3D's Theta*/LazyTheta* are NOT ported: they don't fit
  CkAStar's closed relaxation loop, and McGill-2024 evidence shows refinement recovers the quality
  while CkAStar's budgeted/warm-start machinery is what horde scale actually needs. CkAStar itself
  untouched.
- **[C-D4]** Graph layer addresses abstract cells from day 1; node merging (the ~4× leaf-count /
  sub-1ms lever) lands in Phase 5 behind that abstraction with an A/B benchmark gate.
- **[C-D5]** The box-occupancy primitive + static-domain filters + first `BroadPhaseQuery` wrapper
  live in **CkJolt**, not CkVoxelNav. Keeps the JPH allowlist closed and the capability reusable.
- **[C-D6]** Commits stay local on `feature/ckvoxelnav-port` (CkFoundation; CkTests gets a same-named
  branch when tests land). No pushes / pointer bumps / dev delivery without an explicit maintainer
  go. (Maintainer's standing goal authorizes the campaign work itself, not publication.)
- **[C-D7]** Scale premise (maintainer, 2026-08-03): CkFoundation must serve future horde-scale
  games (hundreds-to-thousands of navigating enemies), not just Rewind99 (~150 agents). Immutable
  post-bake octree (lock-free parallel reads) is a design requirement, not a nicety.
- **[C-D8]** Recorded follow-ups OUTSIDE this campaign's scope: (a) batch probe AABB updates
  (`CkProbe_Processor.cpp:711` → `NotifyBodiesAABBChanged(ids,N)`); (b) adopt `Get_OverlapEntities`
  / broadphase tier at Compass/Minimap/EQS iterate-all sites; (c) `CkSpatialHash` per-frame-rebuilt
  proximity grid module (kNN + body-free entities) when the first massed consumer lands;
  (d) CkRaySense/CkOverlapBody Chaos→Jolt consolidation.
- **[C-D9]** Phase 3 must audit Nav3D's dynamic-occlusion machinery before porting it — upstream
  issue #39 reports it broken in v2.0. Port the design, verify the behavior, don't trust the code.
- **[C-D13]** UFUNCTION categories are FEATURE-named, not module-named: `Ck|Utils|VoxelNavVolume` /
  `[Ck][VoxelNavVolume] ...` (matches the utils class per CkTimer precedent; port-map §5.4 form
  wins over the 0C brief's `Ck|Utils|VoxelNav`). Future features (path/follower) get their own.
- **[C-D16]** ROOT-CAUSE FIX to the host project (outside the plugin, in scope as gate repair):
  add `OnlineSubsystem` + `OnlineSubsystemUtils` (Enabled) to `CkPlugins.uproject`'s Plugins array.
  Diagnosis (Opus, verified end-to-end): `1357343` "perf: slim the editor startup plugin set"
  (2026-07-31) set `DisableEnginePluginsByDefault: true` without those two in the closure →
  `GameNetDriver`/IpNetDriver can never be created → every `FCk_Latent_StartPIEMultiClient` PIE
  (80 files in CkTests) emits `NetDriverCreateFailure`+`EnableListenServer` ensure noise → every
  C++ automation test using the fixture fails; AS functional tests are immune (log capture gated
  to the functional-test window). Precedent: BusterBlock.uproject lists exactly these two under
  the same slim flag; same repair shape as `aaa2a9a` (Content Browser closure patch). Rejected:
  harness/per-test whitelists (mask a real misconfig), standalone-PIE fallback (breaks
  Get_ServerWorld contract for 80 tests). Undo = delete the two entries. NOTE: partially trims
  `1357343`'s startup-perf intent — flag to maintainer in the session report.
- **[C-D17]** GATE DEFINITION AMENDED: the standard full-suite `--test` run EXCLUDES the
  `Ck.<Feature>.*` C++ automation family (EngineFilter flags — verified: zero `Ck.Jolt.*` rows in
  the full-suite run's name list; my earlier "exemplar passes in-suite" premise was WRONG — it
  never ran there). Phase gates are therefore: full suite (delta-zero vs baseline) PLUS targeted
  C++ patterns (`Ck.VoxelNav`, `Ck.Jolt.Query`, and future phase patterns) green. VALIDATION.md
  inherits this definition.
- **[C-D18]** SPEED POLICY (maintainer directive 2026-08-04: "reduce tests, complete quickly"):
  (a) during-phase verification = TARGETED patterns only (`Ck.VoxelNav*`, `Ck.Jolt.Query`, the
  phase's new tests — ~40-60s each); (b) full suite runs ONCE per phase boundary, `--parallel 1`
  (project memory: lanes false-red), no re-samples — a single-occurrence new red in a known-fragile
  FOREIGN family (PathNetworkFollower, the Jolt-parity crash) is recorded as flake-observed and
  does NOT block the phase if a solo re-run passes; only deterministic or own-code reds block;
  (c) pre-warm the toolbox warm server during test-authoring iterations (`--live` routing, zero
  boot), with the phase-boundary run staying a fresh boot (gate-of-record rule); (d) fewer, larger
  dispatch waves (Phase 1: 1A∥1B as wave 1, 1C+1D as one sequential wave 2, 1E with the boundary).
- **[C-D10]** (OQ-1, blocker) 0B's consumer surface must be JPH-free: CkJolt ships opaque
  TPimplPtr-backed `FCk_Jolt_QuerySession` + `FCk_Jolt_BoxProbe` value types; the JPH-signature
  functions stay CkJolt-internal. Rejected alternative: allowlisting CkVoxelNav for direct JPH
  includes — would hollow out the Jolt-agnostic backend the maintainer explicitly asked for.
  PHASE_0.md 0B amended.
- **[C-D11]** (OQ-2) Voxel build processors: `FGroup_Transform`, `RunAfter
  FProcessor_JoltWorld_WaitForAsync` + `RunBefore FProcessor_JoltWorld_Step` — the only window
  provably outside the async step. Adjacent finding recorded as follow-up (e) under [C-D8]:
  CkEqs queries Jolt from `FGroup_PostTransform`, i.e. after Step dispatches the async batch — a
  latent async-mode exposure, NOT ours to fix in this campaign.
- **[C-D12]** The port map's recommendations (research/phase1-port-map.md §6 OQ-3..OQ-10) are
  adopted as written — incl. OQ-9 rename `_VoxelExtent`→`_FinestCellSizeUu` (PHASE_0 amended),
  OQ-4 landscape fence (packaged builds have no Jolt landscape outside WITH_EDITOR → volumes over
  landscape voxelize as free; ensure via `Get_NumStaticBodies` sanity signal + doc fence), drop of
  the L1 overlap cache (hierarchy provides the pruning; Jolt any-hit replaces the triangle
  narrowphase). Any implementation-time deviation needs a new decision entry.

## Blockers

- (none)

## Next step

Baseline gate completes → record counts above → dispatch Phase 0 units 0A/0B/0C (parallel, opus)
then 0D → orchestrator re-runs gate → phase boundary ritual → author PHASE_1.md.

## Session log

- 2026-08-03 — orchestrator: Fable 5 (interactive). Research (7 Opus dossiers, ~1.3M subagent
  tokens) → plan → campaign opened: branch, doc set, baseline started. Routing: all execution
  units → opus; judgment/audit inline at orchestrator.
- 2026-08-04 (same session) — PHASE 0 executed and CLOSED: 4 opus units (0A-0D) + port map +
  diagnosis agent; rulings C-D10..C-D18; root-caused and fixed the project-wide missing
  OnlineSubsystem closure ([C-D16], superproject commit). Speed policy [C-D18] adopted on
  maintainer directive. Commits: CkFoundation d401a54aa/13e30ab1f/49da3aeb5/1726f6f0d + docs;
  CkTests `test(voxelnav)` commit; superproject uproject fix commit. Phase 1 wave 1 dispatched.
