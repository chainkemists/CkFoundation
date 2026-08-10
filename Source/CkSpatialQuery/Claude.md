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

### ProbeTrace world-hit policy

A ProbeTrace can report non-probe Jolt bodies — baked static world, JoltBodies — alongside the
probes it matches. It is **opt-in per call** through the settings struct, and the default is
bit-exact with the probe-only behaviour that predates it.

| `_WorldHitPolicy` | Meaning |
|---|---|
| `Ignore` (default) | Non-probe bodies are invisible. Every pre-existing caller relies on this. |
| `Blocking` | The nearest passing world hit truncates: probes beyond it are neither returned nor overlap-fired, and the world hit is the final element. The gun trace. |
| `Reported` | World hits interleave with probes in fraction order; nothing is truncated. The melee swing that sparks on the wall AND still cuts the enemy behind it. |

The default is load-bearing rather than polite: EQS line-of-sight reads `Hits.IsEmpty()` as
"clear", the EQS crowd test counts hits, and the claw machine guards on `Get_Probe()` validity. If
world hits ever became the default, all three invert silently.

Result contract, on both `FCk_Probe_RayCast_Result` and `FCk_ShapeCast_Result`:

- `_HitKind` discriminates Probe from World. `_Probe` is populated for **Probe hits only** — a World
  hit deliberately carries an INVALID probe handle, which is what keeps the claw machine's
  `Is_NOT_Valid(Get_Probe())` guard rejecting walls.
- `_HitEntity` is the generic attribution: the probe entity for Probe hits, the `JoltStaticActor`
  (or JoltBody) entity for World hits. It MAY be invalid — a body with no owning entity is still a
  real physical hit.
- `_SurfaceNormal` is the true surface normal. `_NormalDirLen` is NOT (see below).
- `Request_Single*` is `Multi[0]`, so under `Blocking` it answers "did the shot land or hit the
  wall" and nothing more. "Was there a probe behind the wall?" needs `Multi` or `Reported`.

Two supporting knobs, both on the transient settings only:

- `_OverlapNotifyPolicy` (`ECk_ProbeResponse_Policy`, default `Notify`) gates whether surviving
  PROBE hits ping Begin/EndOverlap into the probes they hit. This exists because the game's weapon
  aim sweep (`bb_aim::Sweep`) could not use this API at all: the script-facing overloads hard-wired
  `FireOverlaps = true`, so an aim PREVIEW landed as a real hit. World hits never fire overlaps
  under any policy. The persistent settings deliberately lack this field — the persistent processor
  does its own begin/updated/end bookkeeping and never fires through that path.
- `_IgnoredEntities` drops hits whose resolved entity is listed, before ordering and blocking. It is
  the caller-controlled tool for own-collision-pill exclusion; self-skip covers only the tracing
  entity itself and is deliberately NOT extended to an ancestor chain.

Persistent traces get `_WorldHitPolicy` / `_WorldFilter` / `_IgnoredEntities` and a new
`OnProbeTraceWorldHit` signal. It is begin-equivalent and deduped per contact episode (bodies with
no entity share one anonymous slot); there is no end signal.

**The trace context (`Probe/CkProbeTrace_Context.h`).** A trace needs two things out of the Jolt
world — the `JPH::PhysicsSystem` and the collision layer table — and both are published as registry
contexts by `UCk_Jolt_Subsystem`. `FCk_ProbeTrace_Context::Get_ForEntity(Handle)` bundles them, so
the internal C++ trace overloads take ONE opaque parameter and callers never name a Jolt type. It is
a plain struct (not a USTRUCT), its two members are private with `UCk_Utils_ProbeTrace_UE` as the
only friend, and it is cheap enough to build per tick — never cache one across worlds, because the
weak physics pointer is the liveness gate. An invalid context is a legitimate state (no Jolt
subsystem outside Game/PIE worlds), so `Get_ForEntity` never ensures; the trace entry points do.
This is what lets **CkEqs depend on CkSpatialQuery WITHOUT depending on CkJolt** — EQS issues no Jolt
queries and owns no bodies, and its Build.cs deliberately does not list `CkJolt`. If a change to this
module puts a Jolt type back into the trace signatures, that dependency edge comes back with it.

Implementation shape worth knowing before touching it: world-ness is decided **per hit inside the
collector**, never by handing the channel filter to the cast. Passing it as the cast's
`ObjectLayerFilter` (the `CkJoltQuery_Utils` pattern) would channel-filter PROBES too and break
every probe-only trace whose probes do not answer the channel. The collectors already received
every body and discarded the non-probes, so the narrow-phase cost was already being paid.

### Bugs the current shape encodes — do not "simplify" these back

- **Trace filter direction.** `Request_Multi*Trace` matches `ProbeName.MatchesAny(Filter)`, mirroring `Get_CanOverlapWith`. The earlier `Filter.HasTag(ProbeName)` expanded the FILTER's parents, so a probe with the default root `Probe` name matched *any* filter and large trigger volumes stole Single-policy traces from real targets.
- **Body-slot leak on probe teardown.** `FProcessor_Probe_EndPlay` must call `DestroyBody` unconditionally; `RemoveBody` only detaches from the broadphase. The old code gated `DestroyBody` on `IsAdded` *after* `RemoveBody` (and skipped LinearCast probes, whose bodies are created in Setup and never added), making the destroy unreachable on every path — each probe leaked a body slot until `MaxBodies` exhausted under churn.
- **Per-direction overlap gating.** `ContactCastCollector` gates each direction of a mutual overlap on the receiver's own Silent policy / context / tag filter, matching `CkContactListener`. Without it a fast-moving probe fires overlap events into probes its filter excludes.
- **Empty-filter traces have never fired overlaps.** `Request_Multi*Trace` returns ALL probe hits and returns EARLY when `_Filter` is empty — before the overlap block. That is arguably a bug, but every empty-filter caller (the save/load probe fixture, BB's persistent player traces) has always had it, so "fixing" it silently starts firing overlaps into probes those callers never touched. The world-augmented path preserves the same early return.
- **`_NormalDirLen` is not a normal.** It is `StartPos - HitLocation`, a reverse ray VECTOR, and consumers `GetSafeNormal()` it as a direction (`CkProbeTrace_Processor.cpp` feeds it straight into the overlap payload's contact normal). Normalising it in place, or repurposing the field, silently changes every consumer. The true normal lives in `_SurfaceNormal`, added beside it.
- **Body UserData 0 is never resolved to an entity.** `jolt::TryGet_EntityFromBody` has no zero guard and raw entity id 0 is the registry's transient root, so resolving it hands back a live, unrelated handle. The trace collector therefore reads the raw UserData FIRST and branches three ways — `0` is an anonymous world body, a value equal to the tracing entity's own id is a self-hit and is dropped, and only the remaining case is resolved. Collapsing that back into one `TryGet_EntityFromBody` call turns a self-hit into a phantom wall.

---

## See also

- `CkShapes/Claude.md` — shape data.
- `CkPerception/Claude.md` — AI perception (for perceived stimuli vs. raw overlap).
- `CkOverlapBody/Claude.md` — signal-based overlap (vs. query-based).
