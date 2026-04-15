# CkAggro

**Purpose:** Aggro / threat management — tracks threat levels from multiple sources against a target entity. Drives AI target selection by maintaining a threat table.

**Depends on:** `CkAttribute`, `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** AI systems, MMO-style combat.

---

## Key API

- `UCk_Utils_Aggro_UE` (or similar) — add threat, remove threat, get highest-threat target.
- Built on `CkRelationship` for attitude filtering and `CkTargeting` for selection.

---

## Pattern

Damage dealt → add threat to aggro table → targeting processor queries aggro for highest-threat target.

---

## Anti-patterns

Don't maintain separate threat tables per feature module — use `CkAggro` as the single source of truth.

---

## See also

- `CkTargeting/Claude.md`, `CkRelationship/Claude.md`, `CkAttribute/Claude.md`.
