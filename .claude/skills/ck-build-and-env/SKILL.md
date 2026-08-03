---
name: ck-build-and-env
description: "Use when setting up a Ck engine/plugin workspace or fixing engine association, stale binaries, DLL locks, build config, module rules, or headless boot; not for failure triage."
---

# ck-build-and-env — recreate the working environment from scratch

## Overview

This skill rebuilds the environment the Ck plugin suite compiles and boots in: the
UnrealEngine-Angelscript source engine, the three Ck plugins as host-project git submodules, the
shared `CkModuleRules` build spine, the generic UBT invocation shapes, and the known environment
traps. It is **plugin-scoped**: host projects own their own build pipelines — BusterBlock gets
exactly one labeled example line here.

## When NOT to use this skill

| Task | Load instead |
|---|---|
| Writing or invoking any test (AutoTest / Gauntlet / gym), test flags | `ck-tests-authoring-and-running` |
| Triaging a compile/UHT/linker/AS error once the environment is sane | `ck-debugging-playbook` |
| `Script/Generated/` hygiene, dynamic-handle registry, AS binding breakage | `ck-angelscript-interop` |
| Style, macros, naming, non-negotiables | root `Plugins/CkFoundation/CLAUDE.md` |


## Reference files — load only what the task needs

Section numbers cited elsewhere in this skill point into these files.

| Topic | Read |
|---|---|
| Engine, plugin placement, CkModuleRules, building, adding a module | `references/environment-setup.md` |

## 6. Traps — symptom → cause → fix

| # | Symptom | Cause | Fix |
|---|---|---|---|
| T1 | Editor boot: "The following modules are missing or built with a different engine version:" naming Ck modules right after a branch/submodule switch (exact string: engine `LaunchEngineLoop.cpp`); or compile errors referencing types the branch deleted | `<Plugin>/Binaries/` + `<Plugin>/Intermediate/` still hold the previous branch's artifacts | Delete the plugin's `Binaries/` and `Intermediate/` (per affected plugin), plus the project's `Intermediate/` if the solution is confused; regenerate project files and rebuild — all of these regenerate. Do NOT delete `Saved/` (local config/logs, not build state) or `DerivedDataCache/` (expensive shader rebuild) for this class of problem |
| T2 | `Build.bat` fails with LNK1104 "cannot open file ...dll" | A running editor has the module DLLs mapped — the linker cannot replace them | Close the editor (verify the process is gone), rebuild. [operator experience, 2026-06: an editor launched early off a premature "done" signal caused exactly this] |
| T3 | Boot reports missing modules though you switched nothing | A UBT/build process was killed mid-run: UBT deletes stale outputs before relinking, so an interrupted build leaves modules deleted-but-not-relinked | Run an explicit `Build.bat` to completion, then boot. Prevention: verification boots always pass `-skipcompile` so a canceled boot can never invoke (and orphan) UBT. [operator experience, 2026-06] |
| T4 | Two working copies sharing one engine checkout trample each other's binaries | Any UBT run writes/deletes shared `Engine/Binaries` state | Headless boots use `-skipcompile`; check for running MSBuild/UBT/editor processes before starting a build from either copy. [operator experience, 2026-06] |
| T5 | Redefinition / ODR link errors for file-local helpers that "should" be private; errors appear or vanish when unrelated files are added | Unity build (on by default, §3) concatenates TUs — anonymous-namespace and `static` file-local symbols in sibling .cpps collide; chunk membership shifts as files come and go, so the collision can surface later than the code that caused it | Wrap helpers/constants in a filename-derived named namespace (rule + naming: root CLAUDE.md). Do NOT "fix" it by passing `UseUnityBuild=false` |
| T6 | Multiline `rg` patterns match nothing; `foo$` misses lines | Every plugin file is CRLF (verified: `CkBuildConfig.Build.cs` = 230 CRLF, 0 bare LF) | Write multiline patterns as `\r?\n`; for `$` anchors use `rg --crlf` |
| T7 | Case-exact tooling misses per-module docs | File-name case is inconsistent on disk: `Source/CkGoap/CLAUDE.md` vs `Source/CkBuildConfig/Claude.md` vs plugin-root `CLAUDE.md` (verified 2026-07-02). Same hazard class: 6 of 101 module build files are lowercase `.build.cs` (e.g. `CkThirdParty.build.cs`), so `*.Build.cs` globs miss them. Harmless on Windows' case-insensitive FS; a hazard for case-sensitive filesystems/CI and exact-name filters | Search case-insensitively (`rg -i`, `-g '*[Bb]uild.cs'`; `Get-ChildItem -Filter` is already case-insensitive on Windows); don't mass-rename — churn without local benefit |
| T8 | Claude-session Grep/Glob tools return zero matches under `Script/`, `docs/`, `Content/` | Superproject `.ignore` hides those trees from ripgrep-based tools | Zero matches ⇒ re-check with `rg --no-ignore --files` or `Get-ChildItem` (root CLAUDE.md, Provenance) |

