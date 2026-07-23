# CkVisibleRange

**Purpose:** Distance-based range culling with a fade band, plus an independent explicit show/hide
override — composable onto any entity. Presence in this module's fragment set toggles a
reference-counted `FTag_VisibleRange_Hidden` so consumers get a fast filtered entt view
(`TExclude<FTag_VisibleRange_Hidden>`) instead of computing/branching per entity every frame.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLog`. Deliberately minimal — **zero knowledge of
`CkPoi`** or any consumer. Extracted as its own module specifically so it's reusable outside Poi
(aggro-range indicators, ambient-trigger fade-in, LOD-style visibility) — see root `CLAUDE.md`
non-negotiable #9.

**Used by:** `CkPoiDisplayDefinition` (parent→child visibility cascade — see that module's
`CLAUDE.md`); `CkCompass`/`CkMinimap` (CkPoi v2 Gate 3: projectors read the range/fade CONFIG via
`Get_MinRange`/`Get_MaxRange`/`Get_FadeBandCm` and keep their own distance math — the
`Update_Distance` + hidden-tag-exclusion flow is the Gate 4 upgrade); `CkMapDebugger` (max-range row).

---

## Key API

- `UCk_Utils_VisibleRange_UE::Add(InHandle, Params)` — direct-attach only. No `Create()`/child-entity
  variant exists; VisibleRange never spawns anything, it composes onto whatever entity you hand it.
- `Update_Distance(InHandle, InDistance)` — caller-supplied. VisibleRange has no concept of "viewer"
  (camera, local player, or otherwise) — resolving what to measure distance to is entirely the
  caller's job. A plain setter, not a request: it only replaces a cached input; the next due Update
  tick is what interprets it.
- `Request_SetVisibility(InHandle, ECk_VisibleRange_ShowHide)` — an explicit override, independent
  of range.
- `Get_IsHidden(InHandle)`, `Get_FadeAlpha(InHandle)`, `Get_MinRange`/`Get_MaxRange`/`Get_FadeBandCm`.
- `Compute_FadeAlpha`/`Compute_IsInRange` — pure statics (no entity needed), exposed for consumers
  that want the math without composing the feature.
- `BindTo_OnHiddenChanged` / `UnbindFrom_OnHiddenChanged`.

## The two-vote-source contract

`FTag_VisibleRange_Hidden` is `CK_DEFINE_ECS_TAG_COUNTED`, not a plain tag, because "hidden" has
**two independent sources**: the entity's own out-of-range state, and an explicit
`Request_SetVisibility(Hide)`. Either alone is enough to hide the entity; both must clear before
it's visible again. A plain tag cannot express this — one source's "clear" would silently wipe the
other source's still-active vote. Each source tracks whether it currently holds a vote
(`_IsOutOfRange`, `_IsExplicitlyHidden` on `FFragment_VisibleRange_Current`) so a repeated identical
call never leaks an extra increment.

`OnHiddenChanged` fires only on an actual 0↔>0 transition of the tag's presence (checked via
`Has<FTag_VisibleRange_Hidden>()` before/after each mutation), never on every vote — flipping one
source while the other is still active does not re-fire the signal.

**Consumer note for `CkPoiDisplayDefinition` (or any future parent→child cascade):** this counted
tag only carries VisibleRange's own two sources. A parent→child cascade needs its OWN separate tag
on the child, maintained by whichever module owns that relationship — do not try to add a third
vote onto this tag from outside the module. See `REFACTOR_MultiProjectorPoi.md` (CkPoi) and
`PROMPT.md` decision #5 for why this boundary is deliberate.

## Cadence — bucketed sub-processors (reference consumer of `CkProcessor_CadenceBuckets.h`)

`_UpdateInterval` is quantized at `Add` — toward FASTER, so an entity never updates slower than it
asked for — into the fixed bucket set `ck::cadence::BucketIntervalsSeconds` ({0, 0.1, 0.25, 0.5, 1,
2, 4}s; 0 = every tick), and the entity is tagged with `TTag_CadenceBucket<FCk_Handle_VisibleRange, N>`.
`FProcessor_VisibleRange_Update_Bucket<N>` (one instantiation per bucket, `CkVisibleRange_Processor.h`)
runs the shared eval body at its bucket's rate via the compile-time `TickRate` trait
(`ck::Seconds{...}`, resolved by `TProcessorBase::Get_TickRate`); bucket 0 declares no trait and runs
every tick. Due-ness is bucket-tag view membership — there is no per-entity chrono poll (the old
`_CadenceChrono` + `Tick(dt, Wrap)` gate is retired; `FCk_Chrono`'s `Wrap` policy itself remains the
documented few-entity throttle primitive in CkCore).

Immediate-first-eval: for nonzero buckets, `Add` (via `cadence::AddCadenceTags`) transiently also
joins bucket 0 and arms `TTag_CadenceFirstEval`; the eval body's first line
(`cadence::TryConsume_FirstEval`) strips both after the first evaluation — so the entity never shows
the default (visible) state for up to one full interval, the same guarantee the retired already-Done
Chrono seed provided. Quantization is internal to `Add`; the public API (C++/BP/AS) is unchanged.

Wake-alignment contract (load-bearing for `Ck_AutoTest_VisibleRange_CadenceGatesUpdates`): a bucket
processor's accumulator FREEZES while its view is provably empty (the scheduler skips the whole
dispatch), so a bucket's phase aligns to the moment its view last became non-empty. See
`CkEcs/DESIGN_SubInstancedCadenceProcessors.md` for the full analysis and accepted consequences.

## Anti-patterns

1. Don't give this module any knowledge of Poi, projectors, or "what a viewer is" — that's exactly
   the coupling it was extracted to avoid. If a consumer needs viewer resolution, that logic lives
   in the consumer, which then calls `Update_Distance`.
2. Don't use a plain `CK_DEFINE_ECS_TAG` for `FTag_VisibleRange_Hidden` if you ever add a third vote
   source — the counted variant exists specifically because two-plus independent sources need it.
3. Don't call `AddOrGet<FTag_VisibleRange_Hidden>()` to add a vote — `AddOrGet` does NOT increment a
   counted tag's depth on the already-present branch (`FCk_Registry::AddOrGet`, `CkRegistry.h:534-543`
   — it only bumps the dirty-marker version). Always use `Add<T>()`/`Remove<T>()` for counted tags;
   they're the ones with the increment/decrement logic (`CkRegistry.h:495-504,642-651`).

## See also

- `CkCore/Chrono/Claude.md` (or README) — `FCk_Chrono` for the primitive this module builds on.
- `CkPoi/REFACTOR_MultiProjectorPoi.md` — the design this module was extracted for.
