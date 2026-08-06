# PHASE 4 — Close-out: docs, AS verification, BB adoption slice

**Goal:** the module is documented, verified in all three environments, exercised once from real
game code, and the campaign is handed back for Fable audit + Adam review/push.

## Entry criteria

Phases 0-3 exit criteria hold. Skills: `ck-change-control`, `ck-angelscript-interop`.

## Steps

### 4.1 Docs
- `Source/CkGameSettings/Claude.md` — purpose / depends-on / used-by / key API / the
  Provider-vs-External and orphan-vs-deferred distinctions / anti-patterns (no GConfig; don't
  conflate the late cases; packs are reference consumers; widget layer optional) / see-also
  (design doc, CkLoadingScreen, CkInput, CkCVar). Mirror the CkLoadingScreen Claude.md structure.
- `Source/CLAUDE.md`: add the tier-table row (T4, full dep list with widget-only annotations
  matching Build.cs) and the "I need to…" row ("user-facing game settings (registry, persistence,
  packs, optional menu widgets) | `CkGameSettings`").
- Design doc: append a short "As-built deviations" section listing every PROGRESS-recorded
  divergence (or "none").

### 4.2 AngelScript verification (three-environment check, non-negotiable #4)
1. Run any toolbox `--test` (boots an editor → regenerates `Script/Generated/`).
2. Verify `Script/Generated/utils_game_settings.as` exists and contains the full public surface
   (`rg --no-ignore -n "Request_RegisterSetting|Get_SettingValue_Float|Request_BeginPendingChanges" ../../Script/Generated/utils_game_settings.as` from plugin root — Grep tool is blind under
   `Script/`, use `rg --no-ignore` or the Bash tool).
   Missing functions → a signature violated a PROMPT fence (delegate position, overload,
   InternalUseOnly) → fix and re-run. Missing FILE → the BFL name/suffix is wrong → STOP, blocker.
3. Write a minimal AS AutoTest in CkTests that registers one setting, sets it, reads it back via
   `utils_game_settings` — this is the AS-environment proof: `...GameSettings_AS_RegisterSetRead`.
4. Blueprint check is structural (UFUNCTIONs with correct Category/DisplayName exist — spot-check
   three in the generated editor is `[EDITOR-VERIFY]`; headless proof is the successful UHT pass).

### 4.3 BB adoption slice (the one cross-repo step — keep it bounded)
In the BB superproject (`Script/` game code, on a BB branch named `feature/game-settings-adoption`):
register a **slice** — 3-5 settings that mirror BB 5.5 `bb.*` CVar settings (e.g.
`bb.mouse.sensitivity` → `input.mouse.sensitivity` Float with CVar binding to the existing BB CVar
name if it exists in current BB, else handler-bound) from an AS subsystem/entity script at startup,
via `utils_game_settings`. Acceptance: a BB AutoTest or existing-suite delta-zero + the settings
visible in the Phase-3 gym when run in BB. If current BB has no equivalent CVars yet, register
pure stored-value settings and note it — the point is proving the AS authoring path from game
code, not migrating BB's whole settings surface (that is post-campaign work).

### 4.4 Final gate + handoff
1. Run everything in `VALIDATION.md` §Automated. Record actual outputs there or in PROGRESS.
2. Full suite `--test --no-live` (the gate of record) — delta-zero vs the Phase-0 baseline, stated
   as "baseline N failing {names} → still N {names}".
3. Comment audit across the whole campaign diff (`git diff dev...feature/game-settings`) — delete
   breadcrumbs/what-comments.
4. Commit; **no push anywhere**. List in PROGRESS: every branch (CkFoundation, CkTests, BB), tip
   SHAs, and the exact `[EDITOR-VERIFY]` list awaiting the human.
5. End with PROGRESS §Handoff filled: "ready for Fable audit (/audit-package) + Adam review".

## Fences

- No new features in this phase. Anything discovered missing → PROGRESS blockers/follow-ups, not code.
- BB adoption stays ≤5 settings; do not migrate the full 27.
- Do not bump submodule pointers in BB; do not push.

## Exit criteria

1. VALIDATION.md §Automated fully executed with recorded outputs; delta-zero stated with names.
2. `utils_game_settings.as` verified + AS AutoTest green.
3. Claude.md + both Source/CLAUDE.md rows exist; design doc "As-built deviations" appended.
4. All branches committed, unpushed, enumerated in PROGRESS with SHAs.
