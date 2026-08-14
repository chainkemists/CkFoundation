---
name: ck-debugging-playbook
description: "Use when triaging Ck build, UHT, linker, AngelScript, editor-boot, packaged-only, GC, DLL-lock, or PIE-versus-packaged failures; not for setup, profiling, or incident history."
---

## Overview

Symptom-first triage for the failures that actually occur in Ck plugin development: build/UHT/linker
breakage, AngelScript compile walls, and the crashes that only exist outside PIE. Start at the triage
table, jump to the numbered section, run the discriminating experiment BEFORE applying a fix — most of
these symptoms have 2-3 causes and the wrong fix (deleting directories, re-running until green) destroys
the evidence. Facts and line numbers verified 2026-07-02 against CkFoundation dev `7330c1bab`, engine
UnrealEngine-Angelscript 5.7.4 at `D:/Repos/UnrealEngineAngelscript` (root CLAUDE.md "Identity");
commands assume cwd = superproject root (the BusterBlock layout is the verified example).

Jargon used below, once: **UBT/UHT** = UnrealBuildTool / UnrealHeaderTool. **Unity build** = UBT
concatenating many .cpp files into one translation unit (on by default for every Ck module —
`CkModuleRules` ctor, `Source/CkBuildConfig/CkBuildConfig.Build.cs:203`). **Fragment** = ECS component
(root CLAUDE.md Lingo). **Ensure** = `CK_ENSURE_IF_NOT`, the house validation macro. **Disregard-for-GC
set** = UE's permanent object pool, closed once at boot (`GUObjectArray.CloseDisregardForGC()`, engine
`Runtime/Launch/Private/LaunchEngineLoop.cpp:3863`); GC never traverses objects inside it.

## Triage table

| # | Symptom | Ranked likely causes | Discriminating experiment | Fix | § |
|---|---|---|---|---|---|
| 1 | Boot prompt: "The following modules are missing or built with a different engine version" (engine LaunchEngineLoop.cpp:6639) | 1. binaries older than source 2. DLLs deleted by a killed/overlapped UBT run 3. engine version bumped | compare DLL mtime vs your last edit; `Get-Process msbuild,dotnet,UnrealBuildTool` | rebuild the target; see §7 if multi-session | §1 |
| 2 | Boot: "Plugin 'X' failed to load because module 'Y' could not be found" (engine PluginManager.cpp:2734) | 1. launched config ≠ built config (per-config DLL names) 2. new module never built / missing from .uplugin | `ls Binaries/Win64 \| grep <Module>` — which config suffixes exist? | build or launch the matching configuration | §1 |
| 3 | Build: `LNK1104: cannot open file '...-<Module>.dll'` | 1. editor running (incl. one launched mid-build, or a zombie) 2. second build in flight | log-lock probe (§1.2) + process list | close/kill the holder, rebuild | §1 |
| 4 | Ghost compile/UHT errors after a branch switch; errors citing code that no longer exists | stale Intermediate (UHT gen / obj desync) | do the errors reference deleted symbols/files? | targeted Intermediate delete (§1.3) — never Saved/ or DDC | §1 |
| 5 | UHT: "Expected a GENERATED_BODY() at the start of the class" / "#include found after .generated.h file" | reflection body/include ordering | — (message is the diagnosis) | fix the order (§2) | §2 |
| 6 | Compile: `'ThisType': undeclared identifier` at a `CK_PROPERTY*` site | `CK_GENERATED_BODY(...)` missing or placed after the macro that needs it | — | put `CK_GENERATED_BODY` right after `GENERATED_BODY()` | §2 |
| 7 | Link: LNK2019/LNK2001 unresolved external, symbol lives in another Ck module | 1. missing `CK<MODULE>_API` export 2. missing Build.cs dependency | is the defining module in your Build.cs? is the symbol exported? | add the export / the dep (tier rules apply) | §3 |
| 8 | Link/compile: LNK2005/C2084 duplicate symbol; or an UNTOUCHED file suddenly fails after you edited a sibling .cpp | 1. file-local `static`/anonymous-namespace helper colliding under unity 2. unity group shift exposing a latent missing include | did you add a file-local helper? did the blob membership change? | named namespace (root doctrine); add the explicit include | §3 |
| 9 | First editor boot after a handle/codegen change: wall of "'FCk_Handle_X' is not a data type" + "Hot reload failed ... Keeping all old script code" | EXPECTED self-heal transient (1st boot) vs real AS error | boot a second time: still red = real | none needed if 2nd boot clean; else fix the script | §4 |
| 10 | AS: "No matching signatures" calling `UCk_Utils_X_UE::Func(Handle, ...)` | arg0 type == the class's `ScriptMixin` target → bound as member only | does arg0's type match the `Meta = (ScriptMixin = "...")` string? | call `Handle.Func(...)` or `utils_x::Func(...)` | §4 |
| 11 | Runtime AS throw: "Invalid type to append to string." | raw `FCk_Handle` in an f-string (compile-clean, fails at exec) | — | `{Handle.ToString()}` | §4 |
| 12 | Packaged client crashes 0xC0000005; PIE always clean | disregard-for-GC pool violation (AS-born owners referencing normal-pool objects) | cooked client + `Ck.Diag.VerifyGCAssumptions` | §5 runbook | §5 |
| 13 | Standalone / `-nullrhi`: actor (e.g. PlayerController) destroyed seconds after spawn; fine in net play | UObject with no strong GC root — fragments/weak chains only; a live netdriver was masking it | reproduce with no netdriver; audit "who roots this object?" | give the owning fragment a `TStrongObjectPtr` | §5 |
| 14 | Works in PIE, silently different in packaged (no crash) | cook stripping, editor-only module, define gate, config diff | walk the §6 checklist | per axis | §6 |
| 15 | "My ensure never fired" in headless / Test / Shipping | 1. `-unattended`/commandlet → LogOnly, no dialog 2. Test/Shipping → ensures are SILENT by design | grep the log; know the flag matrix | expected behavior — read §6.2 before "fixing" | §6 |
| 16 | Module DLLs vanish mid-build; rebuild storms; generated-file ping-pong with two sessions | multi-session traps | `Get-Process` sweep; check the generator lock | §7 rules | §7 |


