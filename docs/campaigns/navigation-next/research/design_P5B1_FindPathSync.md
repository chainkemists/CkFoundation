# [P5-B1] design — a synchronous path query on the NavSurface facade (option 1a)

Read-only pass, 2026-09-06. Every line ref re-verified against the working tree this session.
Path legend (all under `Plugins/CkFoundation/Source/`):
`NS/` = `CkNavigation/Public/CkNavigation/NavSurface/` · `NAV/` = `CkNavigation/Public/CkNavigation/Nav/`
`GN/` = `CkGroundNav/Public/CkGroundNav/` · `PN` = `CkPathNetwork/Public/CkPathNetwork/Network/CkPathNetwork_Processor.cpp`

---

## §1 What exists

### 1.1 The facade capability table — 14 entries, none a path

`FCk_NavSurface_ProviderTable`, `NS/CkNavSurface_ProviderTable.h:20-78`:

| # | Capability | Decl | Neutral util |
|---|---|---|---|
| 1 | `_ProjectPoint` | `:24` | `Try_ProjectPoint` `NS/CkNavSurface_Utils.h:95` |
| 2 | `_MoveAlongSurface` | `:27` | `Try_MoveAlongSurface` `:104` |
| 3 | `_SurfaceRaycast` | `:30` | `Try_SurfaceRaycast` `:113` |
| 4 | `_BoundarySegments` (off-thread by contract, `:32-35`) | `:36` | `Get_BoundarySegments` `:121` |
| 5 | `_IsReachable` | `:39` | `Get_IsReachable` `:131` |
| 6 | `_SurfaceBounds` | `:42` | `Get_SurfaceBounds` `:180` |
| 7 | `_ProviderHealth` | `:45` | `Get_ProviderHealth` `:188` |
| 8 | `_IsBuildInProgress` | `:48` | `Get_IsBuildInProgress` `:196` |
| 9 | `_IsSurfaceSettled` | `:55` | `Get_IsSurfaceSettled` `:216` |
| 10 | `_SurfaceRevision` | `:58` | `Get_SurfaceRevision` `:172` |
| 11 | `_RequestSurfaceRebuild` | `:61` | `Request_SurfaceRebuild_ForTesting` `:224` |
| 12–14 | `_ApplyAreaMarkup` `:67` · `_IsMarkupLive` `:70` · `_ReleaseAreaMarkup` `:76` | | `Request_AreaMarkup` `:140`/`:155`, `Get_IsMarkupLive` `:164` |

Two hard constraints on any addition:
- `Get_IsComplete()` (`NS/CkNavSurface_ProviderTable.cpp:90-107`) ANDs all 14; `Register_Provider`
  refuses an incomplete table with `CK_ENSURE_IF_NOT` (`:118-123`). **Adding a 15th entry without
  filling it in BOTH provider tables in the same change refuses both registrations at module startup
  — a total navigation outage, not a compile error.**
- Two providers only: `ECk_NavSurface_Provider {Recast, GroundNav}` (`NS/CkNavSurface_Fragment_Data.h:32-37`).
  Recast fills the table in `CkNavigation/CkNavigation_Module.cpp:16-58`; GroundNav in
  `GN/Facade/CkGroundNav_NavSurfaceAdapter.cpp:636-651`.

Status vocabulary already in place: `ECk_NavSurface_QueryStatus {Success, NoSurface, Unbuilt, Blocked,
NoProvider}` (`NS/CkNavSurface_Fragment_Data.h:54-65`) and `ECk_NavSurface_ProviderHealth {Ready,
Building, NoData, Error}` (`:93-99`).

### 1.2 How GroundNav answers a path today

Deferred, entity-scoped, frame-sliced — `UCk_Utils_GroundNavPath_UE::Request_FindPath`
(`GN/Path/CkGroundNavPath_Utils.h:145-160`; contract comment `:141-151`: sliced, parks on unbuilt
ground, a second request supersedes the first). The episode runs in
`FProcessor_GroundNavPath_HandleRequests` (`GN/Path/CkGroundNavPath_Processor.cpp:556+`), slicing at
`:731-736` with `FCk_GroundNav_PathSliceParams`.

