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

## Implementation notes

### Jolt axis convention

Jolt's `CapsuleShape`/`CylinderShape` are **Y-axis aligned**; we run Jolt in Unreal's Z-up frame and `jolt::Conv` is an axis passthrough. Both the probe factories (`CkProbe_Processor.cpp`) and the shape-trace path (`CkProbeTrace_Utils.cpp`) therefore stand the leaf shape up inside a `JPH::RotatedTranslatedShape` via `jolt::Get_ShapeAxisCorrection_YToZ`. Two consequences:

- The four `TProbeShapeFactory` specializations return `JPH::ShapeSettings::ShapeResult` rather than a concrete settings struct — with the wrapper in play the resulting shape type differs per fragment, and `ShapeResult` is the one type all four share.
- `Shape::GetTriangles*` is a LEAF-only API and asserts on decorated shapes, so the debug-draw path unwraps to the inner shape and folds the wrapper's rotation into the draw rotation. The wrapper's translation is always zero and the leaf's centre of mass is the origin, so the draw position is unaffected.

### CkJolt split residue

- `CkSpatialQuery_Utils.h` re-exports `CkJolt/CkJolt_Utils.h` purely for API stability — the generic UE↔Jolt conversion layer (Conv overloads, axis correction, body-UserData resolvers) moved to CkJolt with the world-ownership split, and `Get_ProbeBodyUserData` is now a thin wrapper over `ck::jolt::Get_BodyUserData`. Consumers reaching `ck::jolt::*` through this header keep compiling.
- `ECk_MotionType` / `ECk_MotionQuality` / `ECk_BackFaceMode` migrated from `CkProbe_Fragment_Data.h` to `CkJolt/CkJolt_Common.h` (generic Jolt vocabulary); CoreRedirects cover serialized BP references.
- Probes live on CkJolt's dedicated probe layer from the signature-driven layer table, which pairs them with dynamic-domain WorldDynamic bodies (i.e. other probes) and never with the static world — exactly the pre-table `ObjectLayer{1}` behaviour.

### Bugs the current shape encodes — do not "simplify" these back

- **Trace filter direction.** `Request_Multi*Trace` matches `ProbeName.MatchesAny(Filter)`, mirroring `Get_CanOverlapWith`. The earlier `Filter.HasTag(ProbeName)` expanded the FILTER's parents, so a probe with the default root `Probe` name matched *any* filter and large trigger volumes stole Single-policy traces from real targets.
- **Body-slot leak on probe teardown.** `FProcessor_Probe_EndPlay` must call `DestroyBody` unconditionally; `RemoveBody` only detaches from the broadphase. The old code gated `DestroyBody` on `IsAdded` *after* `RemoveBody` (and skipped LinearCast probes, whose bodies are created in Setup and never added), making the destroy unreachable on every path — each probe leaked a body slot until `MaxBodies` exhausted under churn.
- **Per-direction overlap gating.** `ContactCastCollector` gates each direction of a mutual overlap on the receiver's own Silent policy / context / tag filter, matching `CkContactListener`. Without it a fast-moving probe fires overlap events into probes its filter excludes.

---

## See also

- `CkShapes/Claude.md` — shape data.
- `CkPerception/Claude.md` — AI perception (for perceived stimuli vs. raw overlap).
- `CkOverlapBody/Claude.md` — signal-based overlap (vs. query-based).
