# design_PathShortcut — jagged GroundNav routes over open floor

Read-only design pass, 2026-09-06. CkPlugins paths relative to
`D:\Repos\CkPlugins_3\Plugins\CkFoundation\Source\` unless stated; JoltNav paths to `D:\Repos\JoltNav\`.

---

## §1 Root cause — the mechanism is HALF right. The funnel is not the defect; three other things are.

`Do_Funnel` (`CkGroundNav/Query/CkGroundNav_Funnel.cpp:144-236`) is a correct simple-stupid funnel over
portal **EDGES** — it carries `_LeftVertex`/`_RightVertex` from `InPortals[Index]._Left`/`._Right`
(`:177-178`) and restarts the apex on the vertex just emitted (`:193-203`, `:220-230`). It *does*
string-pull across the union of the corridor's plates — **for the corridor it is handed**. It is the
same algorithm JoltNav ships (`Jolt2DNavigation/Private/NavGraph2D/Query/MerNavGraph2DQueryGround.cpp:126-357`,
self-described "Detour style funnel" at `:122`). The bug is *which vertices it is handed* and *what
happens to the apex afterwards*.

**(1a) — the larger culprit: the corridor CHOICE, priced portal-midpoint to portal-midpoint.**
A search node is a crossing arrival whose arrival/departure point is the crossing's raw **midpoint**
(`CkGroundNav/Search/CkGroundNav_PlatePortalGraph.h:160-170`; `Get_CrossingTransitionPoint`). `Cost`
prices a leg `Get_LegCost(departure-midpoint → arrival-midpoint)`
(`CkGroundNav_PlatePortalGraph.cpp:392-398`) and `Heuristic` measures from the departure midpoint
(`:594-600`). A* therefore minimises **the midpoint polyline**, not the funnelled length. On a slab cut
into rectangles the two disagree, and the chain whose midpoint polyline is shortest is frequently not
the chain containing the straight line. The funnel then answers the taut path of a channel that
genuinely does not contain it — a real bend, in open floor, far from any pillar. The suite already
knows the two objects differ and measures the gap
(`Plugins/CkTests/Source/CkTests/Private/UnitTests/CkGroundNav/Test_GroundNav_FunnelImprovement.cpp:1-15`);
what nobody noticed is that the chain being pulled was **chosen by the other metric**.
**Yes — corridor choice is the larger culprit.**

**(1b) — false apexes from an inset by AGENT RADIUS at open-floor portal ends.** `Get_Inset`
(`Funnel.cpp:92-110`) pulls both ends of every portal toward its midpoint by the agent radius, applied
unconditionally (`:321-322`). A portal is the full contiguous run of a shared plate boundary
(`CkGroundNav/Bake/CkGroundNav_Portals.h:20-22, 42-45`) and a plate is an axis-aligned **merged
rectangle** of cells (`CkGroundNav/Bake/CkGroundNav_Plates.h:69-87`). Where a rectangle boundary ends
**in open floor** — every place the decomposition split the slab for shape reasons rather than for an
obstacle — the inset moves the usable interval a radius inside free space and the string bends around
a wall that is not there.

*JoltNav insets too* (`MerNavGraph2DQueryGround.cpp:185-198`) — but by **one cell**, not an agent
radius (`:130-131`, `constexpr float tolerance = 1.0f;` in cell units), because their agent radius is
eroded into the mesh at **build** time (`Builder/MerNavGraph2DBoxBuilderGround.cpp:918`
`rcCfg.walkableRadius = ceil(agentWidth*0.5/cs)`, consumed by `rcErodeWalkableArea` at `:1671`). One
field per agent profile buys them a query-time inset an order of magnitude smaller than ours.

**(1c) — the corner offset AMPLIFIES a false corner by a full radius and cannot refuse to.**
`Get_CornerOffset` (`CkGroundNav/Search/CkGroundNav_PathPostProcess.cpp:328-386`) moves each interior
waypoint to `Corner - Bisector * (_CornerOffsetK * radius)`, bisector pointing into the concave side
(`:358-373`); `_CornerOffsetK` defaults to `1.0` (`CkGroundNav/Search/CkGroundNav_SearchTypes.h:112`).
At a real inside corner that pushes off the wall — correct. At a **false** corner it pushes further
from the straight line. And the probe cannot stop it: `Get_IsCornerAccepted` returns `true`
unconditionally when `Get_ClosestBoundary` finds no wall in the probe radius (`:252-256`, *"No answer
means no wall inside a radius … open floor, and the strongest pass the probe can report"*). **The pass
succeeds at full magnitude exactly where it is most wrong.** Net jog at a false corner ≈ 2 radii.

Stage order in `Get_PathPlan` (`:510-565`): funnel → link endpoints → corner offset → skip-first →
fill → link stamp. Nothing between them re-tests the polyline against the field.

**Why a polygon mesh has no false corners and rectangles do.** Recast's greedy triangle→convex-polygon
merge (`Builder/Recast/RecastMesh.cpp:1145-1190`, `getPolyMergeValue:416` — merge value = squared
shared-edge length, so the longest interior edges dissolve first) **removes** interior edges rather
than enumerating them, and contour simplification (`MerNavGraph2DBoxBuilderGround.cpp:2542-2545`,
`maxSimplificationError = 1.4`, `maxEdgeLen = 0`) leaves long boundary edges. Few long portals ⇒ the
funnel's side tests (`MerNavGraph2DQueryGround.cpp:217-259`) rarely fire ⇒ apexes only at genuine
obstacle silhouettes. A rectangular decomposition inverts all three: many short axis-aligned portals,
so a diagonal crossing exits the funnel on alternating sides and emits an apex per boundary.

**The gym scene** (`Plugins/CkTests/Script/CkGroundNav/CkGroundNavGym_Walk_PlayerController.as:16-28`):
3600×2400 slab (X ±1800, Y ±1200), four 150×150×300 pillars at (-900,-60), (-300,120), (300,-100),
(900,80); W0 runs west→east (`:351-352`). Four *staggered* pillars is near-worst-case for (1a).

---

## §2 Options

**Which of the three our gap corresponds to — none of them; it corresponds to a FOURTH thing JoltNav
ships that the brief did not name.** Their funnel is ours. Their follow-time raycast shortcut
(`Private/Path/MerPathPointList2D.cpp:13-78`) absorbs follower drift and replan seams, not
representation artefacts (default budget: **≤2 raycasts per call** — `furthestPointToShortcut = 2`,
`:21, :35`). Their curve builder (`Private/Path/MerCurveBuilder2D.cpp:1377-1464`) is cosmetic.
But they *also* run **`MerNavGraph2DQueryGround::StraightenPath` (`:1938`) at plan time** — and the
funnel deliberately refuses to straighten across cost-multiplier boundaries and defers to it
(`:170-181`: *"for cost multiplier changes, don't want to straighten the path … StraightenPath will
perform adjustments that take into account total cost later"*). **Option (A) below is our version of
that pass. The reference implementation already validates the shape.**

Cost units: `n` = plan waypoints (3–12 here), `P` = corridor plates, `L` = segment length, cell = 25 uu
(`CkGroundNav/Bake/CkGroundNav_BakeTypes.h:113`). One `Get_SurfaceRaycast`
(`CkGroundNav/Query/CkGroundNav_Query_SurfaceWalk.cpp:558-668`) costs **O(1)** — one cell read — when
both ends lie in one plate whose `_MinClearanceUu` admits the body (`Get_PlateEarlyOut`, `:599-610`);
otherwise a DDA cell walk of ~`L/25` steps (~`1.4·L/25` diagonal), each a `Get_StepAcross` admission
test. It is radius-aware and, being a *surface* walk, inherently rejects steps a body cannot take —
the same guarantee JoltNav buys with its `bCheckHeights` flag (`MerNavGraph2DQueryGround.cpp:2404-2422`).
It is **not** plate-cost aware: `kCostMultiplier = 1.0` (`Query_SurfaceWalk.cpp:23`), so
`_AccumulatedCost` is pure distance and `_MaxCost` is a distance cap. The traversal already detects
`CrossedAPlate` (`:247-250`).

### (A) Plan-time line-of-sight shortcut, after the corner offset — **RECOMMENDED, primary**
A sixth stage in `Get_PathPlan`, between `Get_CornerOffset` and `Get_SkipFirstWaypoint`
(`PathPostProcess.cpp:541-549`). From waypoint `i`, walk `j` down from the span end to `i+2`; accept
the first `j` whose segment raycasts clear **within a cost budget**; drop `(i+1 … j-1)`.
- **Cost-awareness — copy JoltNav's budget form, not a set test.** `MerPathPointList2D.cpp:37-39`:
  `maxCost = m_path[j].averageCostMultiplier * m_path[j].pos.Distance(newFrom)`, passed as the
  raycast's `maxTotalCost`; the ray accumulates real per-cell cost and fails if it exceeds it. Our
  analogue: budget = (max `Get_AreaMultiplier` over the waypoints being replaced) × chord length, fed
  to `FCk_GroundNav_RaycastQuery::_MaxCost` once the traversal weights each cell by its plate's
  multiplier. That makes *"a shortcut never crosses a plate dearer than the corridor paid"* fall out
  of arithmetic the search already owns, rather than a second rule.
- **Cost bound:** worst case `n(n-1)/2` raycasts; at `n ≤ 12` that is ≤ 66, and in open floor nearly
  all hit the single-plate O(1) early-out. Runs **once per completed search**, game thread, in
  `DoPublish_Success` (`CkGroundNav/Path/CkGroundNavPath_Processor.cpp:395`) — not per tick, not
  inside the sliced search (`FProcessor_GroundNavPath_Slice`, `:654-745`).
- **Link endpoints pinned, never crossed.** The pinned set already exists (`Get_LinkWaypoints`,
  `PathPostProcess.cpp:48-62`, exact equality); treat each pinned point as a hard span split.
  JoltNav gets this structurally — nav links are split into their own `MerNavLinkPathSection` at plan
  time (`JoltCore/Public/Paths/MerPath.h:1707-1731`), so no shortcut can span one — and its waypoint
  immunity is the same idea (`MerPathPointList2D.cpp:26-33`; `MerCurveBuilder2D.cpp:1388-1393`
  *"Don't drop waypoints"*). We have one flat array, so we must pin explicitly.
- **Failure mode:** without the cost budget a shortcut silently undercharges — it cuts the corner of a
  cheap plate into an expensive one and the plan's `_CostFromStart` stops matching what the search
  priced. That is the one piece that must be built, not assumed.

### (B) Follow-time incremental shortcut, JoltNav-style
The crowd follower **already has the mechanism**: `CkCrowd/Agent/CkCrowdAgent_Steering_Processor.cpp:156-193`
raycasts the chord to `Waypoints[i+1]` via `UCk_Utils_NavSurface_UE::Try_SurfaceRaycast` before
retiring a waypoint; `CkCrowdAgent_OnPathResolved_Processor.cpp:274-296` does the same at install.
Retiring a waypoint the agent can *see* rather than one it has *reached* is a ~10-line change.
- **Cost bound:** 1 raycast per agent per tick at lookahead 1 (JoltNav's default is ≤2, gated by a
  movement threshold at `JoltCore/Public/Paths/MerLinePathSection.h:235`, so a stationary agent casts
  nothing). At the B4 reference agent count this lands **per frame** — on the metric being protected.
- **Failure mode:** straightens the *followed* line, leaves the *plan* jagged — `ck.GroundNav.PathAt`,
  `_LengthUu` and every path pin still show the jog. It cannot touch (1a). **Do not ship alone.**

### (C) Funnel / search-cost change
- **C1 — stop insetting at open-floor portal ends.** Removes (1b) at source. *Failure mode:*
  `Get_Inset` **is** the funnel, shared with the flood fill (`Get_StringPull_ToSegment`,
  `Funnel.cpp:354-385`) and `Get_FloodDistanceTo`; every flood distance and reference-distance pin
  moves. High blast radius. (JoltNav sidesteps this entirely by eroding at build time — for us that
  means a field per agent radius, which the campaign's profile-variant work already half-implies.)
- **C2 — price A* legs on the point the funnel would use, not the midpoint.** The real fix for (1a),
  and expensive: an incremental funnel carried per search node. *Failure mode:* the node key, the
  `w = 1` admissibility argument (`SearchTypes.h:157-159`, `PlatePortalGraph.cpp:265-270`) and every
  recorded search budget (`Test_GroundNav_ReferenceNumbers.cpp:536-539`) are all stated in the
  midpoint metric. A phase, not a unit.

### (D) Curve smoothing on top
Cosmetic. JoltNav's `DropPoints` is *not* naive smoothing — it re-validates every candidate with a
parabola-cast that clamps to the surface (`MerCurveBuilder2D.cpp:1422`, `SandwichCastAndClampToSurface`
at `:1819`) and refuses to lower friction / raise cost (`:1416-1419`). That is a project, and it would
hide (1a) rather than fix it. **Not now.**

### Recommendation
**(A) alone this batch.** It fixes (1b) and (1c) outright and *masks* (1a) — a chord across the union
of the corridor's plates recovers the straight line whenever the wrongly-chosen chain still contains
it, which on an open slab is almost always — at a cost paid once per query. Then **(B) behind a cvar,
default off**, once (A)'s benchmark is on the board. Skip (D). Park (C2) by name.

**Any wall-clock number must come from a benchmark; nothing above estimates one.** The benchmark:
1. **The existing B4 metric** — completed queries per frame at the reference agent count
   (`docs/campaigns/navigation-next/PROGRESS.md:1959`, ruling `[NN-D93] F4` at `:2621`), run with the
   pass off and on; the delta is the claim.
2. **A C++ automation timing test** mirroring the fixture style of
   `Test_GroundNav_ReferenceNumbers.cpp` — specifically
   `FCkTest_GroundNav_Reference_SearchBudgetsAreStableAndRecorded` (`:536-539`): fixed stub geometry,
   a fixed query population, `_Cost._CellsRead` and `_ExpansionCount` recorded and **pinned as counts**
   plus the pass's own raycast count. Counts, not milliseconds — the suite's existing habit and the
   only machine-independent number.

---

## §3 Units, definition of done, test pins

Disjoint ownership; U1 lands before U2.

| Unit | Owns (exclusive) | Work |
|---|---|---|
| **U1** | `CkGroundNav/Query/CkGroundNav_QueryTypes.h`, `CkGroundNav/Query/CkGroundNav_Query_SurfaceWalk.{h,cpp}` | Make the raycast cost plate-weighted **opt-in**: add `TMap<int32,float> _PlateCostMultipliers` to `FCk_GroundNav_RaycastQuery`; in the traversal weight each cell segment by `Get_AreaMultiplier` where `CrossedAPlate` is already detected (`:247-250`), and on the single-plate early-out (`:599-610`). Empty map ⇒ `kCostMultiplier = 1.0` and byte-identical behaviour. `_MaxCost` becomes a true cost budget, matching JoltNav's `maxTotalCost`. |
| **U2** | `CkGroundNav/Search/CkGroundNav_PathPostProcess.{h,cpp}` | New pure stage `Get_Shortcut(InWaypoints, InPinnedWaypoints, InField, InCost, InAgent, InVerticalToleranceUu) -> TArray<FVector>`, inserted between the corner offset and skip-first (`:541-549`). Greedy farthest-visible within each pinned span; a candidate is accepted only when the raycast is `Success` under a budget of (max `Get_AreaMultiplier` over the replaced waypoints) × chord length. New `_ShortcutSpanCap` on `FCk_GroundNav_PathCostParams`; zero switches the pass off, mirroring `_CornerOffsetK = 0`. |
| **U3** | `.../UnitTests/CkGroundNav/Test_GroundNav_PathShortcut.cpp` (new), `Test_GroundNav_QueryFixtures.h` (append only) | `Make_FourPillarSlabScene()` / `Bake_FourPillarSlabScene()` mirroring the Walk gym numbers verbatim, plus pins 1–4. |
| **U4** | `Plugins/CkTests/Script/CkGroundNav/CkAutoTest_GroundNav_Path_ShortcutAcrossTheFourPillarSlab.as` (new) | AS autotest mirroring the Walk gym scene through the neutral facade. |
| **U5** (after benchmark) | `CkCrowd/Agent/CkCrowdAgent_Steering_Processor.cpp` | Option (B) behind `ck.Crowd.Steering.VisibilityShortcut`, default 0. |

**Definition of done.** Pins below green; `GroundNav` delta-zero against the last recorded gate
(407/407, `P8-B1-Final4.log`, `PROGRESS.md:1929`) plus the new tests; `Crowd`/`Nav` baseline-only red;
the B4 metric recorded off and on. `[EDITOR-VERIFY]`: W0's route under `ck.GroundNav.PathAt` shows no
interior waypoint standing in open floor.

**Test pins (U3 unless noted).**
1. **`Path_FourPillarRouteHasNoFalseCorners`** — plan W0's west→east query; for every interior
   waypoint `Get_ClosestBoundary` must answer `Success` with
   `_DistanceUu <= radius + _CornerOffsetK*radius + oneCell`, i.e. **every remaining corner is a real
   corner**. Same query shape as `Test_GroundNav_PathPostProcess.cpp:244-250`.
2. **`Path_ShortcutKeepsEveryLinkEndpoint`** — reuse the barrier-with-a-link fixture from
   `Test_GroundNav_PathLinkMetadata.cpp` (`Bake_Barrier`, `Make_LinkRecord`, `:619-621`); assert the
   plan still carries exactly one Entry/Exit pair and both stamped locations equal the resolved
   endpoints **exactly**.
3. **`Path_ShortcutNeverLowersThePlateCostPaid`** — a markup fixture where the straight chord crosses
   a cost-marked plate the corridor routed around; assert `Plan._Waypoints.Last()._CostFromStart`
   after the pass is `<=` the value before, and that no accepted chord's weighted raycast cost
   exceeded its budget.
4. **`Path_ShortcutIsIdempotent`** — running the pass over its own output changes nothing.
5. **(U4, AngelScript)** spawn the slab + four pillars with the constants at
   `CkGroundNavGym_Walk_PlayerController.as:16-28` via `CkGroundNavGym::Spawn_Box`, request a path
   west→east through the neutral facade, assert the waypoint count is ≤ the pre-pass count and the
   polyline length is within a cell of the straight-line distance where that line is clear.

**Pins a shortcut WOULD perturb — checked.**
- `FCkTest_GroundNav_Path_LCorridorThreeWaypointsAllClearOfBoundary`
  (`Test_GroundNav_PathPostProcess.cpp:236-241`, `kBentOnceIsThreeWaypoints = 3` at `:93`) — the L
  corridor's one bend is a **real** corner. This is the over-shortcutting regression pin; a red here
  is a bug in U2, never a pin to update.
- `Reference_NumbersAreStableAndRecorded` / `Measure_LCornerBoundaryUu`
  (`Test_GroundNav_ReferenceNumbers.cpp:397-431`, `kBendsOnce = 3` at `:315`) — reads
  **`Get_Funnelled` directly**, not `Get_PathPlan`, so a stage inside `Get_PathPlan` cannot perturb
  it. Verified by reading the body.
- `PathLinkMetadata.MetadataSurvivesSkipFirstAndCornerOffset`
  (`Test_GroundNav_PathLinkMetadata.cpp:607-646`) — the "drops exactly one waypoint" assertion (`:645`)
  is relative and survives, but `Kept._Waypoints.Num() >= kFewestPointsWithAnInterior` (`:631`) can
  break if a shortcut collapses that route to two points. **Re-run and reason about it in U2.**
- `CkAutoTest_GroundNav_Shadow_InstalledPathIsByteIdenticalToRecast.as` — **NOT affected.** Despite the
  name it compares a Recast leg A against a Recast leg B under shadowing (file header lines 13-16);
  GroundNav's waypoints are never compared to Recast's. The brief's worry does not bind here.
- `Facade.Equivalence_FacadeAnswersEqualDirectCalls`
  (`Test_GroundNav_Facade_Equivalence.cpp:109-112`) — facade vs direct call on the same provider;
  moves in lockstep. Unaffected.

---

## §4 Forks for the orchestrator

- **F-1 — (A) alone, or (A) + (B)?** *Recommend (A) alone.* (B)'s cost lands on the exact per-frame
  budget B4 measures; shipping both makes the B4 delta unattributable.
- **F-2 — cost-awareness shape: weighted-raycast budget (U1) or a plate-set membership test?**
  *Recommend U1's budget*, copying `MerPathPointList2D.cpp:37-39`. The set test is cheaper to write
  and wrong: leaving the corridor's plate set is precisely what a shortcut must be allowed to do.
- **F-3 — is (1a) accepted as parked?** *Recommend yes — park C2 as a named decision*, reason
  recorded: it changes the node key, the admissibility argument and every recorded search budget. If
  (A)'s benchmark shows routes still jagged because the chosen chain never contained the straight
  line, C2 returns as a phase.
- **F-4 — span cap.** A cap of 4 bounds the pass at `4n` raycasts; no cap is `n²/2` but always finds
  the best answer, and at `n ≤ 12` that is small. *Recommend no cap by default* (`_ShortcutSpanCap = 0`
  meaning unbounded is confusing — prefer `0` = off, `MAX_int32` = unbounded), and let the benchmark
  argue otherwise.
- **F-5 — one field per agent radius (JoltNav's build-time erode)?** Out of scope here, but it is the
  only fix that removes (1b) *and* shrinks every query-time inset. Worth recording against the
  profile-variant work rather than deciding now.

---

## §5 Risks, and what I could not verify

**Risks.**
- **(A) masks (1a), it does not fix it.** If a corridor weaves to the *wrong side* of a pillar, no
  chord across its own plates recovers the other side. Pin §3.1 is deliberately phrased "every
  remaining corner is a real corner" — such a route would still satisfy it while looking wrong. Only
  the `[EDITOR-VERIFY]` walkthrough catches that.
- **Raycast admission vs search admission.** `Get_IsAdmitted` at search time and `Get_StepAcross` in
  the walk are meant to be the same rule; if they have drifted, a shortcut cuts through ground the
  search refused. I read both call sites; I did **not** prove them equivalent.
- **U1 changes a shared query type.** `_MaxCost` currently means "distance cap"; after U1 it means
  "cost budget" whenever the map is non-empty. Every existing `_MaxCost` caller must be enumerated.
- **`_LinkWaypoints` is index-keyed** into `_Waypoints` (`Source/CkGroundNav/Claude.md:300-306`). The
  stamp runs after the fill and matches by exact position, so dropping waypoints before the fill is
  safe — a future consumer caching indices across the pass is not.
- **Cost monotonicity is asserted, not proved.** Pin §3.3 checks the shipped plan, not the greedy
  accept order.

**Not verified.**
- **No measurement of anything.** No build, no test, no PIE. Every cost in §2 is a count bound derived
  from code, never a wall-clock number.
- **I did not bake the four-pillar scene and count plates.** That it yields rectangle corners in open
  floor follows from `FCk_GroundNav_Plate` being axis-aligned (`Plates.h:76-87`) and portals being runs
  along shared boundaries (`Portals.h:20-22`) — an inference, not an observation.
- **The B4 harness.** `[NN-D93] F4` defines the metric but PerfLab's generator half is parked on
  `[P6-B1]` (`PROGRESS.md:1959-1960`); I could not confirm a runnable harness exists today.
- **JoltNav citations are second-hand** — from a parallel read-only reference-study agent, not my own
  reads. Two load-bearing findings to re-check before coding against them: their `Raycast` returns
  **`true` for no hit** (`MerNavGraph2D.cpp:7569-7575`), and their one-cell inset is applied
  **unconditionally** — the corner-only behaviour is emergent from the merged convex mesh, not from a
  conditional.
