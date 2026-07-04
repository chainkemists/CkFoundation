---
name: ck-build-and-env
description: Use when setting up the engine + Ck plugins from scratch or fixing environment breakage — "Select Unreal Engine Version" prompt, unregistered EngineAssociation GUID (HKCU registry Builds key), "The following modules are missing or built with a different engine version", LNK1104 DLL locks, stale plugin Binaries/Intermediate after a branch switch, WITH_ANGELSCRIPT_CK questions, CkModuleRules / new-module Build.cs, per-config CK_* defines, GenerateProjectFiles / Build.bat / headless -Cmd.exe -nullrhi boots, unity-build symbol collisions, CRLF rg mismatches. Not for running tests (ck-tests-authoring-and-running) or compile-error triage past the environment (ck-debugging-playbook).
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

## 1. The engine — UnrealEngine-Angelscript 5.7.4, source build

The suite targets the UnrealEngine-Angelscript fork (Hazelight lineage — AngelScript is a
first-class consumer of every public API; identity facts: root CLAUDE.md). NOT stock UE, NOT a
Launcher binary — a source checkout you register yourself.

Facts read from the authoring machine's checkout `D:/Repos/UnrealEngineAngelscript` (2026-07-02):

| Fact | Value | Evidence |
|---|---|---|
| Version | **5.7.4** (Major 5, Minor 7, Patch 4; CompatibleChangelist 47537391; BranchName "UE5") | `Engine/Build/Build.version` |
| Working branch | `main-ck` (HEAD `4176d9c5b90f` at verification) | `git branch --show-current` |
| Remote `origin` | `github.com/CommitAndChill/UnrealEngine-Angelscript.git` | `git remote -v` |
| Remote `origin-ck` | `github.com/chainkemists/UnrealEngine-internal.git` (carries `main-ck`) | `git remote -v` |
| Remote `origin-hazelight` | `github.com/Hazelight/UnrealEngine-Angelscript.git` (upstream) | `git remote -v` |

`main-ck` exists on both `origin` and `origin-ck`. Which remote a fresh machine should clone is an
access/policy question for the maintainer — the URLs above are what the working checkout actually
points at. (Local remote URLs may embed personal access tokens — never paste `git remote -v`
output into docs or commits unstripped.)

### Registering a source build (Windows)

A `.uproject` names its engine by GUID: `"EngineAssociation": "{...}"`. Windows resolves that GUID
through the registry key `HKCU\Software\Epic Games\Unreal Engine\Builds` — one `REG_SZ` value per
build, **value name = the GUID string (braces included), value data = engine root path**. Live
entry read 2026-07-02:

```
{21E60FAC-48AD-69BF-42B6-E98C333A2E90}  =  D:/Repos/UnrealEngineAngelscript
```

Forward slashes in the path are valid. The GUID is arbitrary — association works by exact string
match between the `.uproject`'s `EngineAssociation` and a value name.

### The unregistered-GUID symptom

**Symptom.** Double-clicking the `.uproject` shows the "Select Unreal Engine Version" picker;
right-click → Generate/Switch version fails; any script that resolves the engine path from the
GUID fails. **The engine checkout itself is fine** — invoking `<ENGINE>\Engine\Build\BatchFiles\Build.bat`
by explicit path never consults the registry.

Live instance on the authoring machine (2026-07-02): `BusterBlock.uproject` has
`EngineAssociation = {22D2B5AE-4AE5-C485-F291-F79F407369F4}`, but the registry contains only the
`{21E60FAC-...}` value above — the project's GUID lookup fails even though the engine IS
registered (under a different name).

**Fix** (PowerShell; local-only, touches no version-controlled file):

```powershell
New-ItemProperty -Path 'HKCU:\Software\Epic Games\Unreal Engine\Builds' `
    -Name '{22D2B5AE-4AE5-C485-F291-F79F407369F4}' `
    -Value 'D:/Repos/UnrealEngineAngelscript' -PropertyType String
```

Name = your `.uproject`'s `EngineAssociation` verbatim; value = your engine root. The alternative —
editing `EngineAssociation` to an already-registered GUID — changes a shared, version-controlled
file for every teammate. Prefer the registry fix.

## 2. Plugin placement — git submodules under `<Project>/Plugins/`

The three Ck plugins live as submodules of the host project (URLs verified from a host
`.gitmodules`, 2026-07-02):

| Path | URL |
|---|---|
| `Plugins/CkFoundation` | `https://github.com/chainkemists/CkFoundation.git` |
| `Plugins/CkGameplayDebugger` | `https://github.com/chainkemists/CkGameplayDebugger.git` |
| `Plugins/CkTests` | `https://github.com/chainkemists/CkTests.git` |

New host project (Git Bash, cwd = project root):

```bash
git submodule add https://github.com/chainkemists/CkFoundation.git Plugins/CkFoundation
git submodule add https://github.com/chainkemists/CkGameplayDebugger.git Plugins/CkGameplayDebugger
git submodule add https://github.com/chainkemists/CkTests.git Plugins/CkTests
```

Fresh clone of an existing host:

