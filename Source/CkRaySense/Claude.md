# CkRaySense

**Purpose:** Raycast sensing — ECS-managed raycasts fired from an entity each tick, with results stored in a fragment and exposed via signals. Use for line-of-sight checks, ground detection, and range sensing.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`, `CkShapes`.
**Used by:** AI sight, ledge detection, jump prediction.

---

## Key API

- `UCk_Utils_RaySense_UE::Add(InHandle, InParams)` — attach a ray sense to an entity.
- `FFragment_RaySense_Current` — last hit result (actor, component, distance, normal).

---

## Pattern

RaySense entity fires a raycast in `ForEachEntity` each tick and writes results to its fragment. Downstream processors read the fragment for decision-making.

---

## Processor scheduling

All five trace/sweep Update processors sit in `FGroup_PostTransform` and share `MarkedDirtyBy =
FTag_Transform_Updated` with the `CkIsmRenderer` IsmProxy PostTransform processors. Each therefore
declares `RunAfter` the IsmProxy pair (`FProcessor_IsmProxy_TransformInstance`,
`FProcessor_IsmProxy_EnsureStaticNotMoved_DEBUG`) so traces read instance transforms after they are
pushed — and so the scheduler's dirty-marker-conflict advisory stays quiet.

The Update processors also declare `PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump`: they broadcast
`UUtils_Signal_OnRaySenseTraceHit` on hit, and `FTag_Transform_Updated` is sticky here, so a pump
would re-trace and re-broadcast the same hit within one frame.

The Updates are chained (LineTrace → BoxSweep → SphereSweep → CapsuleSweep → CylinderSweep), and
`FProcessor_RaySense_HandleRequests` declares `RunAfter` the chain tail (Cylinder) only — that
transitively orders it after all five, so an Enable/Disable request issued this frame never races
the same frame's traces.

The line-trace path routes its hit through the same `Request_ProcessTraceHit` tail as the sweeps, so
it honours the configured `CollisionResponse`. (It previously did not: `Collide` worked only for
sweeps and the params field was silently ignored on the line-trace path.)

## Anti-patterns

Don't call `World->LineTraceSingleByChannel` directly inside processors — route through RaySense so results are cached and deduplicated per entity per tick.

---

## See also

- `CkSpatialQuery/Claude.md` — volume-based spatial queries.
- `CkPerception/Claude.md` — AI perception (higher-level).