## Common mistakes

- Looking for `UnrealEditor-Cmd.exe` in `Engine/Binaries/Win64/` when the host target uses
  `TargetBuildEnvironment.Unique` — the editor binary is project-named in the project's `Binaries/`.
- "Fixing" an unregistered GUID by editing `EngineAssociation` — that edits a shared file; add the
  local registry value instead (§1).
- Deleting `DerivedDataCache/` or `Saved/` to cure a missing-modules error — wrong artifacts, real cost.
- Letting a headless boot compile (omitting `-skipcompile`) on a machine where anything else might
  touch the same binaries.
- Treating the `WITH_ANGELSCRIPT_CK=0` path as dead code — the uplugin dependency is Optional and
  both sides must compile.
- Assuming PIE behavior == packaged-Development behavior — four `CK_*` defines differ (§3 matrix).

## Provenance and maintenance

Authored 2026-07-02 (Phase-2 skills campaign). Every volatile fact above was verified first-hand on
that date. Re-verification commands (cwd `Plugins/CkFoundation` unless stated):

- **Engine version:** `Get-Content D:/Repos/UnrealEngineAngelscript/Engine/Build/Build.version`
- **Engine remotes/branch:** `git -C D:/Repos/UnrealEngineAngelscript remote -v` and
  `git -C D:/Repos/UnrealEngineAngelscript branch --show-current` (strip embedded tokens before
  quoting anywhere)
- **Registered builds:** `Get-ItemProperty 'HKCU:\Software\Epic Games\Unreal Engine\Builds'`
- **Host association GUID:** `EngineAssociation` field in the host `.uproject`
- **Submodule URLs:** `git config -f <host>/.gitmodules --get-regexp 'submodule.Plugins/Ck.*url'`
- **Angelscript-Optional dep + module count/types/phases:** parse `CkFoundation.uplugin`
  (99 modules: 71 Runtime / 25 UncookedOnly / 3 Editor; 3 non-Default phases as of 2026-07-02)
- **CkModuleRules facts + define matrix:** re-read `Source/CkBuildConfig/CkBuildConfig.Build.cs`
  (cited line numbers drift with any edit — re-anchor on `SetBuildConfiguration`)
- **Non-inheriting modules:** `rg --no-ignore -n ': ModuleRules' Source -g '*[Bb]uild.cs'`
  (expect exactly 3 hits: `CkThirdParty.build.cs` — lowercase `b`; 6 of the 101 module build files
  use lowercase `.build.cs`, so a case-exact `*.Build.cs` glob misses them — plus
  `CkIskmRendererVF.Build.cs`, and CkBuildConfig's own `public class CkModuleRules : ModuleRules`
  base-class definition line)
- **Vestigial ability define:** `rg --no-ignore -l 'CK_DISABLE_ABILITY_SCRIPT_DEBUGGING' Source/`
  (expect CkBuildConfig.Build.cs only)
- **Batch-file arg shapes:** headers of `<ENGINE>/GenerateProjectFiles.bat` and
  `<ENGINE>/Engine/Build/BatchFiles/Build.bat`
- **Editor binary location:** `Get-ChildItem <ProjectRoot>/Binaries/Win64/*Editor*.exe` and
  `BuildEnvironment` in `Source/<Project>Editor.Target.cs`
- **Missing-modules string:** `rg -l 'built with a different engine version' <ENGINE>/Engine/Source/Runtime/Launch/`
