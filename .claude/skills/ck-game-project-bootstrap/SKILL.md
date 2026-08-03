---
name: ck-game-project-bootstrap
description: 'Use when bootstrapping a new CkFoundation game repo: engine registration, Git and submodules, project targets, config, script generation, skills, and first entity.'
---

# ck-game-project-bootstrap — empty repo to first entity on screen

## Overview

This is the "first week" runbook for a new CkFoundation game: obtain and register the
UnrealEngine-Angelscript engine fork, lay down the repo skeleton, add the framework submodules,
write the (tiny) mandatory C++ layer, wire the config files, boot once, and put one entity on
screen. Everything is manual wiring by design — there is no template repo (maintainer ruling,
2026-07-03).

Conventions used throughout: `<Game>` = your project name (BusterBlock uses `BusterBlock`),
`<Prefix>` = your class prefix (`Bb_` in BusterBlock, `Vns_` in Venus), `<prefix>` = your
lowercase asset/log namespace (`bb`, `vns`). All facts marked "(verified 2026-07-03)" were read
first-hand from the BusterBlock corpus at commit `52a75e13d` / CkFoundation `1bdc3f3b2`.

**Adopting the framework in an EXISTING project?** Same runbook minus Step 2's `git init` — start
at Step 1 (engine), merge Step 2's `.gitattributes`/`.gitignore` lines into your existing files,
then continue from Step 3 unchanged.

## When NOT to use this skill

| Task | Load instead |
|---|---|
| Build/environment breakage after setup (missing modules, LNK1104, stale Binaries, GUID troubleshooting depth) | `ck-build-and-env` |
| Cook/stage/package flow and its traps (AS staging, two-pass self-heal cook, never-cook lists) | `ck-game-build-and-cook` |
| Writing an actual gameplay feature past the hello-world below | `ck-game-feature-recipe` |
| Test-layer choice, coverage norms, running suites | `ck-game-testing-discipline` (+ `ck-tests-authoring-and-running` for mechanics) |
| AS language, hot-reload loop, silent AS failures | `ck-game-angelscript-gameplay` (+ `Plugins/CkFoundation/Script/CLAUDE.md`) |
| `Script/Generated/` internals, dynamic-handle registry, self-heal | `ck-angelscript-interop` |

---


## Reference files — load only what the task needs

Section numbers cited elsewhere in this skill point into these files.

| Topic | Read |
|---|---|
| Steps 1-9 — engine through first entity | `references/bootstrap-steps.md` |

## Step 10 — Verification gates for the week

Capture these as your baseline before feature work starts (record the actual outputs — "no
regressions" later means a diff against these numbers):

| # | Gate | Command / evidence |
|---|---|---|
| 1 | C++ compiles | Step 6's `Build.bat` exits 0 |
| 2 | AS compiles clean | Headless boot (Step 6) exits; fresh `Saved/Logs/<Game>.log` has zero `Angelscript: Error` lines (`rg --no-ignore "Angelscript: Error" Saved/Logs/<Game>.log` → empty) |
| 3 | One AutoTest green | Author a minimal `UCk_AutoTest_Base` subclass + a `UCkAutoTestMapConfig` for your test map — **inside the `<Game>Tests` plugin (Step 3), not project `Script/`** — and run it headless. Authoring/invocation mechanics: `ck-tests-authoring-and-running`; where game tests live, coverage norms, and evidence standards: `ck-game-testing-discipline` |

A green Gate 2 from a boot that predates your latest edit is not evidence — re-run on the final
binary, read the fresh log (logs rotate per boot).

## Common mistakes

- Copying BusterBlock's `DefaultCkFoundation.ini` wholesale — you inherit the dead
  `_ProcessorInjectors` key, BB's nav filters, and `/Game/BusterBlock` scan paths (Step 5).
- Listing CkFoundation/CkTests/CkGameplayDebugger in the `.uproject` `Plugins` array — harmless
  but wrong mental model; they self-enable via `EnabledByDefault` (Step 3).
- Writing C++ GameMode/GameInstance/GameEngine subclasses "because it's an Unreal project" —
  none are needed; GameMode is a Blueprint concern (Step 4).
- Hand-copying BB's three `Ck*` collision-profile ini lines instead of just the two channel
  lines — the profiles self-register (Step 5).
- Committing `*_EntitySpawnParams.as` or letting `DynamicHandleTypes.json` be treated as text —
  the two highest-torque repo-hygiene mistakes (Step 2); each has caused a real incident
  (cook wedge; "not a data type" cascade).
- Skipping the session restart after the first `sync-skills.ps1` run, then concluding the skills
  don't work (Step 7).
- Starting the game module's Build.cs from BB's 45-module list instead of the 3-module core
  (Step 4) — pure drag; add modules when your C++ includes them.
- Forgetting that every dev machine must register the engine GUID (Step 1, mechanism A) — the
  "Select Unreal Engine Version" prompt on a teammate's machine is this, not a broken checkout.

## Provenance and maintenance

Authored 2026-07-03 (ck-game consumer-skill campaign) against BusterBlock `52a75e13d` +
CkFoundation `1bdc3f3b2`, with Venus recon as the second-consumer calibration. Corpus facts are
labeled inline; BusterBlock appears only as the verified exemplar — every command/file shape above
is genericized. Re-verification commands (cwd = a consuming project root):

- **Ck plugins self-enable:** `grep -n "EnabledByDefault" Plugins/Ck*/Ck*.uplugin` (expect `true` ×3)
- **Submodule URLs:** `git config -f .gitmodules --get-regexp 'submodule.(Plugins/Ck.*|CkAuto).url'`
- **Dead `_ProcessorInjectors` key:** `rg --no-ignore -n "_ProcessorInjectors" Plugins/CkFoundation/Source/` (expect zero hits; if a hit appears, the key came back to life — update Step 5)
- **Collision self-registration:** `Plugins/CkFoundation/Source/CkOverlapBodyEditor/CkOverlapBodyEditor_Module.cpp` (channel names scanned by name at indices 14-32; profiles added if missing)
- **Registry path resolution:** `Plugins/CkFoundation/Source/CkDynamic/Public/CkDynamic/Settings/CkDynamic_Settings.cpp:36`
- **Spawner transform fallback:** `Plugins/CkFoundation/Source/CkEntitySpawner/Public/CkEntitySpawner/CkEntitySpawner_Actor.cpp:53-56`
- **Engine association mechanics:** `CkAuto/Get-ProjectEnginePath.ps1` + `Get-ItemProperty 'HKCU:\Software\Epic Games\Unreal Engine\Builds'`
- **Missing CkTests tags DataTable (Step 5 claim):** `Get-ChildItem -Recurse Plugins/CkTests -Filter '*GameplayTags*.uasset'` (expect none; if it reappears, restore the `GameplayTagTableList` guidance)
- **sync-skills behavior:** `Plugins/CkFoundation/.claude/scripts/README.md`

Volatile line-number cites (BB `.uproject`, `.gitignore`, `DefaultEngine.ini`) drift with edits —
re-anchor on the quoted key names, not the numbers.