```bash
git submodule update --init --recursive Plugins/CkFoundation Plugins/CkGameplayDebugger Plugins/CkTests
```

### AngelScript is optional at build time

- `CkFoundation.uplugin` declares the dependency `{"Name": "Angelscript", "Enabled": true, "Optional": true}`
  — the plugin loads on an engine without the AS plugin.
- `CkModuleRules` auto-detects it: `IsAngelscriptPluginEnabled` probes
  `<ENGINE>/Engine/Plugins/Angelscript/Angelscript.uplugin`
  (`Source/CkBuildConfig/CkBuildConfig.Build.cs:16-17`), then checks
  `Plugins.IsPluginEnabledForTarget(...)` (`:33-39`). Present + enabled → adds the
  `AngelscriptCode` dependency and `WITH_ANGELSCRIPT_CK=1`; otherwise `WITH_ANGELSCRIPT_CK=0`
  (`:53-61`). No manual switch exists — the engine checkout decides.
- **With** the engine AS plugin: full `Script/` layer, generated bindings, AS test harnesses.
  **Without**: C++/BP surfaces still build and run; everything guarded by `WITH_ANGELSCRIPT_CK`
  compiles out. Code touching AS bindings must compile both ways (root CLAUDE.md, Identity).

## 3. `CkModuleRules` — the shared Build.cs spine

`Source/CkBuildConfig/CkBuildConfig.Build.cs` defines `public class CkModuleRules : ModuleRules`.
Every Ck module inherits it except the two exceptions below. `CkBuildConfig` itself is NOT listed
in `CkFoundation.uplugin` — it exists to host this class; the defines it emits are consumed in
`CkCore` (e.g. `Source/CkCore/Public/CkCore/Ensure/CkEnsure.h`).

Constructor (`CkBuildConfig.Build.cs:201-222`):

| Setting | Value | Line |
|---|---|---|
| `bUseUnity` | ctor param `UseUnityBuild`, **default true**; no subclass passes `false` today | `:203` |
| `CppStandard` | `CppStandardVersion.Cpp20` | `:204` |
| `PCHUsage` | `UseExplicitOrSharedPCHs` | `:205` |
| `SetupIrisSupport(Target)` | UBT-provided Iris replication setup (engine `UnrealBuildTool/Configuration/ModuleRules.cs`) | `:207` |
| Base public deps | `ApplicationCore`, `Core`, `CoreUObject`, `Engine` | `:213-219` |
| `SetBuildConfiguration(Target)` | the define matrix below | `:221` |

Unity-on-by-default is why file-local `static` helpers and anonymous namespaces are banned —
concatenated TUs collide (style rule + named-namespace convention: root CLAUDE.md; failure mode:
trap T5 below).

**Always set, every configuration:** `CK_FORMAT_FORCE_DETAILED=0` (`:50`),
`CK_DEBUG_NAME_FORCE_VERBOSE=0` (`:51`), `CK_BUILD_LOGGING=1` (`:63` — the `EnableLogging` field
is effectively constant: `SetBuildConfiguration` runs inside the base ctor, before any derived
ctor body could flip it, and no module touches it), plus `WITH_ANGELSCRIPT_CK` per §2.

### Per-configuration define matrix (`SetBuildConfiguration`, `:65-198`)

**The full 17-define × 5-config matrix is owned by `ck-macros-and-codegen` §2.4** (with the
decision notes) — load it there; this skill keeps only the environment facts you must not forget:

