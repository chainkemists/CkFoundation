# CkChaos

**Purpose:** Utilities for UE's Chaos destruction system — specifically `UGeometryCollectionComponent` queries (closest particle index to location, etc.). Thin wrapper, no ECS Record pattern.

**Depends on:** `CkAttribute`, `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`, `CkTargeting`.
**Used by:** Destruction and physics features.

---

## Key API

- `UCk_Utils_Chaos_UE::Get_ClosestParticleIndexToLocation(UGeometryCollectionComponent*, FVector)` — returns particle index nearest a world position.

---

## Pattern

Used to query destructible mesh state for targeting, VFX anchor, or physics force application.

---

## Anti-patterns

Don't query Chaos particle state every frame without caching — particle topology changes on break events.

---

## See also

- `CkPhysics/Claude.md` — physics modifiers.
- `CkTargeting/Claude.md` — targeting system that uses chaos queries.
