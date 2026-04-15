# CkObjective

**Purpose:** Objective system — quest-like objectives on entities with stages, conditions, and completion signals. Built on top of `CkCue`, `CkEntityCollection`, and `CkAttribute` for flexible goal tracking.

**Depends on:** `CkActorRelay`, `CkAttribute`, `CkCore`, `CkCue`, `CkEcs`, `CkEcsExt`, `CkEntityCollection`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** Quest systems, challenge tracking, tutorial steps.

---

## Key API

- `UCk_Utils_Objective_UE` — add objectives, complete/fail stages, query state.
- Signals: `OnObjectiveCompleted`, `OnObjectiveFailed`.

---

## Pattern

Objective entity holds stages (Record entries). Each stage has condition attributes. A processor ticks conditions and advances stages on completion.

---

## Anti-patterns

Don't hardcode objective logic in an EntityScript — define completion conditions via data assets and let the objective processor evaluate them.

---

## See also

- `CkAttribute/Claude.md` — progress tracking via attributes.
- `CkEntityCollection/Claude.md` — entity set tracking for multi-entity objectives.
