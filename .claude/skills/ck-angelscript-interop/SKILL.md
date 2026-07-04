---
name: ck-angelscript-interop
description: Use when exposing or consuming CkFoundation API in AngelScript, or when AS breaks strangely — "No matching signatures", "Identifier 'FCk_Handle_X' is not a data type", "Invalid type to append to string", tests vanish from Session Frontend, editor frozen reloading Script/Generated, _StubRecovery_ files, DynamicHandleTypes.json, ScriptMixin vs utils_* call forms, self-heal banners, -NoCkAsRegen. Not for AS language syntax (Script/CLAUDE.md), test authoring (ck-tests-authoring-and-running), or build env (ck-build-and-env).
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

## 1. Binding mechanics — how C++ reaches AS

### 1.1 UFUNCTIONs auto-bind; two metas opt out

Every **native** UFUNCTION binds to AS automatically — there is no per-function opt-in. Exclusions
(engine `Private/Binds/Bind_BlueprintCallable.cpp:15-38`):

- `meta = (NotInAngelscript)` → never bound.
- `meta = (BlueprintInternalUseOnly)` → not bound, **unless** also `meta = (UsableInAngelscript)`.
- Non-native (BP-defined) functions and functions with unbindable param types are skipped.

Nuance: `Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY"` (used on e.g. `UCk_Utils_Timer_UE::Add`) is a
**category string**, not the `BlueprintInternalUseOnly` metadata — such functions bind to AS fine;
the category only buckets them away from casual BP graph wiring.

### 1.2 ScriptMixin — statics whose arg0 matches become handle members

The load-bearing pattern of every feature's utils class. Engine mechanics
(`Private/Binds/Helper_FunctionSignature.h:291-297`, meta names declared at `:15-38`): when a class
carries `Meta = (ScriptMixin = "FCk_Handle_Timer")` and a static UFUNCTION's **first parameter type
equals the mixin target type** (must be a reference or object pointer), that function binds as a
**member method of the target type** — arg0 becomes `this` and is removed from the AS signature.

Resolution consequence (the single most confusing behavior for newcomers):

| C++ static on `UCk_Utils_Timer_UE` (mixin target `FCk_Handle_Timer`) | AS sees |
|---|---|
| `BindTo_OnUpdate(FCk_Handle_Timer& InTimerEntity, ...)` — arg0 **==** target | **Member only**: `Timer.BindTo_OnUpdate(...)`. The static form `UCk_Utils_Timer_UE::BindTo_OnUpdate(Timer, ...)` does **not resolve** — "No matching signatures" even though the C++ exists. |
| `Add(FCk_Handle& InHandle, ...)` — arg0 is the **base** handle, ≠ target | **Plain static**: `UCk_Utils_Timer_UE::Add(H, P)` resolves (and the generated wrapper calls exactly that). |

Confirmed against `Source/CkTimer/Public/CkTimer/CkTimer_Utils.h:34` (UCLASS meta) and the generated
wrapper `Script/Generated/utils_timer.as:196-200` (member forward) vs `:266-270` (static call).
Multiple mixin targets are supported as a space-separated list in the meta string.

**Corollary — a mixin method needs a mutable lvalue.** The C++ takes `UPARAM(ref) FCk_Handle_X&`;
AS by-value params are read-only, so the member form won't bind on a by-value param or a const
local. This is why every generated wrapper body copies first:

```angelscript
auto _InTimerEntity = InTimerEntity;                              // copy to mutable local
return _InTimerEntity.BindTo_OnUpdate(InDelegate, ...);           // then the member form binds
```

Do the same in hand-written AS when calling a mixin method on a function parameter.

### 1.3 Typesafe-handle registration and conversions (CkEcs machinery)

`CK_REGISTER_ANGELSCRIPT_HANDLE_CONVERSION(_HandleType_)`
(`Source/CkEcs/Public/CkEcs/Handle/CkHandle_TypeSafe_AngelScript.h:41-88`) is a deferred
(PreCompile) registration that binds, per typesafe handle:

- `opImplConv()` / `opImplCast()` / `H()` to `FCk_Handle&` — the lambda body is `return InOther;`
  (`:48-57`). A **reference pass-through: no fragment check, no ensure**. See catalog item 5.
