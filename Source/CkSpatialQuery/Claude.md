# CkSpatialQuery

**Purpose:** Volume-based spatial queries — finds entities within a shape (box, sphere, capsule) each tick and writes results to a fragment. Backed by the Jolt world owned by `CkJolt`; the Probe feature creates kinematic Jolt sensor bodies as its implementation detail.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkJolt`, `CkLabel`, `CkLog`, `CkPhysics`, `CkProvider`, `CkRecord`, `CkSettings`, `CkShapes`, `CkThirdParty`.
**Used by:** `CkCrowd` (neighbor overlaps), `CkEqs` (trace overloads), `CkProjectile` (LinearCast impacts), `CkEcsDebugger` (editor inspectors).

**Jolt-world split (2026-07-16):** the `JPH::PhysicsSystem`, JobSystem, listeners, debug renderer, and per-tick update moved to `CkJolt` (`UCk_Jolt_Subsystem`). `UCk_SpatialQuery_Subsystem` remains as a non-tickable bridge that translates CkJolt's drained contact events into Probe overlap requests and gates CkJolt's debug draw on this module's user settings. Probe processors obtain the world via the `TWeakPtr<JPH::PhysicsSystem>` registry context, unchanged.

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
