# CkProjectile

**Purpose:** Projectile entities — ballistic/physics-driven projectiles with velocity, range, hit detection, and penetration. Built on `CkPhysics` for movement and `CkOverlapBody` (optionally) for hit detection.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLog`, `CkPhysics`, `CkRecord`, `CkVariables`.
**Used by:** Ranged combat, thrown objects.

---

## Key API

- `UCk_Utils_Projectile_UE::Add(InHandle, InParams)` — spawn a projectile entity.
- Signals: `OnHit`, `OnRangeExpired`.

---

## Pattern

Projectile entity has velocity/range fragments; `FProcessor_Projectile_Move` advances position; hit detection fires `OnHit` signal to the instigator entity.

---

## Anti-patterns

Don't use UE's `AProjectile` actor for ECS-driven projectiles — the overhead of a full actor per projectile defeats the purpose.

---

## See also

- `CkPhysics/Claude.md` — acceleration/movement.
- `CkOverlapBody/Claude.md` — collision detection.