- `IsValid()`, `ToString()`, `Debug()` (fires the ensure then returns ToString), `opEquals` vs same
  type and vs base.
- Registers the type's `Has`/`Cast`/`CastChecked` lambdas plus its **mixin parent type name** into
  `FCkAngelScript_HandleRegistry`.

Parent-chain propagation (`Source/CkEcs/Public/CkEcs/Handle/CkHandle_AngelScript_Registry.h:26-41`):
`FCkAngelScript_HandleTypeInfo::MixinParentTypeName` is populated from the `MixinParentHandle`
typedef planted by `CK_GENERATED_BODY_HANDLE_DERIVED`. It drives two registry passes:

- `BindBaseMixinMethods()` (`:198`) — methods mixin-bound to a **parent** handle are re-bound onto
  the derived handle, so AS callers never cast back to the parent to call `Get_*`/`Request_*`.
- `BindParentChainConversions()` (`:200-215`) — implicit conversion of a derived handle to **every**
  typesafe ancestor, unchecked by design (doc comment in the header states the contract; use the
  explicit `As_<Parent>` cast when you want the boundary diagnostic).

Also bound per type: `ck::IsValid(x)` / `ck::Is_NOT_Valid(x)` free functions in the `ck` namespace
via `CK_DEFINE_ANGELSCRIPT_IS_VALID` (`Source/CkCore/Public/CkCore/Validation/CkIsValid_AngelScript.h:40-62`).

### 1.4 The `utils_*` wrapper layer — why "always call through utils_*" is mechanical, not style

