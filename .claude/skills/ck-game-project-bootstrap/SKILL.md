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

## Step 1 — Engine: obtain and register the UnrealEngine-Angelscript fork

The framework targets the UnrealEngine-Angelscript source fork (Hazelight lineage), NOT stock UE
and NOT a Launcher binary. Fork identity, remotes, version (5.7.4 at last verification), and every
environment trap (DLL locks, `-skipcompile`, stale Binaries) are owned by **`ck-build-and-env`
§1 and §6** — read §1 before cloning; do not improvise remotes.

1. Clone the fork (multi-hour first build; start it early in the week):
   `git clone -b main-ck https://github.com/chainkemists/UnrealEngine-internal.git`
   (requires org access — a safe assumption per maintainer 2026-07-03; the internal repo is the
   Chainkemists downstream of the Hazelight fork. Remotes/branch depth: `ck-build-and-env` §1).
2. Run `Setup.bat` then `GenerateProjectFiles.bat` at the engine root, build the editor target
   once (shapes in `ck-build-and-env` §4).
3. Register the engine so your `.uproject` can find it. Two mechanisms — pick one:

**A. GUID + registry (per-machine — every dev machine must repeat this).** A `.uproject`'s
`"EngineAssociation": "{GUID}"` is resolved via the registry key
`HKCU\Software\Epic Games\Unreal Engine\Builds`: value name = the GUID string (braces included),
value data = engine root (mechanism verified in `CkAuto/Get-ProjectEnginePath.ps1:69-84` and
`ck-build-and-env` §1).

```powershell
# PowerShell — pick any fresh GUID once, commit it in the .uproject, register per machine:
$guid = '{' + [guid]::NewGuid().ToString().ToUpper() + '}'
New-Item -Path 'HKCU:\Software\Epic Games\Unreal Engine\Builds' -Force | Out-Null
New-ItemProperty -Path 'HKCU:\Software\Epic Games\Unreal Engine\Builds' `
    -Name $guid -Value 'D:/Repos/UnrealEngineAngelscript' -PropertyType String
$guid   # paste this into the .uproject's EngineAssociation
```

**B. Relative path (committed once, works for every machine that keeps the same disk layout).**
`EngineAssociation` also accepts a path, absolute or relative to the `.uproject` directory —
e.g. `"EngineAssociation": "../UnrealEngineAngelscript"` (path-form resolution verified in
`CkAuto/Get-ProjectEnginePath.ps1:85-92`; standard UE behavior). Trade-off: A survives arbitrary
checkout locations but is per-machine ceremony (an unregistered GUID is the #1 new-machine
symptom — fix in `ck-build-and-env` §1); B is zero-ceremony but pins the relative layout.

**Verify:**

```powershell
Get-ItemProperty 'HKCU:\Software\Epic Games\Unreal Engine\Builds'   # your GUID → engine root
# Once Step 3's CkAuto submodule exists, the canonical resolver:
& ./CkAuto/Get-ProjectEnginePath.ps1    # prints the engine root or a precise failure reason
```

## Step 2 — Repo skeleton: git init, LFS, attributes, ignores

```bash
# Git Bash, in the new project root
git init -b dev
git lfs install
```

**`.gitattributes`** — the minimal set (distilled from BusterBlock's, verified 2026-07-03). The
last line is load-bearing: `DynamicHandleTypes.json` is UTF-16; text normalization mangles its BOM
on rebase/checkout and breaks the dynamic-handle parser (project-wide "not a data type" cascade —
BB's own comment at `.gitattributes:55-58`).

```gitattributes
*.bat eol=crlf
*.sh eol=lf

[attr]lock filter=lfs diff=lfs merge=binary -text lockable
[attr]lfs filter=lfs diff=lfs merge=binary -text

*.uasset lock
*.umap lock
*.png lfs
*.fbx lfs
*.exe lfs
*.dll lfs
*.pdb lfs
*.lib lfs
*.zip lfs