- `CK_DISABLE_ENSURE_CHECKS=0` in EVERY configuration **including Shipping** — Ck ensures stay
  active in Shipping by default (root CLAUDE.md non-negotiable #2 rests on this line).
- **Development-Editor ≠ Development-game** (split on `Target.bBuildEditor`, `:107`; four defines
  differ) — PIE and a packaged Development build legitimately behave differently. Check the matrix
  before treating a packaged-only difference as a bug (then load `ck-debugging-playbook`).
- These are `PublicDefinitions` — they propagate to every module that depends on a Ck module.

### The two modules that do NOT inherit `CkModuleRules`

Don't add a third without cause (module-authoring rule: `Source/CLAUDE.md`).

| Module | Why plain `ModuleRules` |
|---|---|
| `CkThirdParty` | Vendored libraries (EnTT 3.16.0, JoltPhysics, fmt, cleantype, ctti, delegate, bitwise-enum). `bUseUnity = false` (`CkThirdParty.Build.cs:41`); Jolt defines `JPH_ENABLE_ASSERTS` + `JPH_DEBUG_RENDERER` always, `JPH_SHARED_LIBRARY`/`JPH_BUILD_SHARED_LIBRARY` for non-Server targets (`:26-39`) |
| `CkIskmRendererVF` | Its own header comment states it: "NO Ck deps, NOT CkModuleRules — CkModuleRules pulls AngelscriptCode/ApplicationCore which cannot load at PostConfigInit" (`CkIskmRendererVF.Build.cs:3-5`). Engine-only deps (`RenderCore`, `RHI`; private `Renderer`, `Projects`) so the vertex factory registers before the engine seals its factory list |

## 4. Generating project files, building, headless boot — generic shapes

PowerShell. `<ENGINE>` = engine root; `<UPROJECT>` = absolute path to the host `.uproject`.

**Generate project files.** The root `<ENGINE>\GenerateProjectFiles.bat` forwards to
`Engine\Build\BatchFiles\GenerateProjectFiles.bat`, which runs
`dotnet UnrealBuildTool.dll -ProjectFiles %*` — every argument you pass goes straight to UBT:

```powershell
& "<ENGINE>\GenerateProjectFiles.bat" -Project="<UPROJECT>" -Game
```

(No args = full engine solution. Args verified in this engine's UBT: `-Game` at
`ProjectFileGenerator.cs` `case "-GAME":`; `-Project=<path>` — or a bare `.uproject` path —
parsed by `System/Utils.cs::TryParseProjectFileArgument`. `-Engine` is an explicit no-op in this
UBT, "no longer needed as the engine module is always included now" — don't cargo-cult it.)

**Build.** `Build.bat`'s own header documents the shape: `%1` target, `%2` platform, `%3`
configuration, remaining args passed to UBT:

```powershell
& "<ENGINE>\Engine\Build\BatchFiles\Build.bat" <Project>Editor Win64 Development `
    -Project="<UPROJECT>" -WaitMutex
```

`-WaitMutex` queues behind a concurrent UBT instead of failing (verified UBT option:
`UnrealBuildTool.cs`, `[CommandLine(Prefix = "-WaitMutex", ...)]`).

**Where the editor binary lands — check the host's target file.** With
`BuildEnvironment = TargetBuildEnvironment.Unique` in `<Project>Editor.Target.cs`, output is
`<ProjectRoot>/Binaries/Win64/<Project>Editor.exe` + `<Project>Editor-Cmd.exe` (verified 2026-07-02:
the BusterBlock host uses Unique and has project-named binaries; its engine checkout has NO
`UnrealEditor.exe`/`UnrealEditor-Cmd.exe` under `Engine/Binaries/Win64/` — only DebugGame-suffixed
variants). With the default shared build environment expect
`<ENGINE>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe`. If one location is empty, check the other
before concluding the build failed.

**Headless editor boot** (environment sanity + AS compile verification — the AS plugin compiles
all of `Script/` during startup):

```powershell
& "<ProjectRoot>\Binaries\Win64\<Project>Editor-Cmd.exe" "<UPROJECT>" `
    -skipcompile -unattended -nosplash -nullrhi -ExecCmds="Quit"
```

- `-skipcompile` — the boot never invokes UBT (traps T2/T3 below explain why this is load-bearing).
  Build explicitly with `Build.bat` first; a boot is a boot, not a build.
- `-nullrhi` — no GPU/swapchain; runs on a headless agent.
- Verdict lives in `<ProjectRoot>/Saved/Logs/<Project>.log` (rotates per boot — read the fresh
  one). AS error grammar and what counts as pre-existing noise: `ck-angelscript-interop`.
  Anything that runs tests: `ck-tests-authoring-and-running`.

**Labeled host example:** BusterBlock drives builds/tests via its `CkAuto/UnrealToolbox.exe` —
superproject-specific, see that repo's docs.

## 5. Adding a new module

1. **Build.cs** — `Source/<M>/<M>.Build.cs` inheriting `CkModuleRules`. Canonical minimal example,
   copied verbatim from `Source/CkTimer/CkTimer.Build.cs` (C#):

```csharp
using System.IO;
using UnrealBuildTool;

public class CkTimer : CkModuleRules
{
    public CkTimer(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateIncludePaths.AddRange(new string[] {
            // ... add other private include paths required here ...
        });

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",

            "CkCore",
            "CkEcs",
            "CkEcsExt",
            "CkLabel",
            "CkLog",

            "CkProfile",
            "CkRecord",
        });
    }
}
```

2. **uplugin entry** — add to `CkFoundation.uplugin` `Modules` with the standard shape
   (verified against existing entries): `"Type"` = `Runtime` (or `UncookedOnly`/`Editor` for
   Tier-5), `"LoadingPhase": "Default"`, `"WhitelistPlatforms": ["Win64", "Mac", "Linux"]`.
   **`Default` unless you can justify otherwise** (`Source/CLAUDE.md`) — only 3 of 99 modules
   deviate today: `CkAngelscriptGenerator` + `CkEditorStyle` are `PreDefault` (their
   registrations — AS generator plumbing, shared editor style — must exist before Default-phase
   modules load; role-based inference), and `CkIskmRendererVF` is `PostConfigInit` (verified
   in-code reason: vertex-factory registration before the engine seals the list, §3).
3. **Tier discipline** — deps point to same-or-lower tiers only, never up; runtime never depends
   on Tier-5 editor modules; editor-only deps go inside `if (Target.bBuildEditor)`. Tier table and
   authoring rules: `Source/CLAUDE.md`.
4. **Docs** — ship a `Claude.md` with the module and add its tier-table row (`Source/CLAUDE.md`).
5. **Regenerate project files** (§4) so the module appears in the solution; build; then follow
   `ck-change-control` for what "done" requires.

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