At editor boot, `FCkAngelscriptWrapperGenerator` (module `CkAngelscriptGenerator`, boot-time
`FAngelscriptBinds` Early bind) emits one namespace file per **native, non-abstract CkFoundation
BFL that has static UFUNCTIONs** (filter:
`Source/CkAngelscriptGenerator/CkAngelscriptWrapperGenerator.cpp:219-249`) into
`Plugins/CkFoundation/Script/Generated/`. Count as of 2026-07-02: **268** `utils_*.as` files, plus
`CkFoundation_EntitySpawnParams.as` (the spawn-params generator's output, §3), `cvar.as`,
`collision.as`, `physicalsurface.as`, `deferred_asset_init.as`, and the `_index.as` manifest —
**274 files total**.

Namespace name derivation (`CkAngelscriptGenerator_SharedUtils.cpp:220-250`): strip leading `U`,
strip `Ck_` prefix, strip `_UE` suffix, snake_case the rest → `UCk_Utils_Timer_UE` → `utils_timer`.

Each wrapper function forwards to whichever form the engine actually bound — the **member form for
mixin-matched functions, the static form otherwise** (see 1.2 citations). That is why the doctrine
rule "ALWAYS use `utils_*`, NEVER `UCk_Utils_X_UE::`" (Script/CLAUDE.md §5) is mechanically correct:
the namespace form works uniformly; the full-class-name form randomly fails on exactly the functions
whose arg0 matches the mixin target. Hand-written sugar in `Script/CkUtils_*.as` merges into the
same namespaces (AS merges namespace blocks across files).

The `_UE` suffix on every utils class is also load-bearing: the engine strips BFL suffixes
`Statics/Library/BlueprintLibrary/BlueprintFunctionLibrary/FunctionLibrary` and prefixes
`UKismet/UBlueprint` when namespacing (`Public/AngelscriptSettings.h:124-139`) — `_UE` is not on the
list, so names round-trip. See catalog item 4 and Script/CLAUDE.md §16.1.

### 1.5 `CK_PROPERTY` / `CK_DEFINE_CONSTRUCTORS` — struct accessors in AS

Under `WITH_ANGELSCRIPT_CK` (auto-set by `CkModuleRules`; code must compile both ways — root
doctrine "Identity"), the accessor macros additionally register AS methods on the value class:

- `CK_PROPERTY(_X)` → `CK_ANGELSCRIPT_PROPERTY_REGISTRATION_GETTER_SETTER`, `CK_PROPERTY_GET(_X)` →
  `..._GETTER_CONSTREF` (`Source/CkCore/Public/CkCore/Macros/CkMacros.h:71-131`).
- The registration (`CkMacros_AngelScript.h:293-361`) strips the leading underscore from the member
  name and binds `Get_<Name>()` / `Set_<Name>()` as methods — `_Duration` → `Params.Get_Duration()`,
  `Params.Set_Duration(...)`. The setter returns `ClassType&`, so fluent chains
  (`Params.Set_A(x).Set_B(y)`) work in AS exactly like C++/BP.
- **Ordering requirement:** the registration uses `using ClassType = ThisType;` — `ThisType` is
  defined by `CK_GENERATED_BODY(T)` (`CkMacros.h:67-69`), so `CK_GENERATED_BODY` must appear in the
  struct **before** any `CK_PROPERTY*`. (House canonical shape already does this; see root
  CLAUDE.md "Encapsulation".)
- Registration is skipped when UHT already bound an identically-named method, and dedup-tracked
  per `Class::Get_X` key, so double-registration cannot occur.
- `CK_DEFINE_CONSTRUCTORS(T, ...)` similarly registers the essential-param constructor with AS at
  PreCompile via `CK_ANGELSCRIPT_CTOR_REGISTRATION` (`CkMacros.h:160+`, impl
  `CkMacros_AngelScript.h:206-241`) — this is why `FCk_Fragment_Timer_ParamsData(FCk_Time(0.0))`
  constructs in AS. Param types are validated for AS compatibility and skipped silently if unsupported.

### 1.6 Worked example — Timer across the three environments

**C++** (`Source/CkTimer/Public/CkTimer/CkTimer_Utils.h:34, 49-56, 311-319` — real shapes, trimmed):

```cpp
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Timer"))
class CKTIMER_API UCk_Utils_Timer_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Timer_UE);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Timer] Add New Timer")
    static FCk_Handle_Timer
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Timer_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Timer",
              DisplayName = "[Ck][Timer] Bind To OnUpdate")
    static FCk_Handle_Timer
    BindTo_OnUpdate(
        UPARAM(ref) FCk_Handle_Timer& InTimerEntity,
        const FCk_Delegate_Timer& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);
};
```

**Blueprint**: nodes appear by DisplayName — "[Ck][Timer] Add New Timer" (in the
BLUEPRINT_INTERNAL_USE_ONLY category bucket) and "[Ck][Timer] Bind To OnUpdate" under Ck|Utils|Timer;
`UPARAM(ref)` makes the handle pin pass-by-ref.

**AngelScript** (all three call shapes are live in-tree):

```angelscript
// 1. Generated namespace wrapper — the doctrine-preferred form, works for every function:
auto Timer = utils_timer::Add(Handle, TimerParams);           // wrapper → static UCk_Utils_Timer_UE::Add
utils_timer::BindTo_OnUpdate(Timer, Delegate);                // wrapper → forwards to the member form

// 2. Mixin member form (what the wrapper calls internally; needs a mutable lvalue):
Timer.BindTo_OnUpdate(FCk_Delegate_Timer(this, n"Tick"));     // policy + postfire defaulted, delegate FIRST

// 3. Hand-written sugar merged into the same namespace (Script/CkUtils_Timer.as):
auto T = utils_timer::Create_Tick(InHandle, FCk_Delegate_Timer(this, n"Tick"));

UFUNCTION()
private void Tick(FCk_Handle_Timer InHandle, FCk_Chrono InChrono, FCk_Time InDeltaT) { }
```

The promise pattern is the same mechanism on pending handles: `UCLASS(..., Meta =
(ScriptMixin = "FCk_Handle_PendingEntityScript"))` with `Promise_OnConstructed`
(`Source/CkEcs/Public/CkEcs/EntityScript/CkEntityScript_Utils.h:143,157`) → AS calls
`Pending.Promise_OnConstructed(FCk_Delegate_EntityScript_Constructed(this, n"OnConstructed"))`.

---

## 2. Silent-breakage catalog

Thirteen verified failure modes. Items 1-10 are silent (compile passes or the symptom appears far
from the cause); 11-12 are loud but misleading; 13 is the "where do errors even land" map.
Format per item: **Symptom → Why silent → Check → Fix.**

**1. Stale/absent `DynamicHandleTypes.json`** — the dynamic-handle registry
(`asset <X>Handle of UCkDynamic_HandleDefinition` needs a matching JSON entry; Script/CLAUDE.md §7).
- Symptom: project-wide cascade `Identifier 'FCk_Handle_<X>' is not a data type` at editor start,
  plus `Hot reload failed ... Keeping all old script code`.
- Why silent/misleading: with self-heal ON (default) these first-pass errors are **expected
  transients** — the dispatcher writes `_StubRecovery_DynamicHandleTypes.json` + a permissive
  validator, the deferred regen (OnPostEngineInit) writes the real entry, and the recompile goes
  clean, all in ONE boot. Panicking at the first-pass errors leads to wrong "fixes".
- Check: success gate = a clean reload after the deferred regen AND the entry present in the JSON.
  Path chain: `UCk_Utils_Dynamic_Settings_UE::Get_DynamicHandleRegistryFilePath()`
  (`Source/CkDynamic/Public/CkDynamic/Settings/CkDynamic_Settings.cpp` — default `ProjectConfigDir`,
  superprojects override; BusterBlock: `Config/DefaultCkFoundation.ini:118` →
  `<Project>/Script/Generated/DynamicHandleTypes.json`). PowerShell (handles the UTF-16 encoding):
  `Select-String -Path <Project>\Script\Generated\DynamicHandleTypes.json -Pattern 'FCk_Handle_<X>'`.
- Fix: nothing — boot once and let self-heal converge. **A second boot still red = real problem**
  (convergence banner in the log names the callsites). File is UTF-16 LE — preserve encoding if you
  ever hand-edit (rebase staleness needs a UTF-16-preserving union merge). Manual regen: §3 buttons.

**2. Raw `FCk_Handle` in an f-string** — RUNTIME throw, invisible to every compile gate.
- Symptom: `Invalid type to append to string.` thrown at PIE-start or whenever the line executes;
  script execution of that function aborts.
- Why silent: the throw site is the engine's dynamic append fallback
  (`Private/Binds/Bind_FString.cpp:597`) — **not a compile error**. A clean editor/headless boot
  (with or without `-skipcompile`) proves nothing about this; the line must actually run.
- Check: exercise the code path (PIE, or an AutoTest that executes the line) and grep the log for
  the throw message.
- Fix: `f"{SomeHandle.ToString()}"` — every typesafe handle has `.ToString()` bound (§1.3).

**3. EntitySpawnParams phantom namespace** — deleted entity script + surviving generated block.
- Symptom: an entity-script class deleted and later re-added **with the same name** never registers
  as a live UClass — `UObjectIterator` misses it, the AutoTest populator silently drops the test,
  the class never appears in Session Frontend. Reproduced 2026-05-12.
- Why silent: `<Plugin>_EntitySpawnParams.as` emits the class name as a real AS identifier
  (`namespace U<Script> { ... }` + `F<Script>_SpawnParams`); the stale block makes AS treat the
  re-added name as already-known and it skips UClass registration without any error.
- Check: with the class `.as` deleted, `rg --no-ignore -n "U<ClassName>"
  <Plugin>/Script/Generated/*_EntitySpawnParams.as` — a surviving block is the smoking gun.
- Fix: when reverting generator/test state, revert **every** file under `Script/Generated/*.as`
  atomically, never `AutoTestActors.as` alone (canonical:
  `Source/CkAngelscriptGenerator/Claude.md`, "EntitySpawnParams.as is NOT resilient..."). Emergency
  unblock: rename the class.

**4. BFL suffix-strip collision** — engine rewrites your namespace.
- Symptom: `No matching signatures to 'UMyFeature_FunctionLibrary::Foo()'` — looks like a param
  mismatch; actually the class name was silently rewritten.
- Why silent: the engine strips suffixes `Statics/Library/BlueprintLibrary/BlueprintFunctionLibrary/FunctionLibrary`
  and prefixes `UKismet/UBlueprint` from BFL names when namespacing
  (`Public/AngelscriptSettings.h:124-139`).
- Check: does your BFL name end in a strip-list suffix?
- Fix: name it `UCk_Utils_<X>_UE` (house rule; `_UE` round-trips), or override with
  `UCLASS(meta = (ScriptName = "..."))`. Details: Script/CLAUDE.md §16.1.

**5. Unchecked parent handle up-conversion** — the implicit conversion carries no guarantee.
- Symptom: an ensure/crash **deep inside a downstream util** ("fragment missing") far from the call
  that introduced the bad handle.
- Why silent: derived→parent implicit conversion is a byte pass-through — `opImplConv` is literally
  `return InOther;` (`CkHandle_TypeSafe_AngelScript.h:48-57`), and the parent-chain pass binds the
  same unchecked shape for every ancestor (`CkHandle_AngelScript_Registry.h:200-215`). No
  CastChecked or fragment-presence ensure runs at the boundary.
- Check: when a util ensures on a handle you converted, audit where the typed handle came from —
  especially handles produced while a **permissive dynamic-handle validator** was live (§3) or
  default-constructed.
- Fix: when provenance is uncertain, use the explicit `As_<Parent>()` / typed cast so the boundary
  diagnostic fires at the conversion, not three calls later. (Script/CLAUDE.md §6.)

**6. New C++ ScriptMixin function — wrong call form, wrong build order.**
- Symptom: fresh C++ util function compiles, but AS `UCk_Utils_X_UE::Func(Handle, ...)` reports
  "No matching signatures".
- Why silent/misleading: if arg0's type equals the class's ScriptMixin target, the function was
  bound as a **member only** (§1.2) — the static spelling never existed. Compounding: C++ must be
  **built before** the AS that calls it compiles (next editor boot regenerates the wrapper).
- Check: compare arg0's type to the `ScriptMixin` meta string on the UCLASS; then check
  `Script/Generated/utils_<feature>.as` for the emitted wrapper.
- Fix: build C++ → boot editor (wrapper regenerates) → call `utils_<feature>::Func(...)`, or the
  member form on a mutable local.

**7. Blanket-deleting or touching `Script/Generated/`** — the mtime trap.
- Symptom: multi-second frozen editor right after boot (full AS reload sweep, literal-asset
  re-init), or an endless reload loop; historical incident: ~8s frozen on EVERY launch (2026-06-11).
- Why silent: the Hazelight hot-reload checker baselines `.as` **mtimes** at its first scan — ANY
  later mtime change under `Script/Generated/`, byte-identical or not, triggers a game-thread
  reload. The generator's own hygiene depends on this: manifest-based cleanup via `_index.as` and
  `SaveWrapperFile_IfChanged` (LF-normalized compare) exist precisely so unchanged files keep their
  mtimes (`CkAngelscriptWrapperGenerator.cpp:40,94,190-191`; incident write-up in the module
  Claude.md, "Mtime stability").
- Check: post-init structural reload of a `Generated.*` module in the log; the ES Params generator
  logs a **rewrite reason** whenever a bucket rewrites.
- Fix: **never** `rm Script/Generated/*` and never script anything that rewrites those files while
  an editor runs. Recovery from bad generated state = git revert of the whole directory (item 3)
  with the editor closed, or the §3 regen buttons.

**8. Two editor/headless instances of one project** — Rev 12 single-writer lock.
- Symptom: in the second instance, generated files never update; regen buttons appear to do
  nothing beyond a Warning.
- Why silent-ish: an exclusive-write OS file lock on
  `<ProjectSavedDir>/CkAngelscriptGenerator_RegenOwner.lock`
  (`CkAngelscriptGenerator_RegenOwnership.cpp:115-118`) makes every later instance a **read-only
  secondary**: it compiles against the owner's generated files and writes nothing. One prominent
  startup Warning; per-site skips are VeryVerbose. This *replaced* the pre-Rev-12 failure mode
  (two writers ping-ponging mirror rewrites — 496 rewrites/686 reloads in the 2026-06-12 incident).
  OS releases the handle on any process exit, so stale locks are impossible; a surviving secondary
  lazily takes over at its next regen event ("Ownership ACQUIRED" log line).
- Check: the startup Warning ("SECONDARY"), or the lock file's breadcrumb (pid + cmdline) in Saved.
- Fix: intentional. If you need this instance to generate, close the owner first. A headless
  compile-check alongside an open editor is now safe by design.

**9. Spawn-params codegen lag** — new entity script or new `ExposeOnSpawn` property.
- Symptom: `No matching signatures to '<Class>::Params(...)'` (or
  `Identifier 'F<X>_SpawnParams' is not a data type` on a fresh clone) on the first compile pass
  after the change.
- Why silent/misleading: `<Plugin>_EntitySpawnParams.as` is emitted by a **post-compile** generator
  — a brand-new class + its callsite in the same pass can't see the accessor yet. With self-heal ON
  this is an expected transient: the dispatcher synthesizes a sibling `_StubRecovery_*` stub
  (source-derived full shape when it can find your class's `.as`), compile succeeds, the real
  generator regenerates, the stub is deleted.
- Check: second compile pass clean + the accessor present in the canonical file.
- Fix: none needed normally. Self-heal disabled: break the callsite, compile, restore (the manual
  two-phase). Superproject detail: BusterBlock `Script/CLAUDE.md`, "Codegen lag" section.

**10. Typed delegates can't ride `ExposeOnSpawn`.**
- Symptom: a `::Params(...)` overload taking your `FCk_Delegate_*` / typed AS delegate never
  matches, no matter what you pass.
- Why silent: the generated SpawnParams struct coerces the delegate property to a generic script
  delegate; there is no implicit conversion from the typed delegate at the callsite.
- Check: read the emitted struct in `<Plugin>_EntitySpawnParams.as` — the field isn't your type.
- Fix: `ExposeOnSpawn` the `(UObject Target, FName FunctionName)` pair and rebuild the typed
  delegate inside `DoConstruct`. Worked example: BusterBlock `Script/CLAUDE.md`, "Typed delegates
  can't ride through ExposeOnSpawn".

**11. Adjacent string literals** (loud but misleading).
- Symptom: compile error pair `Expected ')' or ','` + `Instead found '<string constant>'`.
- Cause: `"foo " "bar"` C-style splicing — AS does not splice adjacent literals. The self-heal
  parser recognizes the shape and banners file:line:col with suggested fixes, but **never edits
  user source** (author-side bug, out of its contract).
- Fix: one literal, or f-string interpolation.

**12. By-value struct params are read-only; const propagates hard** (loud but misleading).
- Symptom: `Cannot assign, variable is const or is not a valid l-value` at any nesting depth, or
  baffling "no matching signature" when passing a const value to a non-const value param.
- This is AS language semantics, not a Ck binding bug — full rules and fixes:
  Script/CLAUDE.md §9.1 (by-value read-only) and §9.2 (const propagation, `Cast<T>` preserves
  const, `TArray::Add` of const rejected).

**13. Where AS errors land, and what triggers a recompile** (the map).
- Editing any watched `.as` → engine `FAngelscriptManager::CheckForHotReload`
  (`Private/AngelscriptManager.cpp:1531`) → `PerformHotReload` (`:1211`). Compile diagnostics land
  in the editor log under the **`Angelscript`** category (`DEFINE_LOG_CATEGORY(Angelscript)`,
  `AngelscriptManager.cpp:64`) — grep for `Angelscript: Error`. Verdict lands ~2s after save; check
  immediately, never poll with sleep loops.
- Boot-time compile failure opens the Hazelight modal — that's where the self-heal dispatcher (§3)
  intervenes from a deferred modal-tick callback.
- A parse error anywhere in a file kills **every class in that file** silently downstream: placed
  actors fall back to native base classes, `default X = ...` class references resolve to nothing.
  Fix the first error before chasing ghost symptoms.

---

## 3. Generator + self-heal operations

What `CkAngelscriptGenerator` (editor-only module) runs, when — canonical mechanism doc:
`Source/CkAngelscriptGenerator/Claude.md` (read it before touching the module):

| When | What | Output |
|---|---|---|
| Editor boot (AS Early bind) | `FCkAngelscriptWrapperGenerator` | 268 `utils_*.as` + `cvar/collision/physicalsurface/_index.as` (CkFoundation `Script/Generated/`) |
| After each successful AS compile | `FCkAngelscriptEntityScriptParamsGenerator` | `<Plugin>_EntitySpawnParams.as` per plugin (BP-generated classes excluded) |
| After each successful AS compile | `FCkAutoTestWrapperGenerator` | `<Plugin>_AutoTestActors.as` (resilient `FSoftClassPath` lookup — drift is runtime-only, see catalog 3) |
| Post-compile / on demand | `UCkAssetRegistrySubsystem` | `*Assets.as` accessor files from `UCkAssetRegistryConfig` assets |
| On demand / self-heal deferred regen | `UCkDynamicHandleSubsystem` | `DynamicHandleTypes.json` |

All writes funnel through the Rev-12 ownership gate (catalog item 8). Generated output is
deterministic by contract: no timestamps, stable sorts, CRLF-on-write with LF-normalized compare.

**Manual maintenance buttons** — `UCkDynamicHandleSubsystem` (editor subsystem;
`Source/CkAngelscriptGenerator/DynamicHandles/CkDynamicHandleSubsystem.h:39-41,67-71`):
- `GenerateHandleTypeRegistry()` (`CallInEditor`) — rewrite the JSON from all discovered
  `UCkDynamic_HandleDefinition` assets.
- `ForceRefreshDynamicHandleBindings()` (`CallInEditor`, DevelopmentOnly) — regen + re-register
  live AS bindings **without an editor restart** (also the fix when a definition's
  `RequiredFragments` changed and in-memory validators are stale).

**Escape hatches** (either short-circuits self-heal hook registration):
- `-NoCkAsRegen` launch flag — per-session, for inspecting raw Hazelight diagnostics without the
  dispatcher intervening.
- `UCk_AngelscriptGenerator_ProjectSettings_UE::_EnableAsBootstrapSelfHeal` (default `true`;
  `Source/CkAngelscriptGenerator/Settings/CkAngelscriptGenerator_Settings.h:36`) — project-wide,
  via Editor Settings → "AngelScript Generator" or `CkFoundation.ini`.

**Self-heal in one paragraph:** on a failed boot compile the dispatcher parses Hazelight's error
output (regexes pinned by snapshot tests `Tests/Test_AsErrorParser.cpp` — the engine-upgrade
canary; run `CkAngelscriptGenerator.UnitTests.AsErrorParser.*` before shipping an engine-plugin
upgrade), classifies recognized roots (missing `::Params`, missing `F<X>_SpawnParams`, missing
dynamic-handle type, missing asset accessor, adjacent literals), and writes recovery stubs to
sibling `_StubRecovery_*` files (never the canonicals; gitignored) from a **deferred modal-tick
callback** — writing synchronously inside the reload-error broadcast lands before the hot-reload
thread's mtime baseline and wedges the modal forever. Caps: 3 recovery cycles per bootstrap, 3
synthesis attempts per signature (convergence blacklist with a terminal banner naming the
callsites). Stubs are cleaned per-generator after the canonical regenerates; force-quit survivors
are swept at next StartupModule.

**Rev 9 → 12 hardening story** (dates; full detail in `Source/CkAngelscriptGenerator/Claude.md`):
1. Rev 9 (2026-05-11): descriptor-driven regen via a surgical engine-fork delegate — death-spiraled
   on first end-to-end test (595→595→0→17 classes); fork reverted same day.
2. Rev 10 (2026-05-12): error-driven self-heal dispatcher on the stock `GetReloadHadErrors`
   delegate; modal-tick deferral discovered empirically; 3-cycle bootstrap cap.
3. Rev 10.x (2026-05-13/17): asset-accessor Tier-3 fallback removed (refuse + banner); adjacent-
   literal diagnosis added.
4. Per-signature convergence cap after the 2026-05-21 `Params` dueling-overloads incident (724+
   recovery lines/session).
5. Rev 11 (post 2026-06-10/11 wedges): arg-category normalization, nullptr→UObject fallback +
   same-arity ambiguity gate, stale-canonical **quarantine** (delete-only, forensic copy to
   `Saved/CkSelfHeal/Quarantine/`).
6. Rev 12 (post 2026-06-12 two-instance ping-pong): cross-process single-writer OS file lock +
   real BPGC exclusion in the params generator.

---

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
