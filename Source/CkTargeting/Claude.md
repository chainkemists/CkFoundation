# CkTargeting

**Purpose:** Target selection — finds the best target entity from a candidate set using scoring functions (distance, angle, attribute values, relationship attitude). Feeds into `CkResolver`, `CkAggro`, and ability systems.

**Depends on:** `CkActor`, `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** `CkAggro`, `CkChaos`, `CkResolver`, `CkRaySense`.

---

## Key API

- `UCk_Utils_TargetPoint_UE::Create_New_TargetPoint(InParams)` — create a target point entity.
- Targeting entities hold candidate lists and scoring fragment data.
- Processors score candidates and expose best target via fragment.

---

## Pattern

Gather candidate entities (via `CkSpatialQuery` or explicit list) → create targeting entity → scoring processor evaluates each candidate → best target written to `FFragment_Targeting_BestTarget`.

---

## Anti-patterns

1. Don't score targets inside a `ForEachEntity` body that's already iterating other fragments — separate target scoring into its own processor phase.
2. Don't use targeting for self-targeted abilities; just read the owner entity handle.

---

## See also

- `CkSpatialQuery/Claude.md`, `CkRelationship/Claude.md`, `CkAggro/Claude.md`.