**The machinery underneath is already synchronous and entity-free:**
1. `ck::groundnav::world_fields::TryGet_Field(World, Location, ProfileTag)`
   (`GN/Facade/CkGroundNav_WorldFieldRegistry.h:166-169`) — callable from any thread, hands back a
   reference to an immutable field (`:163`); never falls back across profile tags (`:151-158`).
2. `FCk_GroundNav_PathSearch::Request_Begin(Field, Query)`
   (`GN/Search/CkGroundNav_PathSearch.h:85-88`) — "Answer the query outright, or stand a search up".
   The header states the invariant that makes one-shot legitimate: slicing changes nothing but where
   the work stops, and the one-shot form is the sliced form with no limits (`:26-29`, `:57-61`).
3. `ck::groundnav::Get_PathPlan(Result, Field, PostParams)`
   (`GN/Search/CkGroundNav_PathPostProcess.h:237-240`) — funnel, link endpoints, corner offset,
   skip-first and fill as one pure function; returns `_Waypoints`, `_PlateCorridor`, `_LengthUu`,
   `_PlannedAgainstEpoch` (`:93-100`).

Query inputs are assembled by `Get_Query` (`GN/Path/CkGroundNavPath_Processor.cpp:127-145`) onto
`FCk_GroundNav_PathQuery` (`GN/Search/CkGroundNav_SearchTypes.h:147-170`): `_Start`, `_Goal`,
`_VerticalToleranceUu`, `_Agent._RadiusUu`, `_Cost`, `_GreedyWeightW`, `_MaxExpansions` (`:162`),
`_MaxCorridorLength` (`:165`), `_AllowPartialPath` (`:169`). Terminal statuses:
`ECk_GroundNav_PathStatus {InProgress, Ready, Partial, Unbuilt, NoStartSurface, NoGoalSurface,
Unreachable, BudgetExceeded, Blocked}` (`GN/Search/CkGroundNav_SearchTypes.h:37-70`).

So a synchronous GroundNav path needs **no entity, no fragment, no processor, no registry touch** —
three existing calls over an immutable field snapshot.

### 1.3 Every direct-Recast site in CkPathNetwork (re-verified; only one moved)

All 30 live hits are in `PN`; the 31st is a comment at `CkPathNetwork_Fragment_Data.h:463`.
`CkPathNetworkEditor` still greps zero for the whole pattern.

| `PN` line | Call | What it needs |
|---|---|---|
| `:23,:26,:27` | includes: RecastAdapter, `<NavigationSystem.h>`, `<NavMesh/RecastNavMesh.h>` | deleted when the rows below migrate |
| `:165-167` | comment justifying direct `FindPathSync` under `_MaxRouteQueriesPerFrame` | rewritten by any migration |
| `:182,:183,:186` | `TryGet_NavSystem` / `TryGet_NavData` / `HasValidNavmesh` | provider-availability predicate; drives `EOffPathResolve::NoNavmesh`, upgraded to `PathFailed` at `:1340-1347` |
| `:204,:458,:519,:548,:614,:736,:797` | `Get_CompiledQueryFilter` → `FSharedConstNavQueryFilter` (`:350`,`:370`) | dies with its consumers |
| `:232,:239,:471,:525,:836` | `NavData->ProjectPoint` | **projection** (five sites; not overlay-equivalent to `Try_ProjectPoint`, see §4 FORK-E) |
| `:273,:408` | `NavData->GetConfig().AgentRadius` | corner-offset input; no facade equivalent |
| `:275` | `FCk_Nav_Algorithm::FindPathSync` in `Resolve_OffPathLeg` (`:169-333`, sole caller `:1321`) | **a full path**, synchronous, inside one `DoHandleRequest` pass |
| `:356` | `ARecastNavMesh::NavMeshRaycast` in `Is_NavmeshSegmentDirectlyWalkable` (`:348-364`) | **a raycast / walkability verdict**; callers `:749`, `:871`, `:876` (all clearance/shortcut probes) |
| `:387` | `NavMeshRaycast` in `Try_ResolveNavmeshSegment` (`:367-441`) | **a raycast**; callers `:564`, `:637`, both inside per-segment loops |
| `:410` | `FindPathSync` detour fallback in the same function | **a full path**, N times per route plan |
| `:433-437` | `RaycastResult.HitTime` / `bIsRaycastEndInCorridor` Verbose diag | no neutral equivalent (`FCk_NavSurface_RaycastResult` = `_Status` + `_HitLocation`, `NS/CkNavSurface_Fragment_Data.h:268-284`) |
| `:812,:884` | `NavData->FindDistanceToWall(Point, Filter, MaxRadius, &OutClosestWall)` | **distance to wall AND the closest wall point**, radius-bounded — FORK-2. Live path: `Apply_NavmeshClearance` called `:1756` |

