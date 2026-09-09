# navigation-next — Migration seam design [NN-D8]

> **Status:** DECIDED (planning session 1, 2026-08-31). Executors implement; forks return to
> the orchestrator. Line references are a snapshot @ CkFoundation `a25ec9539`; verify with
> `research/R3-coupling-inventory.md` before executing.

## Ground rules

- The **request/result/revision/cancellation contract is already provider-neutral** and stays
  byte-compatible: `FCk_Request_Nav_FindPath` / `FCk_Nav_PathResult` / `MarkPathPending` /
  `InstallExternalPath` / `AbandonPath` / `FailPath` + the revision ring. VoxelNav and
  PathNetwork already ship paths through this seam; the new provider does the same.
- Recast survives the whole migration as **adapter, A/B oracle, fallback, and rollback path**.
  No flag-day. Every phase leaves the Recast path selectable and green.
- CkCrowd's episode lifecycle (advance revision → abandon previous provider → mark shared slot
  pending → dispatch exactly one provider → install only a fresh result → release terminal
  episodes) is preserved verbatim; the new provider becomes a fourth branch in the existing
  provider fork (`RequestPathForActiveGoal`).

## Step 1 — Neutralize the two public leaks (no behavior change)

The only Unreal-Navigation types in the public contract (R3, bucket [ADAPTER]):

1. `FCk_Nav_QueryFilterOverlay::_ExcludedAreaClasses : TArray<TSubclassOf<UNavArea>>`
   → becomes `TArray<FGameplayTag>` (area tags). The Recast adapter owns a tag→`UNavArea`
   class map (seeded from the existing framework area classes:
   `UCk_NavArea_Restricted`, `UCk_NavArea_CrowdAgent`, the three avoidance-volume areas).
2. `FCk_Request_Nav_FindPath::_QueryFilterClassOverride : TSubclassOf<UNavigationQueryFilter>`
   → becomes `FGameplayTag _QueryFilterOverride` resolved through the same settings table the
   tag path already uses. The settings table
   (`UCk_Nav_ProjectSettings_UE::_QueryFilters`) becomes tag → **provider-neutral filter
   definition** (a data asset naming: allowed/excluded area tags, per-area cost multipliers);
   each provider adapter compiles the definition into its native form (Recast: a
   `UNavigationQueryFilter` instance; CkGroundNav: a plate-policy mask + cost table).

Per CkFoundation doctrine there are **no back-compat shims** — call sites are updated in the
same change. CkCrowd's strict/permissive filter classes become two filter-definition assets.
AngelScript test fixtures that punch holes with `UNavArea_Null` migrate to the neutral markup
request (Step 3) in the same phase, keeping every crowd test's obstacle vocabulary working.

## Step 2 — The provider-neutral ground-query facade

The twelve engine-supplied capabilities beyond FindPath (R3 §C) become one neutral surface,
ECS-idiomatic (utils facade + fragments, not a UObject interface):

`UCk_Utils_NavSurface_UE` (working name), backed per world/provider:
- `Try_ProjectPoint(world-pos, extents)` — projection (single + batch).
- `Try_MoveAlongSurface(from, to)` — constrained walk.
- `Try_SurfaceRaycast(from, to)` — walkability LOS with hit point.
- `Get_BoundarySegments(center, radius)` — wall segments; **documented thread contract:
  callable off the game thread against an immutable field snapshot** (Recast adapter keeps the
  existing stack-local-query discipline).
- `Get_IsReachable(from, to, agent)` — component-id reachability.
- `Request_AreaMarkup(bounds/shape, area-tag, enable)` — dynamic cost/exclusion painting
  (replaces `INavRelevantInterface` markup for consumers).
- `Get_IsMarkupLive(markup-handle)` — the "did my paint actually land" ground-truth probe
  (replaces `GetPolyAreaID` polling).
- `Get_SurfaceRevision(world)` / `BindTo_OnSurfaceRebuilt(bounds)` — rebuild observability
  (consolidates the two duplicate revision subsystems — CkNavigation's and CkQueue's — into
  one; [NN-D8a]).
- `Get_SurfaceBounds(world)` — the built extent, for debugger and status readouts.
- `Get_ProviderHealth(world)` — provider readiness (`Ready` / `Building` / `NoData` / `Error`).
- `Get_IsBuildInProgress(world)` — build-state gate.
- `Request_SurfaceRebuild_ForTesting(world)` — the deterministic test hook, kept as a neutral facade
  call per [NN-D8d]; both providers need it.

