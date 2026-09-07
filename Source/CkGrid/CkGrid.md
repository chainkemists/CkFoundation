# CkGrid

2D grid system with cells, intersection detection, alignment checks, and debug visualization. Supports rotation, pivot anchors, and per-cell enable/disable.

## Key Concepts

- **Grid** — An N x M grid entity with configurable cell size, pivot transform, and 9 anchor positions (Center, BottomLeft, TopRight, etc.).
- **Cell** — Individual grid cell entity. Can be enabled or disabled. Has local bounds.
- **Intersection** — Per-cell overlap analysis between two grids, with snap position calculation.
- **Pivot Anchors** — 9 positions for grid origin placement. Changeable at runtime.
- **Debug Drawing** — Built-in AABB/OBB visualization with color coding.

## Example: Placing a Building on a Grid

```mermaid
flowchart LR
    A["Player drags building<br/>over grid"] -->|"Get_Intersections"| B["Check which cells<br/>overlap"]
    B -->|"all cells free"| C["Snap to grid<br/>place building"]
```

## Usage Examples

### Create a grid on an entity

```cpp
UCk_Utils_Grid2D_UE::Add(Entity, GridParams);
```

### Get cell at a coordinate

```cpp
auto Cell = UCk_Utils_Grid2D_UE::Get_CellAt(GridHandle, Coordinate);
```

### Check intersection between grids

```cpp
auto Intersections = UCk_Utils_Grid2D_UE::Get_Intersections(GridA, GridB);
bool Overlaps = UCk_Utils_Grid2D_UE::Get_IntersectsWith(GridA, GridB);
```

### Debug draw the grid

```cpp
UCk_Utils_Grid2D_UE::DebugDraw_Grid(GridHandle, World, Color, Duration);
```

### Convert between coordinates and world positions

```cpp
auto Coord = UCk_Utils_Grid_UE::Get_LocationAsCoordinate(WorldPos, CellSize);
auto WorldPos = UCk_Utils_Grid_UE::Get_CoordinateAsLocation(Coord, CellSize);
```

## Tests

`CkTests/Script/CkGrid` covers occupancy, placement and external occupant destruction.
The native `Ck.Grid.OccupancyScratch.LifecycleAndMapWorkload` test exercises production
stamping, replacement, destruction cleanup and grid isolation, plus an informational map microbenchmark.

## Occupancy reconciliation

`StampCells` deliberately sweeps every tick: record reverse-links can prune destroyed placements
without a grid dirty tag, and that must clear the old cell stamps. It builds the desired footprint,
diffs it against the previous footprint, and publishes the result in the same pass.

A private per-grid scratch map reuses allocation between passes. Displaced placement handles are
cleared immediately; scratch allocations above 256 KiB are released. This buffer is empty between
passes and is not snapshot or replication state.