Only delta versus the session-9 inventory: `Resolve_OffPathLeg`'s call site is `:1321`, not `:1323`.
Everything else (`:275`, `:410`, `:348`, `:356`, `:387`, `:749`, `:871`, `:876`, `:812`, `:884`) is exact.

---

## §2 The design — option 1a

### 2.1 Capability 15 — `_FindPathSync`

```
TFunction<FCk_NavSurface_PathResult(UWorld*, const FCk_NavSurface_PathQuery&)> _FindPathSync;
```
Placed after `_IsReachable` (`NS/CkNavSurface_ProviderTable.h:39`) — it is a query, and the table is
grouped query-first. **THREAD CONTRACT comment: GAME THREAD ONLY**, unlike `_BoundarySegments`
(`:32-35`), because Recast's side is a live `ARecastNavMesh` query.

`FCk_NavSurface_PathQuery` (new, in `NS/CkNavSurface_Fragment_Data.h`, modelled on
`FCk_NavSurface_RaycastQuery` `:230-261`, `CK_DEFINE_CONSTRUCTORS(_Start, _End)`):
`_Start`, `_End`, `_QueryFilter` (`FGameplayTag`), `_QueryFilterOverlay`, `_ProfileTag`,
`_AllowPartial` (`ECk_EnableDisable`), `_MaxExpansions` / `_MaxCorridorLength` (`int32`, 0 = unbounded),
`_CornerOffsetDistanceUu`, `_SearchHalfExtents` (the endpoint projection box).

`FCk_NavSurface_PathResult` (new, modelled on `FCk_NavSurface_RaycastResult` `:268-284`):
`_Status`, `TArray<FVector> _Waypoints`, `_StartProjected`, `_EndProjected`, `_IsPartial`, `_LengthUu`.
Status reuse — no new enum: `Success` (Ready, and Partial when asked), `Unbuilt`, `NoSurface` (an end
resolves to nothing on built ground), `Blocked` (body refused / budget exceeded / unreachable),
`NoProvider` (no table, no navmesh, no field).

**Recast implementation** — `ck::nav_surface_recast::Try_FindPathSync(World, Query)` in
`NS/Recast/CkNavSurface_RecastAdapter.cpp`, wrapping the existing
`FCk_Nav_Algorithm::FindPathSync` (`NAV/CkNav_Algorithm.h:24-35`) with
`Get_CompiledQueryFilter(*NavData, _QueryFilter, _QueryFilterOverlay)` (`:315`) and
`TryGet_NavData` (`:373`). This is literally what `PN:275` does today, so the Recast branch is
byte-identical by construction. `_MaxExpansions`/`_MaxCorridorLength` are **ignored on Recast** and
that must be said in the field comment, not silently.

**GroundNav implementation** — `Do_FindPathSync` in `GN/Facade/CkGroundNav_NavSurfaceAdapter.cpp`,
same shape as `Do_SurfaceRaycast` (`:173-199`): `TryGet_Field(World, _Start, _ProfileTag)` → null ⇒
`NoProvider`; `Request_Begin(Field, Query)` (one-shot = unlimited slice) with the two caps threaded;
`Get_PathPlan` for the waypoints; then the existing status map (`:60-80`) extended —
`Ready→Success`, `Partial→Success` when asked (else `Blocked`), `Unbuilt→Unbuilt`,
`NoStartSurface|NoGoalSurface→NoSurface`, `Unreachable|BudgetExceeded|Blocked→Blocked`, and
`InProgress` unreachable for a one-shot (assert it).
Unbuilt/building: the registry answers only from PUBLISHED fields, so "building" surfaces as
`Unbuilt` here and as `ProviderHealth::Building` on capability 7 — the existing split, unchanged.

