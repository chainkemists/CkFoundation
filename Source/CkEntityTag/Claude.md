# CkEntityTag

**Purpose:** Entity tag system beyond CkLabel — attaches multiple gameplay tags to an entity by name (via `FName`). Use when an entity needs a flexible, runtime-modifiable tag set that doesn't need the full Record semantics of `CkTagSet`.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** Gameplay systems that tag entities for filtering.

---

## Key API

- `UCk_Utils_EntityTag_UE::Add(InHandle, FName)` — add a named tag. `NAME_None` is rejected at the boundary (Display log, no-op) to prevent default-initialised names from polluting `ForEach_Entity(NAME_None)` queries.
- `Add_UsingGameplayTag(InHandle, FGameplayTag)` — adds the gameplay tag plus its full parent chain as FName entries (e.g., `A.B.C` registers `A.B.C`, `A.B`, `A`), so `ForEach_Entity("A.B")` finds entities tagged with any `A.B.*` descendant.
- `Has` / `Has_UsingGameplayTag`, `Get_AllTags`, `Get_AllTagsAsContainer` (explicit gameplay tags only — does not include parent-flattened ancestors).
- `Request_TryRemove` / `Request_TryRemove_UsingGameplayTag` (rejects partial matches via `HasTagExact` guard).
- `BindTo_OnTagUpdated` / `BindTo_OnGameplayTagUpdated` signals — fire on the 0↔1 presence flip only (not on intermediate count changes). The gameplay-tag binding supports a `RelevantTags` filter container.

> **Counted Add/Remove.** A tag added N times needs N removes to disappear. `Has` reports presence (count ≥ 1); signals fire only on the 0→1 (Added) and 1→0 (Removed) transitions. Parent FNames added via a gameplay-tag Add are reference-counted across all gameplay-tag children that share the ancestor, so removing one child does not strip parent FNames still held by siblings.

---

## Pattern

Use when the tag set needs to change at runtime and you don't need the structural signals of `CkTagSet`.

---

## Anti-patterns

1. Don't use `CkEntityTag` when `CkLabel` (single role tag) or `CkTagSet` (replicated, signaled tag set) would be more appropriate.

---

## See also

- `CkLabel/Claude.md`, `CkTagSet/Claude.md`.
