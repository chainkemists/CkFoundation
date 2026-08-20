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
- **Version-gated evaluation.** Every tag gain/loss bumps a per-tag mutation version (`Set_StoragePresence` → `BumpDirtyMarkerVersion`), and `Evaluate` skips its prune/append/ensure scans whole when no requirement's version has moved since the last evaluated pass. The contract: anything that invalidates cached results *without* touching a tag storage must set `_NeedsEvaluate` — requirement add/remove and the tracked-entity destructor already do (an entity dying mutates no tag storage, so nothing else would re-arm the gate).
- **Ensure cap.** Each requirement carries an optional `_MaxAllowedEnsure`; pass `FCk_EntityTagQuery_Requirement::NoEnsure` to disable. When set, an ensure fires if the global count of entities tagged with the requirement's tag exceeds the cap (catches runaway tagging in development).
- **Empty queries never fire.** A query with zero requirements is a degenerate case and is treated as never satisfied.
- **`OnContinuousUpdate` is delta-only, opt-in.** It broadcasts only on Evaluate passes where the result set actually changed (an entity entered or left a requirement's results), never every pass. Consumers react to the per-requirement `_Added` / `_Removed` deltas and no-op on empty ones, so a no-change pass has nothing to deliver; skipping it avoids a per-frame payload alloc + broadcast + delegate invocation per bound query — the dominant cost when many queries each hold a continuous listener. A satisfaction flip cannot happen without a result-count change, so `_IsSatisfied` transitions still ride out on the same pass as their delta. The whole block is skipped when the listener refcount is 0.
- **Delta accumulators reset at the end of every Evaluate pass.** Deltas not consumed by a fire that pass are dropped deliberately: `OnContinuousUpdate` consumes them exactly on the passes that changed, and the `OnSatisfied` path captures them when it fires. `FProcessor_EntityTagQuery_TrackedEntity_Destructor` writes `_PendingRemoved` in `FGroup_EndPlay` (after Eval), so those writes survive to the next frame's pass.

---

## Save/load restore

EntityTag persists through a **save-only** handler (`Register_SaveOnly<FCk_SaveData_EntityTags>`, in `CkEntityTag_Fragment.cpp`). The feature is UNREPLICATED — no Replicate processor, no RepData, no `MayRequireReplication` — so the handler has no net Apply, only `Produce` + `HydrationApply`, and there is no re-arm step: the local restore IS the whole restore.

- **Transport shape.** `ck::FEntityTagCount` is a plain (non-USTRUCT) struct, so the counted FName set is flattened into two parallel reflected arrays. The `FCk_SaveData_` prefix (deliberately NOT `FCk_RepData_`) keeps it off the RepData census ratchet (`Ck.Snapshot.Meta.RepDataRestoreCoverage`, which enumerates `FCk_RepData_*` only).
- **Scope.** Only the FName `_Tags` set is persisted. The parallel `FGameplayTag _GameplayTagCounts` view is NOT captured — on restore each name is re-added, rebuilding `_Tags`, the per-tag EnTT storage (`ForEach_Entity`) and `Has` / `Has_UsingGameplayTag` (both read `_Tags`), but `Get_AllTagsAsContainer()` (which reads `_GameplayTagCounts`) will not reflect gameplay-tag-only state.
- **Reconstitute-by-request** (a lazily-composed, data-defined feature — `CkSnapshot/Claude.md` §5). `FFragment_EntityTag_Current` is composed lazily by `Add` and auto-removed at zero tags, so it has no "composed-but-empty" state. The v3 load rebuilds entities from spawn recipes (Construct) and does not restore raw fragments, and a generic entity's Construct does not re-compose EntityTag — so the usual "gate on `Has<feature>` → `NotReady`" applier shape would spin to the hydration timeout and drop the payload loudly. Instead `HydrationApply` enqueues exactly ONE composite `FCk_Request_EntityTag_RestoreSet` and returns `Applied`; its drain-time handler `UCk_Utils_EntityTag_UE::DoApply_RestoreSet` is what actually composes Current, rebuilds the storage presence, and fires the Added/Removed signals.
- **Why a composite request, not read-live-then-clear-then-Add.** A rebuilt entity's Construct/BeginPlay may seed EntityTag tags through the same deferred Add requests, but `FProcessor_EntityTag_HandleRequests` is GatedDuringLoad — at `HydrationApply` time those seeds are enqueued and invisible to any `Has<>` / `Get_` read of the "current" set. Reading the live set there and clearing it would MERGE the construct-seeds under the saved adds (monotonic count inflation each save/load cycle). The RestoreSet request rides the SAME FIFO array AFTER those seeds, so its handler runs once they have materialised and diffs against the TRUE live set, reconstituting exactly the saved `{name -> count}` map regardless of pump ordering. Idempotent under double-apply.
- **Not the forbidden "compose in the net Apply" race.** This is the `HydrationApply` slot only (never assigned to a net Apply) and runs authority-side after construction (the dispatcher skips `FTag_EntityScript_ConstructedThisFrame`).
- **Absence is ambiguous.** `Produce` returns UNSET both when the entity never had EntityTag and when every tag was removed pre-save (Current auto-removed at zero). An UNSET payload emits nothing to hydrate, so a Construct-seeded default tag RESURRECTS on load. Pinned as chosen behavior (`CkSnapshot/Claude.md` §5, and the EntityTag parity gate's resurrection scenario).
- **`DoApply_RestoreSet` ordering.** Raises/adds every saved tag to its exact count FIRST — before any removal — so the live set never transiently empties and trips the NowEmpty auto-remove mid-apply; then removes every present tag not in the saved set (gathered first, since mutating `_Tags` while iterating it is unsafe). Storage presence is set unconditionally and idempotently; signals fire only on 0↔1 presence flips, mirroring `DoApply_Add` / `DoApply_TryRemove`. A None name or non-positive saved count means "not present" and is skipped — `Produce` never emits those, the guard is for a hand-built or corrupt payload. The trailing NowEmpty guard is only reachable on a degenerate empty saved set.

---

## Anti-patterns

1. Don't use `CkEntityTag` when `CkLabel` (single role tag) or `CkTagSet` (replicated, signaled tag set) would be more appropriate.
2. Don't read `Has` / `Get_AllTags` in the same tick as the `Add`/`Request_TryRemove` that triggered the change — the mutation is deferred. Use the `OnTagUpdated` signal or `WaitOneFrame`.

---

## See also

- `CkLabel/Claude.md`, `CkTagSet/Claude.md`.
- Debugger inspectors: `FCkInspector_EntityTag`, `FCkInspector_EntityTagQuery` (in CkGameplayDebugger).
- Design + plan: `Source/CkEntityTag/docs/2026-05-25-binds-and-queries-design.md`, `Source/CkEntityTag/docs/2026-05-25-binds-and-queries-plan.md`.
