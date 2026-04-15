# CkSubstep

**Purpose:** Substep physics integration — allows processors to tick at a higher rate than the game thread by hooking into UE's physics substep callback. Use for precise physics-driven entity updates.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`.
**Used by:** High-precision physics simulation (projectiles, ropes, cloth).

---

## Key API

- `UCk_Utils_Substep_UE` (inherits `UCk_Utils_Ecs_Base_UE`) — register entities for substep callbacks.
- Substep processors receive a per-substep DeltaT.

---

## Pattern

Substep entity hooks into UE's physics substep tick; its `ForEachEntity`-equivalent runs multiple times per game frame at physics step rate.

---

## Anti-patterns

1. Don't read gameplay state in a substep processor — it may be mid-frame. Only read/write physics-owned data.
2. Don't use substep for non-physics systems — the overhead isn't justified.

---

## See also

- `CkPhysics/Claude.md`, `CkProjectile/Claude.md`.
