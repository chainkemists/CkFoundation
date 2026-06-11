# CkProjectile

**Purpose:** Projectile entities — ballistic/physics-driven projectiles with velocity, range, hit detection, and penetration. Built on `CkPhysics` for movement and `CkOverlapBody` (optionally) for hit detection. Also home to the **deterministic ballistics** layer (`Ballistic/`, `BallisticMotion/`) that `CkLagCompensation` builds on.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLog`, `CkPhysics`, `CkRecord`, `CkShapes`, `CkSpatialQuery`, `CkVariables`.
**Used by:** Ranged combat, thrown objects, `CkLagCompensation`.

---

## Key API

- `UCk_Utils_Projectile_UE::Add(InHandle, InParams)` — spawn a projectile entity (Euler-integrated path).
- Signals: `OnHit`, `OnRangeExpired`.

### Deterministic ballistics (`Ballistic/`)
- `ck::ballistics` — closed-form Carpentier linear-drag math: `Get_PositionAtTime` / `Get_VelocityAtTime` / `Get_TimeOfFlightTo`. Pure functions of (initial conditions, params, time) — identical on every machine at any tick rate.
- `UCk_Utils_Ballistic_UE` — BP/AS wrappers + `Get_SampledTrajectory` for prediction visuals.

### BallisticMotion (`BallisticMotion/`)
- `UCk_Utils_BallisticMotion_UE::Add(InHandle, InParams)` — closed-form flight feature; requires a Transform. Compose a CkShapes shape + LinearCast Probe for hit detection (probe `ContextOverlapPolicy` gives ignore-shooter).
- `Request_Launch` (supports `OverrideTime` to anchor the trajectory in the past — the lag-comp catch-up), `Request_Stop`.
- Signals: `OnImpact` (analytic impact time/point/velocity), `OnTrajectoryChanged` (per segment, e.g. bounce), `OnStopped`.
- Impact response: `Stop` or `Bounce` (restitution; re-anchors a new trajectory segment at the analytic impact point, so late-observed contacts are corrected exactly).

---

## Pattern

Projectile entity has velocity/range fragments; `FProcessor_Projectile_Move` advances position; hit detection fires `OnHit` signal to the instigator entity.

BallisticMotion instead computes its pose from world time each frame (`FProcessor_BallisticMotion_UpdateTrajectory`); probe overlaps feed `FProcessor_BallisticMotion_HandleImpacts`, which responds analytically on the trajectory.

---

## Anti-patterns

Don't use UE's `AProjectile` actor for ECS-driven projectiles — the overhead of a full actor per projectile defeats the purpose.

---

## See also

- `CkPhysics/Claude.md` — acceleration/movement.
- `CkOverlapBody/Claude.md` — collision detection.