## Reference files — load only what the task needs

Section numbers cited elsewhere in this skill point into these files.

| Topic | Read |
|---|---|
| Failure classes §1-§7 | `references/failure-classes.md` |

## Common mistakes

- **Stale-green** — the canonical telling (root doctrine, `ck-change-control`,
  `ck-macros-and-codegen` §2.11, and the tests skill all point here): a build/test run whose
  binaries predate your last edit proves nothing. Global registrations (`CK_REGISTER_SNAPSHOTABLE`,
  `CK_REGISTER_PROCESSOR`, inspector/provider registrars) are static-init side effects baked into
  the binary — an old binary registered the OLD set and actively lies. Recovery: rebuild, confirm
  the module DLL mtime (§1.1) is newer than your last edit, then re-run the FULL gate and report
  the delta. Doubly treacherous with an editor open during the edit (hot-reload half-state) and in
  shared-engine multi-session environments (§7).
- Declaring the first AS boot's error wall a failure — or its eventual silence a success — without the
  second-boot discriminator (§4.1).
- Deleting `Saved/`, `DerivedDataCache/`, or `Script/Generated/` to "fix" a C++ build problem (§1.3, §7.5).
- Reading "no ensure dialog" as "no ensure fired" in `-unattended`, Test, or Shipping (§6.5).
- Patching a packaged GC crash at the crash site instead of answering "who roots this object?" (§5.4).
- Re-running a flaky build until green instead of sweeping for the concurrent process holding it hostage (§7.1).
- **Piping a build or test command through `tail`/`head`/`grep` and then trusting the exit code** —
  the shell reports the LAST stage's status, so `Build.bat ... | tail -60` prints exit 0 on a failed
  build and the harness's completion ping agrees. Redirect to a log, echo `$?` unpiped, and read the
  run's own verdict line (`Result: Succeeded`, `**** TEST COMPLETE. EXIT CODE: 0 ****`). Never let a
  wrapper's success stand in for the build's.
