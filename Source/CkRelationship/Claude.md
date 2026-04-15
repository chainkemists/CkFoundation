# CkRelationship

**Purpose:** Entity-to-entity relationship system — typed relationships (ally, enemy, neutral) between entity handles. Relationship queries drive targeting, aggro, and AI decision-making.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** `CkAggro`, `CkTargeting`, `CkResolver`, AI systems.

---

## Key API

- `ECk_RelationshipAttitude` — `Friendly`, `Hostile`, `Neutral`.
- `UCk_Utils_Relationship_UE` — set/query relationship between entity pairs.

---

## Pattern

Each entity that participates in team/faction logic has a Relationship fragment. Targeting and aggro processors query `Get_Attitude(EntityA, EntityB)` to filter candidates.

---

## Anti-patterns

Don't hardcode faction checks in individual feature modules — always go through `CkRelationship` so the attitude logic can be data-driven.

---

## See also

- `CkTargeting/Claude.md`, `CkAggro/Claude.md`.
