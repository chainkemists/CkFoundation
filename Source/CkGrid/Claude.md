# CkGrid

**Purpose:** 2D spatial grid — grid cell entities forming a grid on a parent entity. Used for inventory grids, tactical maps, tile-based systems.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`.
**Used by:** `CkInventory` (inventory slot grid).

---

## Key API

- `UCk_Utils_2dGridCell_UE` — add cells, query by grid coordinate, enumerate cells.
- `FCk_Handle_2dGridCell` — handle type for individual grid cells.

---

## Pattern

Create a grid entity on a container (inventory, map), then add rows×cols GridCell entities. Each cell is a Record entry labeled by grid coordinate.

---

## Anti-patterns

Don't flatten the grid into a plain array in a fragment — the Record pattern enables per-cell fragment composition (item stacking, cell states).

---

## See also

- `CkInventory/Claude.md` — primary consumer of the grid system.
- `CkRecord/Claude.md` — Record pattern underpinning the grid.
