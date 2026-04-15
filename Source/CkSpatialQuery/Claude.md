# CkSpatialQuery

**Purpose:** Volume-based spatial queries — finds entities within a shape (box, sphere, capsule) each tick and writes results to a fragment. Uses JoltPhysics or UE's physics query system.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkPhysics`, `CkProvider`, `CkRecord`, `CkSettings`, `CkShapes`, `CkThirdParty`.
**Used by:** `CkAggro`, ability AoE detection, proximity checks.

---

## Key API

- Add a spatial query entity with shape params; the query processor runs the overlap test each tick and writes hit entity handles into `FFragment_SpatialQuery_Results`.

---

## Pattern

Spatial query entity → shape params → processor queries → results fragment → downstream processors read results.

---

## Anti-patterns

Don't use `UKismetSystemLibrary::SphereOverlapActors` inside a Processor — use `CkSpatialQuery` so queries are batched and results are fragment-accessible.

---

## See also

- `CkShapes/Claude.md` — shape data.
- `CkPerception/Claude.md` — AI perception (for perceived stimuli vs. raw overlap).
- `CkOverlapBody/Claude.md` — signal-based overlap (vs. query-based).
