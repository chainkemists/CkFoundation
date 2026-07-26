# CkProjectile

**Purpose:** Projectile entities — ballistic/physics-driven projectiles with velocity, range, hit detection, and penetration. Built on `CkPhysics` for movement and `CkOverlapBody` (optionally) for hit detection. Also home to the **deterministic ballistics** layer (`Ballistic/`, `BallisticMotion/`) that `CkLagCompensation` builds on, and the **proportional-navigation homing** feature (`Homing/`).

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLog`, `CkPhysics`, `CkRecord`, `CkShapes`, `CkSpatialQuery`, `CkVariables`.
**Used by:** Ranged combat, thrown objects, `CkLagCompensation`.

---

## Key API

- `UCk_Utils_Projectile_UE::Add(InHandle, InParams)` — spawn a projectile entity (Euler-integrated path).
- Signals: `OnHit`, `OnRangeExpired`.

### Deterministic ballistics (`Ballistic/`)
- `ck::ballistics` — closed-form Carpentier linear-drag math: `Get_PositionAtTime` / `Get_VelocityAtTime` / `Get_TimeOfFlightTo`. Pure functions of (initial conditions, params, time) — identical on every machine at any tick rate.
- `UCk_Utils_Ballistic_UE` — BP/AS wrappers + `Get_SampledTrajectory` for prediction visuals.

### Homing (`Homing/`)
- `ck::pronav` — stateless Proportional Navigation math: `Compute_GuidanceState` (analytic LOS rotation `Ω = (R × Vrel)/|R|²` — no finite differences, framerate-independent), `Compute_TrueProNav_Acceleration` / `Compute_PureProNav_Acceleration`, `Compute_HomingAcceleration` (full law: speed-control mode, gravity compensation, leftover thrust, desired time-to-impact, receding recovery), `Compute_FiringSolution` (constant-velocity intercept incl. moving shooter, A≈0 linear root, two-roots preference). Unit-tested headlessly in `CkTests/.../UnitTests/CkProjectile/Test_CkHoming_*`.
- `UCk_Utils_Homing_UE::Add(InHandle, InParams)` — requires Velocity + Acceleration + Transform on the entity (i.e. add the Projectile feature first). Speed limits come from the Velocity feature's MinMax; rotation from AutoReorient.
- Requests: `Request_SetTargetEntity` (optional point-on-target local offset), `Request_SetTargetLocation` (fixed world point), `Request_ClearTarget`, `Request_SetDesiredTimeToImpact` (countdown-based), `Request_EnableDisable`.
- Signals: `OnTargetMissed` (closing→receding flip inside the miss-notify threshold — proximity detonation hook), `OnTargetLost` (target entity destroyed; homing deactivates).
- `FProcessor_Homing_Update` runs in `FGroup_Physics`, `RunBefore` the EulerIntegrator (same-frame steering) and only on `FTag_HasAuthority` entities — clients see results via Velocity replication.
- **Acceleration channel ownership:** while active, homing writes `Acceleration_Current = base (captured at Add) + guidance` every frame and restores the base on clear/disable/target-lost. Other acceleration modifiers on the same entity will be stomped while homing is active.

### BallisticMotion (`BallisticMotion/`)
- `UCk_Utils_BallisticMotion_UE::Add(InHandle, InParams)` — closed-form flight feature; requires a Transform. Compose a CkShapes shape + LinearCast Probe for hit detection (probe `ContextOverlapPolicy` gives ignore-shooter).
- `Request_Launch` (supports `OverrideTime` to anchor the trajectory in the past — the lag-comp catch-up), `Request_Stop`.
- Signals: `OnImpact` (analytic impact time/point/velocity), `OnTrajectoryChanged` (per segment, e.g. bounce), `OnStopped`.
- Impact response: `Stop` or `Bounce` (restitution; re-anchors a new trajectory segment at the analytic impact point, so late-observed contacts are corrected exactly).
- **Why the analytic re-anchor is not optional:** probe contacts surface one frame *after* Jolt detects them, so `FProcessor_BallisticMotion_HandleImpacts` never sees the contact on the frame it happened. Responding at the analytic impact point (rather than at the observed pose) makes the correction exact regardless of how late the contact arrived.
- `FProcessor_BallisticMotion_UpdateTrajectory` writes the closed-form pose for the current world time — a function of (initial conditions, time) only, never of last frame's pose. That is what makes the path identical on every machine at any tick rate.

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
