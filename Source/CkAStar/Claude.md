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

## Implementation notes

### Base template processors

`TProcessor_AStar_Execute` / `TProcessor_AStar_EndPlay` (`CkAStar_Processor.h`) are the shared bodies
consumers inherit from — add `Group`/`RunAfter` on the derived type and `CK_REGISTER_PROCESSOR` it:

```cpp
using FFragment_Goap_SearchState = ck::TFragment_AStar_SearchState<FGoapNodeId, FGoapGraph>;
using FFragment_Goap_Result      = ck::TFragment_AStar_Result<FGoapNodeId>;

struct FProcessor_Goap_Execute
    : ck::TProcessor_AStar_Execute<FProcessor_Goap_Execute,
          FCk_Handle_Goap, FFragment_Goap_SearchState, FFragment_Goap_Result>
{
    using TProcessor_AStar_Execute::TProcessor_AStar_Execute;
    using Group    = FGroup_Gameplay_AI;
    using RunAfter = TDepList<FProcessor_Goap_HandleRequests>;
};
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Execute);
```

### Stats live in the templated headers

`STAT_AStar_Execute` / `STAT_AStar_EndPlay` (`CkAStar_Processor.h`) and `STAT_AStar_ContinueSearch` /
`STAT_AStar_Iterations` (`CkAStar_Search.inl.h`) are declared in headers, so they create per-TU
internal-linkage statics — acceptable here because few TUs include them. One declaration per template
covers every derived/aliased processor that instantiates it, all reporting under `STATGROUP_CkAStar`
(the template's owning module). GOAP's aliases therefore surface under CkAStar rather than CkGoap —
intentional, since the executing code lives in CkAStar.

### Path cache invalidation

`TPathCache::InvalidateAll()` is lazy: it bumps a generation counter and frees nothing, so stale
entries are dropped opportunistically as `Find()` walks over them. `Clear()` is the hard reset.

### Precomputed table

`TPrecomputedTable` runs a full unlimited-budget A* per start/goal pair at build time until the memory
cap is hit, then answers queries by hash lookup. It is serializable so the table can be baked at cook
time.

---

## Anti-patterns

Don't run A* every frame if the grid doesn't change — cache paths and re-compute only on grid state changes.

---

## See also

- `CkGrid/Claude.md` — grid cell entities used as pathfinding nodes.