# Generated UTF-16 registry — must stay binary so git never applies text/eol normalization.
Script/Generated/DynamicHandleTypes.json binary
```

(Add lfs lines for your audio/movie formats as content arrives.)

**`.gitignore`** — a standard UE ignore set (`Binaries/`, `Intermediate/`, `Saved/`,
`DerivedDataCache/`, `.vs/`, `*.sln`) PLUS the framework's AngelScript block. The split below is
exactly what BusterBlock converged on (verified `.gitignore:384-430`, incl. the committed-plugin-ESP
cook-wedge rationale in its comments):

```gitignore
# --- CkFoundation AngelScript generated files ---
# IGNORED — regenerated/self-healed every editor boot; a COMMITTED plugin
# _EntitySpawnParams.as blocks self-heal of the base accessor it subclasses.
Script/Generated/_StubRecovery_*.as
Script/Generated/_StubRecovery_*.json
Script/Generated/<Game>_EntitySpawnParams.as
Plugins/*/Script/Generated/*_EntitySpawnParams.as
Plugins/*/Script/Generated/_StubRecovery_*.as
Plugins/*/Script/Generated/_StubRecovery_*.json
Script/Binds.Cache
Script/Binds.Cache.Headers
```

**COMMIT the rest of `Script/Generated/`** — `DynamicHandleTypes.json`, `<Game>Assets.as` /
`EngineAssets.as` (asset accessor registries), and later `*_AutoTestActors.as` — the asset
registries hard-reference generated wrapper types (BB `.gitignore` comment, verified).

**The `.ignore` question (ripgrep exclusion file)** `[UNDER ADJUDICATION — see CkFoundation
.claude/reports/ADJUDICATIONS.md A7]`**.** BusterBlock keeps a repo-root `.ignore` that
hides `/Script`, `/Content`, `/CkAuto`, and plugin `Script`/`Content` dirs from ripgrep — which
means Claude-session Grep/Glob tools **silently return zero matches** for all game AngelScript
(documented trap, `ck-build-and-env` T8). Recommendation for a new project: adopt an `.ignore`
that hides only bulk binary dirs, and do NOT hide `Script/`:

```gitignore
# .ignore — ripgrep-only exclusions (NOT git); keep Script/ searchable
/Content
/Saved
/Intermediate
/DerivedDataCache
/Binaries
/Plugins/*/Content
/Plugins/*/Intermediate
/Plugins/*/Binaries
```

If you do hide `Script/` (BB's choice, made for search-noise reasons), every agent/tool session
must re-check zero-match results with `rg --no-ignore` — budget for that recurring cost.

## Step 3 — Submodules and the `.uproject`

**Minimal framework set** (paths + URLs verified from BusterBlock `.gitmodules:73-84`, 2026-07-03):

```bash
git submodule add https://github.com/chainkemists/CkFoundation.git       Plugins/CkFoundation
git submodule add https://github.com/chainkemists/CkGameplayDebugger.git Plugins/CkGameplayDebugger
git submodule add https://github.com/chainkemists/CkTests.git            Plugins/CkTests
git submodule add https://github.com/chainkemists/CkAuto.git             CkAuto
```

`CkAuto` sits at the repo root (not under `Plugins/`) — it is tooling, not a plugin: engine-path
resolver, `UnrealToolbox.exe` (build/test driver), Gauntlet runners, submodule batch scripts, and
the `Check-UnrealNotRunning.ps1` PreToolUse safety hook (inventory verified 2026-07-03).

**The Ck plugins never appear in the `.uproject`.** All three declare `"EnabledByDefault": true`
in their `.uplugin` (verified: `CkFoundation.uplugin:16`, `CkTests.uplugin:16`,
`CkDebugger.uplugin:16`) — placing the submodule under `Plugins/` is the entire integration.

**What DOES go in the `.uproject`** — your game module plus two engine/feature plugins BusterBlock
enables explicitly (verified `BusterBlock.uproject:336-348`):

```json
{
    "FileVersion": 3,
    "EngineAssociation": "{YOUR-GUID-FROM-STEP-1}",
    "Modules": [
        { "Name": "<Game>", "Type": "Runtime", "LoadingPhase": "Default" }
    ],
    "Plugins": [
        { "Name": "AngelscriptEnhancedInput", "Enabled": true },
        { "Name": "Gauntlet", "Enabled": true }
    ]
}
```

- `AngelscriptEnhancedInput` — the fork's Enhanced Input AS bindings; CkInput's mapping-context
  scan (Step 5) presumes Enhanced Input.
- `Gauntlet` — needed for the process-level Gauntlet test layer; cheap to enable now, required
  before your first `ck-game-testing-discipline` Gauntlet run.
- **Create a `<Game>Tests` editor-only plugin entry now** (even empty), with
  `"TargetAllowList": ["Editor"]` — it is where ALL game test script lives; authoring a test into
  the project `Script/` root plants a Shipping no-boot landmine (test AS inheriting CkTests bases
  that are disabled in packaged targets — full rationale: `ck-game-testing-discipline` §3.1).
  BusterBlock migrated to this shape after shipping without it (verified
  `BusterBlock.uproject:351-356` + `Source/BusterBlock/BusterBlock.Build.cs:98-102` comment);
  starting there avoids the migration.

## Step 4 — Minimal game C++ (four small files, no framework classes)

**CkFoundation requires NO custom GameEngine / GameInstance / GameMode / PlayerController C++.**
Verified 2026-07-03: BusterBlock's Config contains no engine-class overrides, and its default
GameMode is a Blueprint (`GlobalDefaultGameMode=/Game/.../InGame_GameMode_BB_BP.InGame_GameMode_BB_BP_C`,
`DefaultEngine.ini:282`). This contradicts a common assumption (and BB's own stale root CLAUDE.md,
which describes `BbGameMode`/`BbGameEngine` classes that do not exist in `Source/`). Framework
processors self-register from the plugins; your C++ layer is one near-empty primary module.

`Source/<Game>/<Game>.h`:

```cpp
#pragma once