**Cost bounds.** GroundNav is bounded by `_MaxExpansions` and `_MaxCorridorLength`
(`GN/Search/CkGroundNav_SearchTypes.h:162,165`). No wall-clock budget on the neutral query: Recast's
`FindPathSync` cannot honour one, and a budget one provider ignores is worse than no budget. The
per-frame ceiling stays where it is — PathNetwork's `_MaxRouteQueriesPerFrame` (`PN:165-167`) — so
1a preserves the existing budget semantics exactly, which is its whole advantage over 1b.

### 2.2 Capability 16 — `_FindDistanceToWall` (FORK-2)

**Its own capability, not a mode of `_BoundarySegments`.**
```
TFunction<FCk_NavSurface_WallDistanceResult(UWorld*, const FCk_NavSurface_WallDistanceQuery&)> _FindDistanceToWall;
```
Query: `_Location`, `_MaxRadiusUu`, `_QueryFilter`, `_QueryFilterOverlay`, `_ProfileTag`.
Result: `_Status`, `float _DistanceUu`, `FVector _ClosestWallPoint`, `bool _FoundWall`
(the sentinel test at `PN:817` becomes this bool).
- Recast: `NavData->FindDistanceToWall(Location, CompiledFilter, MaxRadius, &OutClosest)` — the
  call at `PN:812` verbatim.
- GroundNav: `ck::groundnav::Get_ClosestBoundary(Field, Query)`
  (`GN/Query/CkGroundNav_Query_Boundary.h:39-42`) — already returns `_Status`, `_ClosestPoint`,
  `_DistanceUu` bounded by `_MaxRadiusUu` (`GN/Query/CkGroundNav_QueryTypes.h:330-354`). A one-to-one
  fit; no new GroundNav code beyond the adapter shim.

Numbering: 15 `_FindPathSync`, 16 `_FindDistanceToWall`, and [P6-B1]'s two generators become **17–18**
(18 total), not "15–16" as PROGRESS.md:4873 words it. Flagged as FORK-F.

---

## §3 Units for an executor

Units 1 and 2 own the same files and are strictly sequential; 3–6 fan out after 2.

**U1 — the two capabilities, both providers, one atomic change.**
Files: `NS/CkNavSurface_Fragment_Data.h`, `NS/CkNavSurface_ProviderTable.h`, `NS/CkNavSurface_ProviderTable.cpp`
(`Get_IsComplete`), `NS/CkNavSurface_Utils.h/.cpp`, `NS/Recast/CkNavSurface_RecastAdapter.h/.cpp`,
`CkNavigation/CkNavigation_Module.cpp`, `GN/Facade/CkGroundNav_NavSurfaceAdapter.cpp`.
DoD: editor boots with no `NavSurface provider … INCOMPLETE` ensure; both providers answer both new
capabilities; `Ck.GroundNav.*` and `Ck.Nav*` delta-zero.
Pins: new `Plugins/CkTests/Source/CkTests/Private/UnitTests/CkNavigation/Test_NavSurface_PathSync_Contract.cpp`
— mirror `Test_NavSurface_Settled.cpp` for shape and `Test_NavSurface_BoundarySegments_ThreadContract.cpp`
for the contract-assertion idiom. Extend
`UnitTests/CkGroundNav/Test_GroundNav_Facade_Equivalence.cpp` (its 1000-seeded-point pattern,
`kPointCount`/`kSeed` at its head) with a path pair and a wall-distance pair: facade-through-GroundNav
vs the direct `Request_Begin`+`Get_PathPlan` / `Get_ClosestBoundary` call.

