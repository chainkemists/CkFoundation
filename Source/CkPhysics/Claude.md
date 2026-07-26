# CkPhysics

**Purpose:** Physics modifiers on entities — acceleration modifiers that drive entity velocity through Chaos physics or UE's movement component. Supports bulk and individual modifier entities.

**Depends on:** `CkActor`, `CkChaos`, `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkRecord`.
**Used by:** `CkProjectile`, `CkSpatialQuery`, `CkOverlapBody`.

---

## Key API

- `UCk_Utils_Acceleration_UE` — add acceleration modifier entities.
- `FProcessor_Acceleration_Setup`, `FProcessor_BulkAccelerationModifier_*` — set up bulk modifiers.
- `FProcessor_AccelerationModifier_EndPlay` — cleanup.

---

## Pattern

Acceleration modifier entity → applies force to owner entity's physics body → `CkChaos` or movement component processes it.

---

## Persistence / replication handlers

Velocity and Acceleration each register a `Register_NetAndSave_SharedApply` handler in their
`_Fragment.cpp`. One applier serves both transports because `Request_OverrideVelocity` /
`Request_OverrideAcceleration` from the payload is idempotent and host-safe.

Neither applier carries a per-feature `NeedsSetup` guard. It was removed deliberately: the late
`FGroup_DeferredApply` dispatch, the ConstructedThisFrame defer, and fire-gating together guarantee
the apply runs AFTER the setup drain, so the applied value is already final.

`FProcessor_Velocity_Replicate` consumes the registered `Produce` rather than building the payload
inline — one projection shared by the wire and the save file, byte-identical to the old inline build.
`Produced` is always set there; the processor's view guarantees the fragment.

## Anti-patterns

Don't call `AddForce` on Chaos bodies directly inside a processor — route through the acceleration modifier system so modifiers are composable and cancellable.

---

## See also

- `CkChaos/Claude.md`, `CkProjectile/Claude.md`.