#include "CoreMinimal.h"

#include "CkLog/CkLog_Utils.h"

<GAME>_API DECLARE_LOG_CATEGORY_EXTERN(<Game>, Log, All);

namespace <prefix>
{
    CK_DEFINE_LOG_FUNCTIONS(<Game>);
}
```

(`CK_DEFINE_LOG_FUNCTIONS` lives in `CkLog/CkLog_Utils.h:106` — BusterBlock omits the include and
leans on a shared PCH; include it explicitly.)

`Source/<Game>/<Game>.cpp`:

```cpp
#include "<Game>.h"

#include "Modules/ModuleManager.h"

class F<Game>Module : public FDefaultGameModuleImpl
{
};

IMPLEMENT_PRIMARY_GAME_MODULE(F<Game>Module, <Game>, "<Game>");

DEFINE_LOG_CATEGORY(<Game>);

namespace <prefix>
{
    CK_REGISTER_LOG_FUNCTIONS(<Game>);
}
```

(BusterBlock's version adds console-command registration and a shader-dir mount — optional,
add when needed; verified `Source/BusterBlock/BusterBlock.cpp`.)

`Source/<Game>/<Game>.Build.cs` — the load-bearing lines are `SetupIrisSupport` (pairs with
Step 5's Iris cvars) and `AngelscriptCode`:

```csharp
using UnrealBuildTool;

public class <Game> : ModuleRules
{
    public <Game>(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        SetupIrisSupport(Target, true);

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "GameplayTags",
            "NetCore",

            "AngelscriptCode",

            "CkCore",
            "CkEcs",
            "CkLog",
        });
    }
}
```

The Ck module list is à-la-carte: Build.cs deps only gate what **your C++** can `#include` — the
plugin's ~99 modules load at runtime regardless. Start with the three above and add
`Ck<Feature>` entries as your C++ (not your AS) touches them. BusterBlock lists 45
(`BusterBlock.Build.cs:41-85`) because its C++ glue accreted; that is history, not a starting
requirement. [Inference from UBT dependency semantics + BB corpus; confirmed by BB defining zero
C++ processors/fragments despite the long list.]

