# CkEntityCollection

**Purpose:** Entity collection tracking — groups of entity handles stored in a fragment, with change detection (previous vs. current set). Used for systems that need to track a set of entities and react when the set changes.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`.
**Used by:** `CkObjective` (tracks entities involved in objectives).

---

## Key API

- `UCk_Utils_EntityCollection_UE` — add, remove, query entity handle sets on a parent entity.
- Processors fire signals when the collection changes (added/removed entries).

---

## Pattern

Attach an EntityCollection fragment to a 'group' entity; processors detect changes by comparing current vs. previous snapshots each tick.

---

## Anti-patterns

Don't use EntityCollection for ordered sequences — it's a set, not a list.

---

## See also

- `CkRecord/Claude.md` — related concept for parent/child entity records.
- `CkObjective/Claude.md` — primary consumer.