- **Reading a wall of C++ errors as a wall of problems.** One bad deduction cascades: `auto* X =
  SomeArray[i];` over a `TObjectPtr` array deduces NOTHING (`auto*` does not apply user-defined
  conversions), so `X` becomes an error type and every later use reports its own unrelated-looking
  failure — 12 errors across 4 call sites from one line. Fix the FIRST error and rebuild before
  reading the rest. Spell the pointer type out when the source is a `TObjectPtr`/handle wrapper.
- **Trusting a verification script you just wrote.** A parameter-order checker reported 44 mismatches
  that were all its own comment-splitting bug. When a checker says everything is wrong, suspect the
  checker; confirm it reports a known-good case as good before acting on its failures.

## When NOT to use this skill

| You actually want | Load instead |
|---|---|
| Write or run tests (AutoTest / Gauntlet / gym / net) | `ck-tests-authoring-and-running` |
| Profile a processor; make/verify a perf claim | `ck-performance-and-analysis` |
| "Has this been tried before?" — incidents, reverts, dead ends | `ck-failure-archaeology` |
| Macro expansions, add a fragment/processor/handle | `ck-macros-and-codegen` |
| The full AS silent-breakage catalog / expose an API to AS | `ck-angelscript-interop` |
| GC/lifetime theory, entity↔actor contract | `ckecs-domain-reference` |
| Engine/plugin/environment setup from scratch | `ck-build-and-env` |

## Provenance and maintenance

Authored 2026-07-02 against CkFoundation dev `7330c1bab`, BusterBlock superproject layout, engine
UnrealEngine-Angelscript 5.7.4 (`D:/Repos/UnrealEngineAngelscript`). Engine file:line cites drift with
engine updates — re-grep rather than trusting numbers. Re-verification (Git Bash, cwd = superproject
root; NOTE: the agent Grep/Glob tools are blind under `Plugins/CkFoundation/Script/` — use
`rg --no-ignore` there, per root CLAUDE.md provenance):

- SHAs still real: `git -C Plugins/CkFoundation show --stat feb08ee94 d77810096 a8a93baac 56b344310 72199da89 700b5ef95 362e8917a 0eb3208aa`
- Diag commands: `rg -n 'Ck\.Diag\.' Plugins/CkFoundation/Source/CkCore/Public/CkCore/Object/CkObject_Utils.cpp`
- Rooting pass + why-not-in-editor note: `rg -n 'WITH_EDITOR|doesn.t manifest in-editor' Plugins/CkFoundation/Source/CkCore/Public/CkCore/IO/CkDeferredAssetInit_AngelScript.cpp`
- Ensure modes: `rg -n 'CK_DISABLE_ENSURE' Plugins/CkFoundation/Source/CkCore/Public/CkCore/Ensure/CkEnsure.h Plugins/CkFoundation/Source/CkBuildConfig/CkBuildConfig.Build.cs`
- Unattended/commandlet LogOnly: `rg -n 'IsUnattended' Plugins/CkFoundation/Source/CkCore/Public/CkCore/Ensure/CkEnsure.cpp`
- Overlay gate: `rg -n 'WITH_CK_DEBUG_OVERLAY' Plugins/CkGameplayDebugger/Source/CkEntityDebugOverlay/CkEntityDebugOverlay.Build.cs`
- DLL naming / build-state layout: `ls Binaries/Win64 | grep CkCore` ; `ls Intermediate/Build/Win64/x64/BusterBlockEditor/`
- Strong-ptr fix current: `rg -n 'TArray<TStrongObjectPtr' Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h`
- Engine anchors: `grep -n 'SKIPCOMPILE\|missing or built with a different' <Engine>/Source/Runtime/Launch/Private/LaunchEngineLoop.cpp` ; `grep -n 'gc.CollectGarbageEveryFrame' <Engine>/Source/Runtime/Engine/Private/UnrealEngine.cpp` ; `grep -n 'bUseAdaptiveUnityBuild' <Engine>/Source/Programs/UnrealBuildTool/Configuration/TargetRules.cs`
- Operator-experience items (§1.1 config drift, §1.2 early-launch lock, §7.1, §7.2) have no git/doc
  anchor by design — if they stop matching observed behavior, update or drop them here rather than
  inventing citations.