`Source/<Game>.Target.cs` — the CkTests exclusion is mandatory before your first Shipping/Test
package (CkTests depends on non-redistributable Developer modules; rationale comment verified
`Source/BusterBlock.Target.cs:17-27`):

```csharp
using UnrealBuildTool;

public class <Game>Target : TargetRules
{
    public <Game>Target(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;

        bWithPushModel = true;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;

        ExtraModuleNames.AddRange(new string[] { "<Game>" });

        if (Target.Configuration == UnrealTargetConfiguration.Shipping ||
            Target.Configuration == UnrealTargetConfiguration.Test)
        {
            DisablePlugins.Add("CkTests");
        }
    }
}
```

`Source/<Game>Editor.Target.cs` — `BuildEnvironment.Unique` is required (the fork/plugins change
build settings; verified `Source/BusterBlockEditor.Target.cs:15`). Consequence: your editor binary
lands at `<ProjectRoot>/Binaries/Win64/<Game>Editor.exe` / `<Game>Editor-Cmd.exe`, NOT in the
engine tree (`ck-build-and-env` §4).

```csharp
using UnrealBuildTool;

public class <Game>EditorTarget : TargetRules
{
    public <Game>EditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;

        BuildEnvironment = TargetBuildEnvironment.Unique;

        ExtraModuleNames.AddRange(new string[] { "<Game>" });
    }
}
```

## Step 5 — Config: the framework-mandated ini lines

