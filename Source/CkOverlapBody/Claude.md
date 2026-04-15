# CkOverlapBody

**Purpose:** ECS-managed collision bodies — `UShapeComponent`-backed entities (box, sphere, capsule) that notify via signals on overlap begin/end. Cleaner than raw actor overlap delegates: overlaps fire ECS signals consumed by processors.

**Depends on:** `CkActor`, `CkCore`, `CkEcs`, `CkEcsExt`, `CkGraphics`, `CkLabel`, `CkLog`, `CkPhysics`, `CkRecord`, `CkSettings`.
**Used by:** Ability hit detection, trigger volumes, melee ranges.

---

## Key API

- `UCk_Utils_Marker_UE` (inherits `UCk_Utils_Ecs_Base_UE`) — add overlap body entities with shape params.
- Signals: `OnOverlapBegin`, `OnOverlapEnd` fired to the parent entity.

---

## Pattern

Create overlap body entity on the attacker; bind `OnOverlapBegin` signal to detect hits; destroy on ability end.

---

## Anti-patterns

1. Don't bind actor overlap delegates directly in processors — use `CkOverlapBody` signals.
2. Don't leave overlap bodies alive after the ability/feature ends.

---

## See also

- `CkEcs/Claude.md` — signal binding.
- `CkPhysics/Claude.md` — physics integration.
- `CkShapes/Claude.md` — shape type data.
