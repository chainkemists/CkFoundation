# CkEntityTag

**Purpose:** Entity tag system beyond CkLabel — attaches multiple gameplay tags to an entity by name (via `FName`). Use when an entity needs a flexible, runtime-modifiable tag set that doesn't need the full Record semantics of `CkTagSet`.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** Gameplay systems that tag entities for filtering.

---

## Key API

- `UCk_Utils_EntityTag_UE::Add(InHandle, FName)` — add a named tag.
- Standard Has, Remove, query operations.

---

## Pattern

Use when the tag set needs to change at runtime and you don't need the structural signals of `CkTagSet`.

---

## Anti-patterns

1. Don't use `CkEntityTag` when `CkLabel` (single role tag) or `CkTagSet` (replicated, signaled tag set) would be more appropriate.

---

## See also

- `CkLabel/Claude.md`, `CkTagSet/Claude.md`.