**`Config/DefaultEngine.ini`** (all verified against BB's, 2026-07-03):

```ini
[SystemSettings]
net.SubObjects.DefaultUseSubObjectReplicationList=1
net.IsPushModelEnabled=1
net.Iris.UseIrisReplication=1

[/Script/BuildSettings.BuildSettings]
DefaultEditorTarget=<Game>Editor
DefaultGameTarget=<Game>

[/Script/Engine.CollisionProfile]
+DefaultChannelResponses=(Channel=ECC_GameTraceChannel1,DefaultResponse=ECR_Ignore,bTraceType=False,bStaticObject=False,Name="CkSensor")
+DefaultChannelResponses=(Channel=ECC_GameTraceChannel2,DefaultResponse=ECR_Ignore,bTraceType=False,bStaticObject=False,Name="CkMarker")
```

- The Iris/PushModel cvars pair with `SetupIrisSupport(Target, true)` + `bWithPushModel = true`
  from Step 4 — set all five together or none.
- Collision: you only author the two **object channels** (`CkSensor`, `CkMarker`; any free
  GameTraceChannelN works — the framework locates them by NAME, scanning indices 14-32). The three
  collision **profiles** (`CkSensor`, `CkSensor_Static`, `CkMarker`) are auto-registered by the
  framework's editor module at startup and error-notify if the channels are missing ("Expected to
  find object channel [CkSensor]. Please add it to your Project Settings > Collision") — verified
  `Plugins/CkFoundation/Source/CkOverlapBodyEditor/CkOverlapBodyEditor_Module.cpp:43-112`. Do not
  hand-copy BB's serialized profile lines; they are the editor-saved result of that registration.

**`Config/DefaultCkFoundation.ini`** — the framework's own settings category:

```ini
[/Script/CkDynamic.Ck_Dynamic_ProjectSettings_UE]
_DynamicHandleRegistryDirectory=(Path="../../Script/Generated")

[/Script/CkInput.Ck_Input_ProjectSettings_UE]
+_MappingContextScanPaths=(Path="/Game/<Game>")
```

- `_DynamicHandleRegistryDirectory` targets `<ProjectRoot>/Script/Generated` — the relative path
  resolves against the editor binary's directory (`Binaries/Win64/`), which Step 4's Unique build
  environment guarantees is project-local. Copy the value verbatim (verified: raw-path join in
  `CkDynamic/Settings/CkDynamic_Settings.cpp:36`; BB value at `DefaultCkFoundation.ini:117-118`).
- `_MappingContextScanPaths` — point at your own content root; CkInput discovers Input Mapping
  Contexts under it.
- **Do NOT copy BB's `_ProcessorInjectors` line.** It is a dead key — `UCk_Ecs_ProjectSettings_UE`
  has no matching member (verified 2026-07-03: zero `_ProcessorInjectors` hits in
  `Plugins/CkFoundation/Source/`; class at `CkEcs/Settings/CkEcs_Settings.h:59`). Processors
  register via `CK_REGISTER_PROCESSOR` / the script-processor base, not config injection.

**`Config/DefaultGameplayTags.ini`**:

```ini
[/Script/GameplayTags.GameplayTagsSettings]
ImportTagsFromConfig=True
WarnOnInvalidTags=True
```

Gameplay tags then register two ways: ini `+GameplayTagList` entries, or `UCk_GameplayTags` asset
blocks in AngelScript (the pattern both corpus games use; tags must be letter-leading — a
digit-leading tag breaks AS registration). Do NOT copy BB's
`+GameplayTagTableList=/CkTests/GameplayTags_Tests_CkDT` line — that DataTable no longer exists on
disk in CkTests (verified 2026-07-03: no `*GameplayTags*.uasset` anywhere under `Plugins/CkTests/`).

**GameMode:** leave the engine default for week one. When you need one, author a Blueprint (or an
AS GameMode subclass — the gym framework in CkTests provides AS bases). No C++ (Step 4).

Optional now, needed before first package: `DefaultGame.ini` cook wiring (AS plugin staging,
never-cook for test maps) — owned by `ck-game-build-and-cook`; one line each exists in BB's
`Config/DefaultGame.ini:124-144` when you want the reference.

## Step 6 — `Script/` root and first boot

The AS runtime compiles `<ProjectRoot>/Script/` plus every enabled plugin's `Script/` root at
editor boot (fork convention; authoritative comment block `Config/DefaultGame.ini:132-139` in BB).
Create it now:

```powershell
New-Item -ItemType Directory -Force Script, Script/Generated | Out-Null
```

Build, then boot once headless (invocation shapes and flags: `ck-build-and-env` §4):

```powershell
$engine = & ./CkAuto/Get-ProjectEnginePath.ps1
if (-not $engine) { throw "engine path unresolved — register the GUID (Step 1 / ck-build-and-env §1)" }
& "$engine\Engine\Build\BatchFiles\Build.bat" <Game>Editor Win64 Development `
    -Project="$PWD\<Game>.uproject" -WaitMutex
& ".\Binaries\Win64\<Game>Editor-Cmd.exe" "$PWD\<Game>.uproject" `
    -skipcompile -unattended -nosplash -nullrhi -ExecCmds="Quit"
```

First boot seeds `Script/Generated/`: expect `DynamicHandleTypes.json`, `<Game>Assets.as`,
`EngineAssets.as`, and `<Game>_EntitySpawnParams.as` (the exact `*Assets.as` set tracks your
content mounts — corpus observation from BB's `Script/Generated/`, verified 2026-07-03). Commit
per Step 2's policy. Generator mechanics, self-heal, and `_StubRecovery_*` behavior:
`ck-angelscript-interop` §3.

## Step 7 — sync-skills: surface the framework skills in your sessions

Claude Code only auto-discovers skills at the session project root — submodule `.claude/skills`
are invisible. Run the framework's junction script from YOUR repo root:

```powershell
pwsh Plugins/CkFoundation/.claude/scripts/sync-skills.ps1
```

**First run only: restart any open Claude Code session** — a newly created top-level
`.claude/skills` directory is only discovered at session start. Re-run (idempotent) after every
`git submodule update`; `-Prune` removes junctions whose targets vanished. Full behavior and
validation record: `Plugins/CkFoundation/.claude/scripts/README.md` (verified 2026-07-03).

## Step 8 — Project CLAUDE.md

Seed your project-root `CLAUDE.md` from the framework's project template:
`Plugins/CkFoundation/.claude/PROJECT_TEMPLATE/` — it carries the consumer-project doctrine
(framework doc pointers, test wiring, naming) so you don't hand-write it. [The template is being
authored in the same campaign as this skill — if the directory is absent, update the CkFoundation
submodule; do not reconstruct it by hand.]

## Step 9 — First entity on screen

The placeable unit in a CkFoundation game is an **EntityScript** (an AngelScript class composing
ECS fragments onto an entity), not an Actor subclass — the No-Actors doctrine (maintainer ruling;
~95% Actor replacement). The smallest visible one needs a transform + an instanced-static-mesh
proxy, using an engine mesh so zero content authoring is required.

`Script/Hello/<Prefix>_HelloEntity.as`:

```angelscript
namespace <prefix>
{
    asset Asset_HelloCube of UCk_IsmRenderer_Data
    {
        _Mesh = Cast<UStaticMesh>(utils_i_o::LoadAssetByName("/Engine/EngineMeshes/Cube.Cube",
            ECk_AssetSearchScope::Engine)._Asset);
        _Mobility = ECk_Mobility::Movable;
    }
}

class U<Prefix>_EntityScript_HelloEntity : UCk_GenericEntityScript_UE
{
    default _Replication = ECk_Replication::DoesNotReplicate;

    UPROPERTY(ExposeOnSpawn)
    FTransform SpawnTransform = FTransform::Identity;

    UFUNCTION(BlueprintOverride)
    ECk_EntityScript_ConstructionFlow DoConstruct(FCk_Handle& InHandle)
    {
        auto TransformHandle = utils_transform::Add(InHandle, SpawnTransform,
            ECk_Replication::DoesNotReplicate);

        auto IsmParams = FCk_Fragment_IsmProxy_ParamsData(<prefix>::Asset_HelloCube);
        utils_ism_proxy::Add(TransformHandle, IsmParams);

        <prefix>::Log("[<Game>] HelloEntity constructed");
        return ECk_EntityScript_ConstructionFlow::Finished;
    }
}
```

(Pattern assembled from framework-owned exemplars, verified 2026-07-03:
`Plugins/CkTests/Script/CkEntityScript/CkEntityScriptGym_Spawn.as` for the EntityScript shape,
`Plugins/CkTests/Script/CkAudio/CkAudioGym_Common.as:3-8` for the engine-mesh asset block,
`.../Advanced/CkAudioGym_Advanced_Base.as:89-107` for transform + ISM proxy composition.)

The property is named `SpawnTransform` deliberately: the level-placement actor auto-binds its own
actor transform to an FTransform property named `SpawnTransform` / `_SpawnTransform` when the
details-panel binding is left empty (verified
`CkEntitySpawner/CkEntitySpawner_Actor.h:15-23`, `.cpp:53-56`).

**Verify headlessly first** — re-run Step 6's headless boot and check the fresh log
(`Saved/Logs/<Game>.log`) for `Angelscript: Error` naming your file. A clean boot proves the
script compiles and the class registered.

**[EDITOR-VERIFY] — the visual check (no headless equivalent):**

1. Open the editor (`Binaries/Win64/<Game>Editor.exe <Game>.uproject`), open/create any level.
2. Place Actors panel → search "Ck Entity Spawner" → drag into the level
   (class `ACk_EntitySpawner_UE`, an `AInfo`; verified `CkEntitySpawner_Actor.h:53`).
3. In its details panel, set **Entity Script** to `U<Prefix>_EntityScript_HelloEntity`. The
   spawner shows an editor preview entity immediately.
4. PIE. Expected: a cube at the spawner's location, and `[<Game>] HelloEntity constructed` in the
   Output Log. The entity is inspectable in the CK ECS Debugger (`ck.EcsDebugger`).

Alternative placement for test-shaped work: a gym map (station framework spec:
`Plugins/CkTests/Script/Common/CkGym_CreationSpecification.txt`). Runtime spawning from code goes
through `utils_entity_script::Request_SpawnEntity(...)` +
`Promise_OnConstructed` — that, spawn params, replication ownership, and the
**discovery/composition-timing trap** (the #1 failure mode when features find each other too
early) are `ck-game-feature-recipe`'s territory. Stop here; write your first real feature there.

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
