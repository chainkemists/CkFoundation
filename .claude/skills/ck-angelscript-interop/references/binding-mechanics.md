# Binding mechanics — how C++ reaches AS

Reference for `ck-angelscript-interop`: the full path from a C++ declaration to a callable AS symbol.

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