Consumers migrate mechanically per the R3 dependency table: CkCrowd (ConstrainToNavmesh,
AvoidanceSample, Steering, OnPath/OnRouteResolved, BlockDetect, PathRefresh, AvoidanceVolume),
CkPathNetwork (off-path legs, segment safety oracle, editor snap), CkEqs, CkQueue,
CkGameplayDebugger (CrowdDebugger status/bounds, PerfLab seeding). Sites bucketed [RETIRE]
(DiagNavClip, DrawNavProjection, debug-settings probe) are deleted, not migrated.

**Multi-PIE fix folded in ([NN-D8b]):** the deferred-request queue
(`GDeferredNavRequests`, explicitly not world-keyed today) becomes per-world state when the
drain moves behind the facade. The 5s force-fail watchdog and latest-wins revision semantics
are preserved.

## Step 3 — Provider selection and coexistence

- Provider choice is a per-world/project setting plus the existing per-agent fork: CkCrowd's
  `RequestPathForActiveGoal` gains a GroundNav branch beside VoxelNav → PathNetwork →
  CkNavigation(Recast). Selection order stays data-driven; default unchanged until promotion.
- **Shadow/A-B mode:** a setting dispatches the *same* request to both Recast and CkGroundNav;
  the Recast result installs (authoritative), the shadow result is compared — waypoint count,
  length delta, endpoint delta, success/failure agreement, query time — and divergences are
  logged + counted into a value-only diagnostics fragment the debugger can render. Same-fixture
  A/B parity is the promotion evidence, gathered on the existing crowd/queue/path-network test
  maps and gyms.
- **Fallback:** any CkGroundNav hard failure (no field, unbuilt region) fails the episode
  exactly as Recast failures do today — same fail reasons, same signals — so CkCrowd's retry
  and escape machinery is provider-blind.

## Step 4 — Promotion and retirement gates (measured, not vibes)

**Promotion to default provider requires all of:**
1. Shadow-mode parity on the full crowd/queue/pathnetwork suites: success/failure agreement on
   every test, path-length delta within a per-map budget, zero containment escapes.
2. The `UNavArea_Null`-derived fixture vocabulary fully served by neutral markup (every crowd
   obstacle test green on CkGroundNav).
3. Dynamic gates: markup-live latency and moved-obstacle repair latency ≤ the Recast-measured
   baseline on the same fixtures.
4. Performance: query-per-frame budget adherence and bake/repair cost within budgets recorded
   in VALIDATION.md (numbers measured, not estimated).
5. Both debugger surfaces (runtime in-world draw + CkCrowdDebugger adapter) at parity per
   VALIDATION.md's checklist, in PIE **and** packaged Development/Test.
6. Multi-PIE and teardown clean (no cross-world leaks, no ensures on world death).

**Recast retirement requires additionally:** PathNetwork fully migrated off its safety oracle,
editor authoring snap migrated, all [ADAPTER] sites deleted, `NavigationSystem`/`AIModule`
removed from the affected Build.cs files, `CkNavmeshDebugDraw` deleted (its replacement is the
CkGroundNav draw module), and a full-suite delta-zero gate on the final artifact. Retirement is
its own phase with its own CTO sign-off; rollback before that point is a one-setting revert.

## Rulings on R3's ambiguous items

- [NN-D8a] Duplicate revision subsystems → consolidated into the facade's revision API.
- [NN-D8b] Deferred queue → per-world, kept (both providers build asynchronously).
- [NN-D8c] `Request_SetActorNavigationRegistered` → kept through migration (Recast adapter
  needs it); retired with the adapter iff CkGroundNav's geometry collection is
  physics-backend-sourced (it is — CkJolt backend seam), unless a consumer surfaces.
- [NN-D8d] `Request_NavigationRebuild_ForTesting` → kept as a neutral facade call
  (`Request_SurfaceRebuild_ForTesting`); both providers need a deterministic test hook.
- [NN-D8e] Projection half-extents → provider-neutral setting (both candidates express a
  search box).
- [NN-D8f] `CkNavmeshDebugDraw` → retired at Recast retirement, NOT rewritten; CkGroundNav
  ships its own draw module (runtime tier) designed against value snapshots.
- [NN-D8g] EQS inline projection duplication → migrate both sites to the facade; re-measure;
  the "keep the two in sync" note dies with the duplication unless measurement vetoes.
- [NN-D8h] `CkCrowd.Build.cs:21` stale comment → corrected in Step 2's mechanical migration.
