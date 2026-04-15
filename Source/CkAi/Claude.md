# CkAi

**Purpose:** AI utilities — Environment Query System (EQS) wrappers for ECS. Allows processors to run UE EQS queries and read results as ECS fragment data.

**Depends on:** `CkCore`, `CkEcs`, `CkLog`.
**Used by:** AI decision-making processors.

---

## Key API

- `UCk_Utils_Eqs_UE` — run EQS query, get result set as entity handles or world locations.
- Results are written to a fragment for downstream processors.

---

## Pattern

AI processor requests an EQS query; the result processor writes found locations to a fragment; decision processor reads the fragment.

---

## Anti-patterns

Don't run EQS from inside a tight `ForEachEntity` loop without throttling — EQS is expensive.

---

## See also

- `CkPerception/Claude.md`, `CkTargeting/Claude.md`, `CkSpatialQuery/Claude.md`.
