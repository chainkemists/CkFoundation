# CkShapes

**Purpose:** Shape definition fragments — box, sphere, capsule shapes stored on ECS entities, consumed by physics, overlap, and spatial query systems. Provides the shape data type layer that other modules use.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** `CkOverlapBody`, `CkSpatialQuery`, `CkRaySense`, `CkPhysics`.

---

## Key API

- `UCk_Utils_ShapeBox_UE`, `UCk_Utils_ShapeSphere_UE`, `UCk_Utils_ShapeCapsule_UE` — add shape entities.
- Shapes are Record entries on the owning entity.

---

## Pattern

Feature creates a shape entity and labels it; the consuming system (OverlapBody, SpatialQuery) reads the shape fragment to configure its query parameters.

---

## Anti-patterns

Don't hardcode shape extents inside overlap/physics feature modules — read from the Shape entity fragment so shapes are configurable and swappable.

---

## See also

- `CkOverlapBody/Claude.md`, `CkSpatialQuery/Claude.md`.
