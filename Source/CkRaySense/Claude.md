# CkRaySense

**Purpose:** Raycast sensing — ECS-managed raycasts fired from an entity each tick, with results stored in a fragment and exposed via signals. Use for line-of-sight checks, ground detection, and range sensing.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`, `CkShapes`.
**Used by:** AI sight, ledge detection, jump prediction.

---

## Key API

- `UCk_Utils_RaySense_UE::Add(InHandle, InParams)` — attach a ray sense to an entity.
- `FFragment_RaySense_Current` — last hit result (actor, component, distance, normal).

---

## Pattern

RaySense entity fires a raycast in `ForEachEntity` each tick and writes results to its fragment. Downstream processors read the fragment for decision-making.

---

## Anti-patterns

Don't call `World->LineTraceSingleByChannel` directly inside processors — route through RaySense so results are cached and deduplicated per entity per tick.

---

## See also

- `CkSpatialQuery/Claude.md` — volume-based spatial queries.
- `CkPerception/Claude.md` — AI perception (higher-level).