**U2 — GroundNav query-filter + cost-cap parity (FORK-5 / FORK-D).**
File: `GN/Facade/CkGroundNav_NavSurfaceAdapter.cpp` only (plus `_MaxCost` on
`NS/CkNavSurface_Fragment_Data.h`'s raycast query if FORK-D is ruled in).
Today `Do_SurfaceRaycast` (`:173-199`) reads neither `Get_QueryFilter()` nor `Get_QueryFilterOverlay()`
and never sets `_MaxCost`. DoD: a filtered query answers differently from an unfiltered one on
GroundNav. **This must land before any verdict-agreement gate, or the gate fails by construction.**
Pin: new case in `Test_GroundNav_Facade_Equivalence.cpp`.

**U3 — PathNetwork safety oracle onto the facade.**
File: `PN` only. `Is_NavmeshSegmentDirectlyWalkable` (`:348-364`) → `Try_SurfaceRaycast`;
`Try_ResolveNavmeshSegment` (`:367-441`) → `Try_SurfaceRaycast` + `Try_FindPathSync` for the `:410`
fallback. Drop the `FRaycastResult` diagnostic (FORK-G). DoD: verdict-agreement run over the authored
corpus with every disagreement adjudicated individually (PHASE_5 §5 gate).
Pins: `Ck.PathNetwork.*` (71 C++) delta-zero on Recast; AS corpus selected by `Ck_AutoTest_PathNetwork`
(16). New agreement pin mirroring
`Plugins/CkTests/Script/CkGroundNav/CkAutoTest_GroundNav_Shadow_InstalledPathIsByteIdenticalToRecast.as`.

**U4 — off-path leg connector.**
File: `PN` only. `Resolve_OffPathLeg` (`:169-333`): `:182-186` → `Get_ProviderHealth`; `:275` →
`Try_FindPathSync`. `EOffPathResolve::NoNavmesh` must keep firing for a provider that cannot answer —
`:1340-1347` upgrades it to `PathFailed` for a strict gap, so a remap changes routing.
`:273` `AgentRadius` → `_CornerOffsetDistanceUu` supplied by the caller.
DoD: PathNetwork suite delta-zero on Recast, green on GroundNav; off-path resolution within the
A7(b)-measured budget. Pin:
`Script/CkCrowd/CkAutoTest_Crowd_PathNetworkStationaryDetour.as` unchanged and green.

**U5 — clearance + projection sites.**
File: `PN` only. `:812`/`:884` → `Try_FindDistanceToWall`; `:232,:239,:471,:525,:836` →
`Try_ProjectPoint` **only after FORK-E is ruled**. DoD:
`CkAutoTest_PathNetworkFollower_DesiredNavmeshClearanceMovesInward.as` green unchanged.

**U6 — dependency and comment sweep.**
Files: `PN` (`:23,:26,:27`), `CkPathNetwork/CkPathNetwork.Build.cs` (drop `"NavigationSystem"`, `:19`),
`CkPathNetwork_Fragment_Data.h:463` (the "agent-radius-eroded Recast boundary" comment),
`GN/CLAUDE.md` + `MIGRATION_SEAM.md`. DoD: `rg 'Recast|NavigationSystem|FindPathSync|NavMeshRaycast'
Source/CkPathNetwork` → zero.

---

## §4 Forks the orchestrator must rule

- **FORK-A — result type.** *Rec: a new `FCk_NavSurface_PathResult`, not `FCk_Nav_PathResult`.*
  The blocker text (PROGRESS.md:4894) names the latter. Failure mode of reusing it: it carries
  `_RequestRevision`, `_PendingSinceSeconds` and `FCk_Nav_PathDiagnostics`
  (`NAV/CkNav_Fragment_Data.h:139-153`) — episode state a pure query never sets, one of which is
  documented as MUST NOT be persisted or replicated (`:149-152`). Consumers would read fields that
  are always zero and cannot tell that from a real answer.
- **FORK-B — wall distance: capability or `_BoundarySegments` mode?** *Rec: a 16th capability.*
  Failure mode of the mode: `_BoundarySegments` returns runs (`NS/CkNavSurface_Fragment_Data.h:325-368`),
  so the consumer re-implements `Get_ClosestBoundary`'s ring search itself and cannot express Recast's
  radius-bounded early-out — different numbers at `PN:812/:884`, i.e. every clearance waypoint shifts
  silently and `…DesiredNavmeshClearanceMovesInward` becomes a tolerance argument.
- **FORK-C — budget shape.** *Rec: `_MaxExpansions` + `_MaxCorridorLength`, no wall-clock.*
  Failure mode of a time budget: Recast cannot honour it, so the same query is bounded on one provider
  and unbounded on the other — a provider-dependent verdict, which is the thing the facade exists to
  prevent.
- **FORK-D — `_MaxCost` on `FCk_NavSurface_RaycastQuery`** (carried from FORK-3).
  *Rec: add the field (a query-struct extension, not a capability).* Failure mode without it:
  `FEATURE_MATRIX.md:1822` prescribes the oracle raycast "with the cost cap"; migrating uncapped
  admits segments the cost model was authored to refuse, and the disagreement surfaces as an
  unadjudicable verdict at the 5C gate.
- **FORK-E — `_QueryFilterOverlay` on `FCk_NavSurface_ProjectionQuery`** (carried from FORK-4).
  *Rec: add it, matching raycast (`:236`) and move-along.* Failure mode of the "prove no caller sets
  one" licence: it is a licence with no enforcement point, and the first consumer that sets an overlay
  gets a silently different projection.
- **FORK-F — numbering vs [P6-B1].** *Rec: 15 path, 16 wall, 17–18 generators.* PROGRESS.md:4873 says
  the generators are "entries 15–16", which collides with this ruling; ruling both together without
  fixing the numbering leaves two capabilities claiming slot 15.
- **FORK-G — the raycast diagnostic** (carried from FORK-6). *Rec: drop the `HitTime` /
  `bIsRaycastEndInCorridor` half of the Verbose line at `PN:430-439`.* Failure mode of extending
  `FCk_NavSurface_RaycastResult`: a Recast-shaped field GroundNav can only fill by inventing a value.
- **FORK-H — thread contract.** *Rec: GAME THREAD ONLY, stated in the table comment beside
  `_BoundarySegments`'s opposite contract.* Failure mode of silence: `_BoundarySegments` establishes
  the precedent that facade queries are worker-callable, and the crowd's avoidance sampler already
  runs on a worker.

---

## §5 Risks and what I could not verify

1. **ASSUMED, load-bearing:** `ARecastNavMesh::NavMeshRaycast` (static, `PN:356/:387`) and the virtual
   `Raycast` the facade uses (`NS/Recast/CkNavSurface_RecastAdapter.cpp:480-506` per session-9 read)
   produce identical hit verdicts. Confirming it needs engine source, which is out of scope by
   standing instruction. It is a 5C **entry** check, not a given.
2. **Not measured:** GroundNav's one-shot cost at `PN:564`/`:637`, which are inside per-segment loops —
   N synchronous searches per route plan. The A7(b) budget was measured on Recast only. If GroundNav's
   per-call cost is materially higher, U3 lands within the facade but outside the budget, and the fix
   is a tighter `_MaxExpansions`, not a wider tolerance.
3. **Unverified:** whether any shipped PathNetwork caller supplies a non-empty
   `FCk_Nav_QueryFilterOverlay`. FORK-E's alternative depends entirely on this.
4. **Unverified (session-9 finding 13, still open):** whether
   `UnitTests/CkSnapshot/Test_Snapshot_WallTimeAllowList_MetaTest.cpp` covers CkPathNetwork. Relevant
   only if U3/U4 add a timing read.
5. **Unverified:** the Recast API backing [P6-B1]'s generators (entries 17–18). I did not confirm a
   Recast equivalent for `Get_RandomPointsByPathDistance`
   (`GN/Query/CkGroundNav_Query_Points.h:36-38`); if none exists, entry 18 is GroundNav-only and the
   completeness gate (§1.1) forbids it as a table entry. **Rule 15–16 now; do not fold 17–18 in until
   the Recast side is confirmed.**
6. `PN` is not on the wall-time allow-list and must stay off it (session-9 ruling, [NN-D72] item 7).
7. This design touches nothing [P5-B2] depends on; 5D row 1 stays blocked on its own ruling.
