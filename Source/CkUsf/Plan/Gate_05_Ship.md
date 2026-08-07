# Gate 5 — Ship: project defaults, CVars, gyms, docs, audit

> **Status:** ⏳ Pending
> **Depends on:** Gate 4 ✅
> **Estimate:** 1–2 sessions — re-date at entry; record actual at exit

## Goal

After this gate: a project can declare a default preset per effect that applies on world init;
debug CVars exist for all three effects; three gym stations let a human tune and A/B every
feature; the permanent documentation lives in CkUsf/Claude.md; the campaign scaffolding is ready
for deletion.

## Entry criteria

- [ ] Gates 2–4 exit checklists re-verified on current HEAD (hash into PROGRESS.md)
- [ ] Baseline test counts re-captured
- [ ] Staleness sweep over the whole campaign doc set (ck-methodology §7)

## Work items

1. **Project defaults** — `UCk_Usf_Stylize_ProjectSettings` (DeveloperSettings; config=Game;
   pattern: existing `UCk_*_Settings` classes — verify the house base class at entry): optional
   default preset soft-ref per effect (`TSoftObjectPtr<UCkUsf_*Preset>`; None = off). Each
   subsystem applies its default on initialization; `Request_ResetToDefaults` re-reads it.
   Soft-refs resolved via the deferred-config discipline (CkCore IO recipe) if AS-authored preset
   assets are referenced.
2. **CVars** (CkCVar pattern): `ck.Usf.{HandDrawn,CelShade,ScreenDither}.{Enabled,Debug}` —
   Enabled `-1/0/1` (−1 = settings-driven, matching the yShade convention we documented) and
   Debug mode override. Subsystems consult them on their param-write path.
3. **Gym polish sweep** — the three gyms shipped inside Gates 2–4; this gate only verifies them
   post-integration: all three appear in the cycler (registry rows alphabetical), stations render
   with correct text, Exec commands work, and cross-effect stacking is demonstrated (one station
   note per gym: cel + dither enabled together is the classic combo; hand-drawn + dither is
   legal; hand-drawn + cel is unsupported-but-harmless — document the observed result).
4. **AS/BP sweep** — regenerate `utils_*.as`; grep `Script/` trees for the new APIs compiling
   (per the BusterBlock sweep-scope lesson: CkGameplayDebugger C++ + the three .as trees).
5. **Docs** — CkUsf/Claude.md gains the permanent Stylize contract (three effects, settings flow,
   stencil contract incl. outline-range disjointness, limitations: no temporal stabilization, cel
   is deferred-only, illumination approximation); `Source/CLAUDE.md` decision-tree rows updated;
   follow-ups recorded (renderer-module cel-pattern sync for ISM/ISKM; per-preset fill textures
   analogue if wanted).
6. **Comment audit** (mandatory closing step, root doctrine) over the whole campaign diff.
7. **Full regate** — entire CkUsf test suite + generation of all looks on the final binary
   (stale-green trap: re-run after the LAST edit).

## Expected observations at the gate

| I will run | I expect to observe | If instead I see | Prewritten response |
|---|---|---|---|
| Full suite on final binary | All green vs recorded baseline | Any red | Own-change isolation; fix before any ship claim |
| [EDITOR-VERIFY] fresh PIE with a default preset configured | Effect active with zero game code | Inactive | Subsystem init order vs settings load — check `Initialize` timing against config availability |
| [EDITOR-VERIFY] all three gym stations | Every group's controls visibly act; debug modes render | Dead control | Param name drift between struct → MID write → .ush — the validator only covers the look asset, not the subsystem's write table; fix the write table |

## Exit criteria — ALL items land in the SAME commit as the last work item

- [ ] Observations confirmed; `[EDITOR-VERIFY]` steps listed; evidence in PROGRESS.md
- [ ] `ck-change-control` done-checklist at campaign scope
- [ ] PLAN.md row + this header updated, same commit
- [ ] CkUsf/Claude.md is the complete permanent record; campaign docs marked ready-for-deletion
      in PROGRESS.md (deletion itself is the maintainer's call)
- [ ] PROGRESS.md final entry: confirmed/inferred split for the whole campaign + the ordered
      list of maintainer-only verification steps
