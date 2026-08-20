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

## Implementation notes

### Occupancy reconcile

The occupancy layer is record-driven: placements live as child entities in the grid's
`FFragment_RecordOf_GridPlacements`, and `FProcessor_2dGridOccupancy_StampCells` reconciles the
cells from that record. It is deliberately **un-gated** (runs every tick) rather than dirty-tagged:
a destroyed placement is pruned from the record by CkRecord's reverse-link whenever that lands, so
no dirty tag can be relied on to re-trigger the un-stamp. The desired-vs-stamped diff makes an
unchanged grid a no-op. Dirty-gating is a candidate optimisation if profiling ever warrants it.

Consequence for readers: the `FTag_2dGridCell_Occupied` stamp and the cell's back-ref lag one
reconcile tick behind a placement's destruction. `Get_PlacementAt` therefore treats a
pending-destroy placement as already gone, and `Get_CanPlace` must query
`UCk_Utils_2dGridOccupancy_UE::Get_OccupantAt` rather than reading the raw `Occupied` tag —
otherwise a same-tick remove + re-place is falsely rejected until the reconcile catches up.

The whole replication path (`MayRequireReplication` dirty tag → authority `Replicate` rebuilds
`FCk_RepData_2dGridPlacements` → client `SyncReplication` diffs current-vs-previous by occupant
identity) mirrors CkInventory's spatial-inventory replication.

### Occupancy persistence

The save/load handlers (`Ck2dGridOccupancy_Fragment.cpp`) are all keyed on the **grid** entity —
that is where the replicated container entry lives — so there is no record indirection between the
payload and the entity it applies to. The load path needs its own `HydrationApply` because the
`SyncReplication` processor is ClientOnly and never runs on the authority host.

`HydrationApply` deliberately does **not** gate on occupant validity. A restored placement's
occupant is deterministically invalid (an unlabeled ConstructSpawned child is save-transient and
comes back as `entt::null`), and `Request_AddPlacement` adds the placement and its Cells regardless
— the Cells are the authoritative data, and an invalid occupant only skips the death-watch. A
`NotReady` gate on the occupant would never flip and would drop the whole payload once the
dispatcher timeout expired.

What survives a load is the placement — grid, anchor, rotation, cells — and
`UCk_Utils_2dGridPlacement_UE::Get_Grid` / `Get_Anchor` / `Get_Rotation` read those back off a live
placement handle (pair them with `UCk_Utils_2dGridOccupancy_UE::Get_PlacementForOccupant` to go
occupant → placement → args). They store nothing; they exist so a consumer whose own DERIVED state is
`Session` — a navmesh cut, a footprint outline — can rebuild it from the placement after a load
instead of re-running the spatial resolve that produced the anchor, which would be a second
implementation of placement math free to drift from this one.

Branch on `Get_Grid` (or on the placement handle's validity) **before** reading anchor or rotation:
those two answer `(0,0)` and `None` for an invalid placement, which is exactly what a real placement at
the grid origin answers. There is no `TryGet` variant on purpose — the grid read already separates
"no placement" from every possible anchor, and a second way to ask one question is how the two drift.

---

## See also

- `CkInventory/Claude.md` — primary consumer of the grid system.
- `CkRecord/Claude.md` — Record pattern underpinning the grid.
