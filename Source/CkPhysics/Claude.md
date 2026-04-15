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

## Anti-patterns

Don't call `AddForce` on Chaos bodies directly inside a processor — route through the acceleration modifier system so modifiers are composable and cancellable.

---

## See also

- `CkChaos/Claude.md`, `CkProjectile/Claude.md`.
