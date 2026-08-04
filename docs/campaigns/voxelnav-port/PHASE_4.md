# PHASE 4 — Consumers: crowd integration + flying agents

> Freshness: authored 2026-08-04 at the 3→4 boundary. Status of record: PROGRESS.md. Spec detail:
> `research/research-navStack.md` — the provider seam (§ "CkNavigation — the path-provider seam"),
> the CkCrowd wiring exemplar (§ "Crowd wiring to copy": `FProcessor_CrowdAgent_OnRouteResolved`
> is ~190 lines and the integration IS a rename of it), and the flying blockers (§ "What BLOCKS
> flying agents", file:line per blocker). This phase EDITS CkCrowd — first foreign-module surgery
> of the campaign; surgical rule applies (match existing style, no adjacent improvements).

## Entry criteria

- [ ] Phase 3 closed. Baseline: full suite 981/975/6 (six known reds — five of them in the
      PathNetworkFollower family this phase works NEXT TO; delta-zero still means exactly six).

## Units

### Wave 1 — 4A: the third provider branch (one agent)

1. `CkCrowd.Build.cs` + `"CkVoxelNav"` (T4→T4, legal). CkVoxelNav itself is NOT edited except
   additively if a getter is missing.
2. Provider selection (`CkCrowdAgent_HandleRequests_Processor.cpp:116-132`): a third branch ABOVE
   the PathNetworkFollower branch — agent has the VoxelNavPath feature (+ a bound volume) →
   `FCk_Nav_Algorithm::MarkPathPending` + `UCk_Utils_VoxelNavPath_UE::Request_FindPath`. Mimic the
   existing branch's shape exactly (incl. its BlockDetect/PathRefresh MarkPathPending call sites
   ONLY if they apply — flying agents exclude PathRefresh, see 4B).
3. `FProcessor_CrowdAgent_OnVoxelPathResolved` — the rename of `OnRouteResolved` (gate tags,
   goal-match tolerance, installed-route dedupe by volume epoch + goal, `InstallExternalPath`,
   waypoint-index reset, mid-walk retire, Failed → `Request_NavigationPath` fallback behind a
   pending tag). Register + group placement copied from the exemplar.
4. Binding API: `Request_SetVolume(path, volume)` already exists or is added on the VoxelNavPath
   utils (the follower's `Request_SetNetwork` precedent) so a crowd agent knows which volume to
   plan against.

### Wave 2 — 4B: flying-agent enablement (one agent, after 4A)

1. `FTag_CrowdAgent_Flying` (registered native tag if replication needs it — check how CkCrowd
   registers its tags): excluded by the views of `ConstrainToNavmesh` (the single transform
   writer — THE hard blocker), `StationaryMarkup`, `PathRefresh`, `AvoidanceSample`, `Separation`,
   `PushApart`, `FaceAngle`. Each is a view-filter edit, not logic surgery.
2. `FProcessor_CrowdAgent_FaceAngle3D` gated ON the Flying tag: yaw + pitch toward the velocity
   (no roll/banking v1), mimicking FaceAngle's smoothing constants.
3. Params opt-in: `_AgentMode` enum (Grounded default / Flying) on the crowd-agent params → Add
   stamps the tag. No global settings knob.
4. Tests: PIE — a Flying crowd agent with the VoxelNav path feature receives its waypoint polyline
   through `FFragment_Nav_PathResult` (assert provider-agnostic delivery), moves along a 3D course
   without floor-snapping (Z tracks the waypoints within tolerance), and a Grounded agent in the
   same world is untouched (still navmesh-constrained). Hermetic where possible (tag exclusion =
   view membership assertions per gotcha (m) if practical).
5. `[EDITOR-VERIFY]` (VALIDATION.md item): flying agent traverses a VoxelNav gym course in PIE —
   exact steps written into VALIDATION.md by this unit.

## Exit criteria

- [ ] Targeted `Ck.VoxelNav` (53 + new) green; the crowd integration tests green.
- [ ] Full suite delta-zero — EXACTLY the six known reds: the PathNetworkFollower family must be
      bit-for-bit the same failures (we edited beside them); ONE sample.
- [ ] `git diff --stat` over CkCrowd reviewed by the orchestrator (surgical-scope check).
- [ ] Comment audit; PROGRESS updated; PHASE_5.md authored at the boundary.

## Fences

- CkCrowd edits are SURGICAL: the third branch, the resolver processor, the tag exclusions, the
  3D facing, the params enum — nothing else. No refactors of the provider if/else into a registry
  (recorded as a future fork in the navStack dossier, NOT this campaign's call).
- CkNavigation: read-only (the seam functions already exist).
- No avoidance/separation replacement for flying agents (3D VO is new work — deferred pool).
- The six known PathNetworkFollower reds are FOREIGN: if any flips (red→green or new red), stop
  and investigate before closing — do not hand-wave a flip as improvement.
