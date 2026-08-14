---
name: ck-angelscript-interop
description: "Use when exposing CkFoundation APIs to AngelScript, debugging ScriptMixin or generated bindings, or repairing AS codegen; not for AS syntax, tests, or build setup."
---

# ck-angelscript-interop

## Overview

CkFoundation's AngelScript surface is mostly **machine-made**: the engine fork auto-binds native
UFUNCTIONs, `ScriptMixin` turns statics into handle member-methods, and `CkAngelscriptGenerator`
emits the `Script/Generated/` accessor layer at boot. Because so much is generated and hot-reloaded
by file mtime, the failure modes are *silent or misleading* rather than loud. This skill covers the
binding mechanics, the silent-breakage catalog, generator/self-heal operations, and the
three-environment verification recipe (root doctrine non-negotiable #4: C++, Blueprint, AND AS).

Jargon used once and throughout: **AS** = AngelScript (Hazelight UnrealEngine-Angelscript fork);
**handle** = `FCk_Handle`, the validated entity reference (typesafe subtypes `FCk_Handle_<Feature>`);
**BFL** = BlueprintFunctionLibrary; **utils class** = `UCk_Utils_<Feature>_UE`, a feature's only
public API surface; **engine plugin** = `<EngineDir>/Engine/Plugins/Angelscript` (on this machine
`D:/Repos/UnrealEngineAngelscript/Engine/Plugins/Angelscript`). Engine file citations below are
relative to that plugin's `Source/AngelscriptCode/`. Line numbers verified 2026-07-02.

## When NOT to use this skill

| You actually want | Go to |
|---|---|
| AS language deltas from C++ (no lambdas, `float` is 64-bit, const rules, `asset ... of ...`) | `Plugins/CkFoundation/Script/CLAUDE.md` |
| Write/run gyms, AutoTests, Gauntlet | `ck-tests-authoring-and-running` skill (CkTests) |
| Engine/plugin setup, build failures, UHT/linker errors | `ck-build-and-env`, `ck-debugging-playbook` |
| `CK_` macro shapes, add a fragment/handle/request | `ck-macros-and-codegen` |
| Style, naming, non-negotiables | root `Plugins/CkFoundation/CLAUDE.md` |

---


## Reference files — load only what the task needs

Section numbers cited elsewhere in this skill point into these files.

| Topic | Read |
|---|---|
| Binding mechanics — how C++ reaches AS | `references/binding-mechanics.md` |
| Silent-breakage catalog | `references/silent-breakage-catalog.md` |
| Generator and self-heal operations | `references/generator-operations.md` |

## 4. The three-environment verification recipe

Root doctrine non-negotiable #4: a public API change is done when verified in **C++, Blueprint, AND
AS**. Run the steps in order — each gate is necessary, none sufficient alone.

**Step 1 — C++ gate.** Build the editor target for your project (mechanics and env traps:
`ck-build-and-env` skill; in BusterBlock, use the Unreal Toolbox per that project's rules). A green
build proves the UFUNCTION surface compiles — nothing about bindings yet.

**Step 2 — AS compile gate: headless boot, then read the fresh log yourself.**
Generic shape (PowerShell; adjust engine/project paths — some projects build their own
`<Project>Editor-Cmd.exe` under the project's `Binaries/Win64/`, use it if present):

```powershell
& "<EngineDir>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<abs>\<Project>.uproject" `
    -skipcompile -ExecCmds="Quit" -unattended -nosplash -nullrhi
# Wait for PROCESS EXIT (not a wrapper's "done" signal), then:
Select-String -Path "<abs>\<Project>\Saved\Logs\<Project>.log" -Pattern 'Angelscript: Error' -Context 0,2
Select-String -Path "<abs>\<Project>\Saved\Logs\<Project>.log" -Pattern '<YourFile>\.as'
```

`-skipcompile` is load-bearing: it suppresses only the boot-time UBT/C++ compile path (AS still
compiles in-process, so this gate is unaffected), and without it a canceled boot can orphan UBT and
delete module DLLs in shared-engine multi-session environments — boot-shape ownership and the trap
details: `ck-build-and-env` §4 (traps T3/T4).

Log convention: `<Project>/Saved/Logs/<ProjectName>.log`, **rotated per boot** — confirm the mtime
is newer than your edit before trusting it. Zero `Angelscript: Error` lines AND no Warning naming
your file = compile-clean. Notes:
- Launching while an interactive editor for the same project is open is safe (read-only secondary,
  catalog 8) — it compiles against the owner's generated files.
- First-boot transients from self-heal (catalog 1, 9) are expected; the gate is the **final** state
  of the boot. Second boot still red = real.
- **Compile-clean is necessary, not sufficient**: the f-string handle throw (catalog 2) and any
  logic bug only manifest when the line executes. If your change adds runtime AS code, drive it —
  an AutoTest that executes the path (see `ck-tests-authoring-and-running`), or Step 3's PIE.

**Step 3 — Blueprint gate.** `[EDITOR-VERIFY]` — run the exact Blueprint checklist owned by
`ck-change-control` §"Three environments" (node search by DisplayName, enum dropdowns, exec pins,
Make-node visibility, EditCondition behavior, typed-handle autocast node, BindTo node). It owns
these steps; this recipe only sequences it between the AS gates.

**Step 4 — AS call-form smoke.** In a scratch `.as` (a gym/station or throwaway test body), compile
BOTH call forms of your new API — this is the check that catches the mixin/static split (catalog 6):

```angelscript
// (a) the generated namespace form — must always resolve:
auto Timer = utils_timer::Add(Handle, TimerParams);
utils_timer::BindTo_OnUpdate(Timer, FCk_Delegate_Timer(this, n"Tick"));

// (b) the mixin member form on a mutable handle — must resolve for arg0-matched functions:
auto TimerLocal = Timer;
TimerLocal.BindTo_OnUpdate(FCk_Delegate_Timer(this, n"Tick"));
```

Save → the hot reload verdict is in the log ~2s later (catalog 13). Then delete the scratch code.
If (a) fails: the wrapper hasn't regenerated — reboot the editor after the C++ build. If only the
static-class spelling fails: that's the mixin split working as designed — use (a).

---

## Common mistakes

- Calling `UCk_Utils_X_UE::Func(...)` in AS and "fixing" the signature when it doesn't resolve —
  check the ScriptMixin arg0 rule first (§1.2), then call through `utils_*`.
- Treating first-boot `not a data type` / `::Params` errors as failures and hand-editing generated
  files or JSON — they are self-heal transients (catalog 1, 9); gate on the boot's final state.
- Deleting `Script/Generated/` to "clean up" (catalog 7) or committing `_StubRecovery_*` files
  (gitignored for a reason).
- Trusting a compile-clean boot as runtime proof — the f-string throw (catalog 2) only fires on
  execution.
- Declaring "verified in AS" from a stale log — the log rotates per boot; check its mtime against
  your edit.
- Naming a new BFL with a strip-list suffix (catalog 4) instead of `_UE`.
- Assuming a C++ member is reachable from AS because it exists in C++. `default
  PrimaryActorTick.bCanEverTick = true;` does not compile — `bCanEverTick` is not bound on
  `FActorTickFunction`. Enable tick the way every CkTests gym does: `SetActorTickEnabled(true)` in
  `BeginPlay` (`CkCameraGym_Pawn`, `CkCompassGym`, `CkMinimapGym`). Check an existing AS consumer of
  the same engine type before writing the C++ shape from memory.
- Hand-editing `DynamicHandleTypes.json` in a UTF-8 editor — it is UTF-16 LE (catalog 1).
- Re-deriving self-heal/modal-tick/ownership behavior from scratch — the module Claude.md is the
  canonical mechanism doc; this skill only carries the operator's view.

## Provenance and maintenance

Authored 2026-07-02 (Phase-2 skill campaign). Every file:line above was read on that date against
CkFoundation submodule worktree + engine `D:/Repos/UnrealEngineAngelscript` (UnrealEngine-Angelscript
5.7.x). Engine line numbers drift on upgrades — re-verify before citing onward:

```powershell
# (cwd = superproject root, e.g. d:\Repos\BusterBlock; AS-plugin cites relative to
#  <EngineDir>\Engine\Plugins\Angelscript\Source\AngelscriptCode)
rg -n "NAME_Function_NotInAngelscript|UsableInAngelscript" <AngelscriptCode>/Private/Binds/Bind_BlueprintCallable.cpp
rg -n "bind it as a member" <AngelscriptCode>/Private/Binds/Helper_FunctionSignature.h
rg -n "Invalid type to append to string" <AngelscriptCode>/Private/Binds/Bind_FString.cpp        # :597 on 2026-07-02
rg -n "SuffixesToStrip|PrefixesToStrip" <AngelscriptCode>/Public/AngelscriptSettings.h           # :124-139
rg -n "::CheckForHotReload|::PerformHotReload" <AngelscriptCode>/Private/AngelscriptManager.cpp  # :1531 / :1211
# Plugin side (Grep tool is BLIND under Plugins/CkFoundation/Script — use Bash rg --no-ignore):
ls Plugins/CkFoundation/Script/Generated/utils_*.as | wc -l                                      # 268 on 2026-07-02
rg --no-ignore -n "return InOther;" Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/Handle/CkHandle_TypeSafe_AngelScript.h
rg --no-ignore -n "MixinParentTypeName" Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/Handle/CkHandle_AngelScript_Registry.h
rg --no-ignore -n "GenerateHandleTypeRegistry|ForceRefreshDynamicHandleBindings" Plugins/CkFoundation/Source/CkAngelscriptGenerator/DynamicHandles/CkDynamicHandleSubsystem.h
rg --no-ignore -n "_EnableAsBootstrapSelfHeal" Plugins/CkFoundation/Source/CkAngelscriptGenerator/Settings/CkAngelscriptGenerator_Settings.h
rg --no-ignore -n "RegenOwner.lock" Plugins/CkFoundation/Source/CkAngelscriptGenerator/CkAngelscriptGenerator_RegenOwnership.cpp
rg --no-ignore -n "BindTo_OnUpdate" Plugins/CkFoundation/Script/Generated/utils_timer.as         # member forward :196-200; static Add :266-270
```

Volatile facts to re-check on drift: the 268 wrapper count; generated-wrapper line numbers (file is
machine-rewritten); the Rev number and gate table in `Source/CkAngelscriptGenerator/Claude.md`
(that doc supersedes this skill's §3 summary on conflict); BusterBlock registry-path override
(`rg -n "DynamicHandleRegistryDirectory" Config/DefaultCkFoundation.ini`).
