# CkEntityTag

**Purpose:** Entity tag system beyond CkLabel — attaches multiple gameplay tags to an entity by name (via `FName`). Use when an entity needs a flexible, runtime-modifiable tag set that doesn't need the full Record semantics of `CkTagSet`.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** Gameplay systems that tag entities for filtering.

---

## Key API

- `UCk_Utils_EntityTag_UE::Add(InHandle, FName)` — add a named tag. `NAME_None` is rejected at the boundary (Display log, no-op) to prevent default-initialised names from polluting `ForEach_Entity(NAME_None)` queries. Deferred — see **Timing**.
- `Add_UsingGameplayTag(InHandle, FGameplayTag)` — adds the gameplay tag plus its full parent chain as FName entries (e.g., `A.B.C` registers `A.B.C`, `A.B`, `A`), so `ForEach_Entity("A.B")` finds entities tagged with any `A.B.*` descendant. Deferred — see **Timing**.
- `Has` / `Has_UsingGameplayTag`, `Get_AllTags`, `Get_AllTagsAsContainer` (explicit gameplay tags only — does not include parent-flattened ancestors).
- `Request_TryRemove` / `Request_TryRemove_UsingGameplayTag` — returns `Succeeded` whenever the handle is valid; the tag-presence check moved into the processor and silently no-ops on absent tags. Deferred — see **Timing**.
- `BindTo_OnTagUpdated` / `BindTo_OnGameplayTagUpdated` signals — fire on the 0↔1 presence flip only (not on intermediate count changes). The gameplay-tag binding supports a `RelevantTags` filter container.
- `UCk_Utils_EntityTagQuery_UE` — typesafe query entities over the tag set. See **Query system** below.

> **Counted Add/Remove.** A tag added N times needs N removes to disappear. `Has` reports presence (count ≥ 1); signals fire only on the 0→1 (Added) and 1→0 (Removed) transitions. Parent FNames added via a gameplay-tag Add are reference-counted across all gameplay-tag children that share the ancestor, so removing one child does not strip parent FNames still held by siblings.

---

## Timing

`Add`, `Add_UsingGameplayTag`, `Request_TryRemove`, and `Request_TryRemove_UsingGameplayTag` are deferred via the request pump. The actual `_Tags` / `_GameplayTagCounts` mutation and the matching `OnTagUpdated` / `OnGameplayTagUpdated` signal fire happen **one processor pass later**. Callers that observe state in the same tick (`Has`, `Get_AllTags`, query satisfaction) must `WaitOneFrame` after the mutation. Consequence for `Request_TryRemove`: the call returns `Succeeded` on any valid handle — the per-tag presence check is in the processor, not at the call site, and an absent tag silently no-ops.

---

## Pattern

Use when the tag set needs to change at runtime and you don't need the structural signals of `CkTagSet`.

---

## Query system

`UCk_Utils_EntityTagQuery_UE` exposes a reactive query entity that watches the global EntityTag store and fires when its requirements are satisfied.

- **`Add(InOwnerEntity)`** — creates a typesafe `FCk_Handle_EntityTagQuery` owned by the supplied entity. Destruction cascades when the owner is destroyed.
- **`Request_AddRequirement(Query, FCk_Request_EntityTagQuery_AddRequirement{Requirement})`** / **`Request_RemoveRequirement(Query, ...)`** — mutate the requirement set. Each requirement is an `FCk_EntityTagQuery_Requirement` containing the target `FName` tag, a count mode, and an optional ensure cap.
- **Count modes (`ECk_EntityTagQuery_CountMode`):**
  - `SingleOnly` — cap = 1; query satisfied when exactly one matching entity exists.
  - `Count` — cap = N; query satisfied when ≥ N matches exist.
  - `All` — unbounded; query re-fires per *new* match after first satisfaction.
- **Convenience factories** (12 UFUNCTIONs) — `Make_Requirement_Single` / `Make_Requirement_Of` / `Make_Requirement_All` (FName flavor) plus the `*_WithEnsure` variants that take a `_MaxAllowedEnsure` cap, plus the `*_FromGameplayTag` variants that accept `FGameplayTag` instead of `FName`.
- **`BindTo_OnSatisfied(Query, BindingPolicy, PostFireBehavior, Delegate)`** — observe satisfaction. Standard `ECk_Signal_BindingPolicy` / `ECk_Signal_PostFireBehavior` semantics.
- **Inspection:** `Get_IsSatisfied(Query)`, `Get_CurrentResults(Query)`, `Get_AllRequirements(Query)`.
- **Fire conditions:**
  1. First satisfaction (transition from unsatisfied → satisfied).
  2. `All`-mode re-fire on each new match while satisfied.
  3. Count drop-and-recover — fires again after the result count falls below the cap and rises back.
- **Lazy-prune.** Cached results are revalidated immediately before fire. The user-facing guarantee is that every handle in the payload is valid and currently carries its requirement's tag — stale entries are filtered out.
- **Ensure cap.** Each requirement carries an optional `_MaxAllowedEnsure`; pass `FCk_EntityTagQuery_Requirement::NoEnsure` to disable. When set, an ensure fires if the global count of entities tagged with the requirement's tag exceeds the cap (catches runaway tagging in development).
- **Empty queries never fire.** A query with zero requirements is a degenerate case and is treated as never satisfied.

---

## Anti-patterns

1. Don't use `CkEntityTag` when `CkLabel` (single role tag) or `CkTagSet` (replicated, signaled tag set) would be more appropriate.
2. Don't read `Has` / `Get_AllTags` in the same tick as the `Add`/`Request_TryRemove` that triggered the change — the mutation is deferred. Use the `OnTagUpdated` signal or `WaitOneFrame`.

---

## See also

- `CkLabel/Claude.md`, `CkTagSet/Claude.md`.
- Debugger inspectors: `FCkInspector_EntityTag`, `FCkInspector_EntityTagQuery` (in CkGameplayDebugger).
- Design + plan: `Source/CkEntityTag/docs/2026-05-25-binds-and-queries-design.md`, `Source/CkEntityTag/docs/2026-05-25-binds-and-queries-plan.md`.
