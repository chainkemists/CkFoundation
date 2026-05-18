# CkEntityTag

**Purpose:** Entity tag system beyond CkLabel — attaches multiple gameplay tags to an entity by name (via `FName`). Use when an entity needs a flexible, runtime-modifiable tag set that doesn't need the full Record semantics of `CkTagSet`.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** Gameplay systems that tag entities for filtering.

---

## Key API

- `UCk_Utils_EntityTag_UE::Add(InHandle, FName)` — add a named tag. `NAME_None` is rejected at the boundary (Display log, no-op) to prevent default-initialised names from polluting `ForEach_Entity(NAME_None)` queries.
- `Add_UsingGameplayTag(InHandle, FGameplayTag)` — same as Add via `Tag.GetTagName()`; an empty `FGameplayTag` reduces to NAME_None and is rejected by the same boundary.
- Standard Has, Remove, query operations.

> **Single-slot per entity.** `Add` and `Add_UsingGameplayTag` share one storage slot. A second Add (either flavor) on an entity that already has an EntityTag will hit entt's per-fragment uniqueness assertion (surfaces as a CK_ENSURE). Either gate calls on `Has(...)` or design entities so the tag is set once at construction.

---

## Pattern

Use when the tag set needs to change at runtime and you don't need the structural signals of `CkTagSet`.

---

## Anti-patterns

1. Don't use `CkEntityTag` when `CkLabel` (single role tag) or `CkTagSet` (replicated, signaled tag set) would be more appropriate.

---

## See also

- `CkLabel/Claude.md`, `CkTagSet/Claude.md`.
