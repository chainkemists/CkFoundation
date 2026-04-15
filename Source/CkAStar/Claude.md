# CkAStar

**Purpose:** A* pathfinding on ECS grids — runs A* over a set of grid-cell entities to find shortest paths. Uses `CkGrid` cell entities as nodes.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`.
**Used by:** Turn-based movement, grid-based navigation.

---

## Key API

- `UCk_Utils_AStarTest_UE` (inherits `UCk_Utils_Ecs_Base_UE`) — pathfinding request / result.
- Returns ordered list of cell handles representing the path.

---

## Pattern

Pass the grid entity + start/end cell handles; the A* utility returns a path as an ordered array of `FCk_Handle_2dGridCell`.

---

## Anti-patterns

Don't run A* every frame if the grid doesn't change — cache paths and re-compute only on grid state changes.

---

## See also

- `CkGrid/Claude.md` — grid cell entities used as pathfinding nodes.
