# Gate 1 — Cadence primitive + CkVisibleRange

> **Status:** ✅ Done (2026-07-21)
> **Depends on:** — (first gate)
> **Estimate:** 1 session — actual: 1 session, ~3 fork attempts + direct cleanup (see PROGRESS.md)

## Goal

"After this gate: `ck::cadence::ShouldRun` exists in `CkCore`; `CkVisibleRange` exists as a fully
standalone, compiling, tested module with zero knowledge of Poi. Composing `VisibleRange` on any
entity with a Transform makes it gain/lose `FTag_VisibleRange_Hidden` as it crosses its configured
range relative to a caller-supplied distance, at the entity's own configured cadence (not every
tick unless `_UpdateInterval == 0`). An explicit `Request_SetVisibility` can independently force
hidden/shown, additively with the range-based state (both must clear for the entity to be visible)."

## Entry criteria

- [ ] `REFACTOR_MultiProjectorPoi.md` and `PROMPT.md` read in full this session.
- [ ] `CkTimer` quartet read as the scaffold template (`Source/CkTimer/Public/CkTimer/`).
- [ ] `CkChrono.h` and `CkChrono_Utils.h` (if it exists) read — confirm `FCk_Chrono::Tick()`'s exact
      return semantics (does it auto-reset on `Done`, or does the caller call `Reset()`?) before
      writing `ck::cadence::ShouldRun` — this was inferred from the header, not confirmed by reading
      the .cpp implementation.
- [ ] `CkSubstep_Fragment.h:9-10` re-read for the tag-swap pattern shape.
- [ ] `CK_DEFINE_ECS_TAG_COUNTED` macro definition read (`ck-macros-and-codegen` skill or grep) to
      confirm its exact generated API (how add/remove/depth-check work) before using it — cited in
      the doctrine's macro table but not yet read in full this campaign.

## Work items

1. **`ck::cadence::ShouldRun(FCk_Chrono& InOutChrono, FCk_Time InDeltaT) -> bool`** —
   `CkCore/Public/CkCore/Chrono/CkChrono_Utils.h` (new file if none exists). Wraps `Tick()`; "0
   interval" special-case matches the existing `if (UpdateInterval > 0 && ...)` convention from
   `CkCompass_Processor.cpp:178-185`. NEW INFRASTRUCTURE — no direct prior art for the free
   function itself, though the accumulate-and-compare logic it wraps is proven 3+ times over.
2. **`CkVisibleRange` module scaffold** — Build.cs pattern-replicated from `CkTimer.Build.cs`; deps
   `Core,Ecs,EcsExt,Log`; register in `CkFoundation.uplugin`. Add row to `Source/CLAUDE.md`'s module
   tier table (T4) and "Finding the right module" table once compiling.
3. **`CkVisibleRange_Fragment_Data.h`** — `FCk_Fragment_VisibleRange_ParamsData`
   (`_MinRange`/`_MaxRange`/`_FadeBandCm`/`_UpdateInterval`), `FCk_Handle_VisibleRange`,
   `ECk_VisibleRange_ShowHide` enum, `FCk_Request_VisibleRange_SetVisibility`. Pattern: `CkTimer`'s
   equivalent file for shape; `FCk_Fragment_Poi_ParamsData` (current `CkPoi_Fragment_Data.h`,
   pre-deletion) for the Min/Max/FadeBand field shape being migrated.
4. **`CkVisibleRange_Fragment.h`** — `FFragment_VisibleRange_Current` (`FCk_Chrono _Chrono`,
   `float _FadeAlpha`, `bool _IsOutOfRange`, `bool _IsExplicitlyHidden`), `FTag_VisibleRange_Hidden`
   via `CK_DEFINE_ECS_TAG_COUNTED`, `FFragment_VisibleRange_Requests`, signal
   `OnVisibleRange_HiddenChanged` (`CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE`).
5. **`CkVisibleRange_Processor.h/.cpp`** —
   - `FProcessor_VisibleRange_Update`: `_TickRate = 0`; gate per-entity work on
     `ck::cadence::ShouldRun`; caller-supplied distance (param or a bound provider — confirm shape
     against how `CkCompass`/`CkMinimap` currently obtain viewer distance before designing the
     signature); on boundary crossing enqueue a request.
   - `FProcessor_VisibleRange_HandleRequests`: applies range-crossing and `SetVisibility` requests
     to the counted tag independently (§ PROMPT.md decision #4 — exactly two sources); fires
     `OnVisibleRange_HiddenChanged` only on an actual 0↔>0 transition of the tag's depth, not on
     every vote.
6. **`CkVisibleRange_Utils.h/.cpp`** — `Add`, `Get_MinRange`/`Get_MaxRange`/`Get_FadeBandCm`/
   `Get_FadeAlpha`, `Request_SetVisibility`, `Compute_FadeAlpha`/`Compute_IsInRange` (pure statics),
   `BindTo_OnHiddenChanged`/`UnbindFrom_OnHiddenChanged`.
7. **`Claude.md`** for the new module (purpose, key API, anti-patterns) — required by
   module-authoring rules before this gate can exit.
8. **AutoTests** proving, independently: (a) own-range boundary crossing toggles the tag; (b) an
   explicit `Request_SetVisibility(Hide)` toggles the tag independently of range state, and both
   must clear before the entity is visible again; (c) cadence actually skips work — assert the
   fade/hidden state does NOT update on a tick where `DeltaT` hasn't reached `_UpdateInterval`, only
   the visible one-frame-later. (c) is the one most likely to be silently wrong if implemented
   sloppily — don't skip it.

## Expected observations at the gate — and what to do on each branch

| I will run | I expect to observe | If instead I see | Prewritten response |
|---|---|---|---|
| `CkVisibleRange` AutoTest suite via toolbox | All new tests pass; module compiles clean, no warnings | Compile error citing `CK_DEFINE_ECS_TAG_COUNTED` usage | Re-read the macro's actual generated members — the doc-table description may be incomplete; don't guess the API |
| Cadence skip-test (8c) | Hidden/fade state unchanged on an under-threshold tick, changed on the tick that crosses `_UpdateInterval` | State updates every tick regardless of `_UpdateInterval` | `ck::cadence::ShouldRun` isn't gating the work — check it's actually called before the compute, not after |
| Two-source independence test (8b) | Tag depth is 2 when both range-hidden AND explicitly-hidden are true; entity stays hidden until BOTH clear | Removing one source's vote makes the entity visible while the other vote is still active | The plain-tag pitfall from the design doc — confirm `CK_DEFINE_ECS_TAG_COUNTED` (not the plain variant) is actually in use |

## Exit criteria — ALL items land in the SAME commit as the last work item

- [ ] All 8 work items complete; AutoTests pass via toolbox (fresh run, this gate's code, not a
      stale green from before this gate's edits)
- [ ] `ck-change-control` done-checklist run for a new-module change
- [ ] `CkVisibleRange/Claude.md` written
- [ ] `Source/CLAUDE.md` module tier table + decision-tree table updated with the new module
- [ ] PLAN.md status row AND this file's Status header updated — same commit
- [ ] PROGRESS.md dated entry appended with actual estimate-vs-actual and any deviations from the
      work-item list above
