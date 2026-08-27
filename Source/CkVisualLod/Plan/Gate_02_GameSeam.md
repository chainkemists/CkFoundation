# Gate 2 — Game seam: viewer discovery + test coverage

> **Status:** 🟡 In progress (entered 2026-08-27)
> **Depends on:** Gate 1 ✅ (368651f5e)
> **Estimate:** 1 session — re-date at entry; record actual at exit

## Goal

After this gate: an arbiter with NO observer wired still ranks against the local player's view
(`UCk_Utils_Camera_UE::TryGet_LocalViewInfo` — discharging the standing "upstream TryGet_LocalCamera
into CkCamera" chip), and the mechanism has framework-side behavior coverage: a CkTests gym the
maintainer can eyeball (promote ring, crossfades, budget caps, locks, hide, suspend/resume) plus
AS autotests over the flip lifecycle.

## Entry criteria (pre-flight)

- [x] Gate 1 exit re-verified: build green at 368651f5e, 8/8 registrations
- [x] Signals confirmed synchronous (Gate 1 exit record)

## Work items

1. **`TryGet_LocalViewInfo` (CkCamera)** — first locally-player-controlled camera entity's
   composed `FMinimalViewInfo` (fragment authoritative, PCM one frame stale); PCM fallback with
   its real `GetFOVAngle()` (not BB's assumed 90°); false on dedicated server/editor worlds.
   Patterns: `Get_ViewInfo` (fragment read), `UCk_Utils_Game_UE::Get_PrimaryPlayerController`
   (null-safe PC resolve), `Get_IsEntityLocallyControlled_ByPlayer` (BB's locality filter,
   already framework).
2. **Arbiter fallback wiring** — observer wins; unset/uncastable → discovery; still nothing → no-op.
3. **CkTests coverage** (own submodule; ck-tests skill loaded at entry): gym station + AS
   autotests (flip lifecycle: acquire→promote→fade→demote→release, hidden round-trip, locks,
   suspend/resume, signal order). NOT run by the agent (standing directive) — authored for the
   maintainer.

## Expected observations at the gate — and what to do on each branch

| I will run | I expect to observe | If instead I see | Prewritten response |
|---|---|---|---|
| toolbox `--build` | green (CkCamera + CkVisualLod recompile) | errors | fix |
| [EDITOR-VERIFY] (maintainer): gym station in PIE | walk toward crowd → nearest in-view members crossfade to SKMC; walk away → dissolve back; budget respected | pops / stuck fades / wrong counts | report verbatim; agent fixes |
| [MAINTAINER-RUN] `--test --test-pattern VisualLod --discover-fresh` | ranking specs + new AS autotests pass (new tests need --discover-fresh + the relink) | failures | agent fixes from pasted output |

## Exit criteria — ALL items land in the SAME commit as the last work item

- [ ] Build green; [EDITOR-VERIFY]/[MAINTAINER-RUN] lists current in PROGRESS.md
- [ ] The TryGet_LocalCamera memory chip noted as discharged in PROGRESS.md decision log
- [ ] PLAN.md row + this Status header updated — same commit
- [ ] PROGRESS.md dated entry appended
