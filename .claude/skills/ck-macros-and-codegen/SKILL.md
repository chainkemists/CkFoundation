---
name: ck-macros-and-codegen
description: "Use when reading or writing CK_ macros or adding Ck fragments, processors, requests, signals, tags, snapshot registration, or typesafe handles; not for architecture rationale."
---

## Overview

Every CkFoundation type is assembled from `CK_` macros: they generate accessors, constructors,
typed handles, signals, and self-registration, and they encode the framework's contracts in their
expansions. This skill gives you each macro's actual expansion, the generated members, the
constraints **with the mechanical reason**, one canonical call site, and file-by-file checklists
for the six "add a new X" rituals. Style/naming rules are owned by the root doctrine
(`Plugins/CkFoundation/CLAUDE.md`) — this skill only explains the machinery behind them.

All file paths below are relative to `Plugins/CkFoundation/Source/`. All line numbers and counts
verified against source 2026-07-02 (re-verification commands at the bottom).

## When NOT to use this skill

| You actually want | Load instead |
|---|---|
| WHY the architecture is shaped this way (handles, deferral, lifecycle invariants) | `ckecs-architecture-contract` |
| How AS bindings work end-to-end, exposing/verifying script APIs, dynamic handles | `ck-angelscript-interop` |
| EnTT theory, entity↔actor lifetime, GC interaction | `ckecs-domain-reference` |
| A build/UHT/linker error that isn't macro-shaped | `ck-debugging-playbook` |
| Which module a feature belongs in | `Source/CLAUDE.md` (topology) |
| To USE an existing feature (add a Health attribute, a timer, …) — not author a new one | `Source/CLAUDE.md` decision tree (+ `Script/CLAUDE.md` §5 for AS); no skill needed |

## 1. Census and family map

**273 `#define CK_*` definition sites, 247 unique macro names** (as of 2026-07-02; the delta is
macros redefined across `#if` branches — `CK_ENSURE_IF_NOT` ×3, AS no-op twins, etc.). Verify:

```powershell
# cwd = d:\Repos\BusterBlock (PowerShell or Git Bash)
rg -c '#define CK_' Plugins/CkFoundation/Source --glob '*.{h,inl,hpp}'   # sum ≈ 273
```

Two non-`CK_` framework macros also live in `CkCore/Public/CkCore/Macros/CkMacros.h`:
**`NOT`** (= `!`, :31 — house style for all logical negation) and **`COMMA`** (= `,`, :32 — passes
commas through macro arguments, e.g. `CK_GENERATED_BODY(TFoo<A COMMA B>)`).

| Family | Representative macros | Defined in | You touch it when… |
|---|---|---|---|
| Identity / class body | `CK_GENERATED_BODY`, `CK_ENABLE_CUSTOM_VALIDATION`, `CK_ENABLE_SFINAE_THIS` | `CkCore/Public/CkCore/Macros/CkMacros.h:63-69,268` | every Ck struct/class |
| Property accessors | `CK_PROPERTY`, `CK_PROPERTY_GET` (+`_BY_COPY`,`_NON_CONST`,`_PASSTHROUGH`,`_STATIC`), `CK_PROPERTY_SET`, `CK_PROPERTY_UPDATE` | CkMacros.h:72-145 | every data member |
| Constructors | `CK_DEFINE_CONSTRUCTORS`, `CK_DEFINE_CONSTRUCTOR_1..9`, `CK_USING_BASE_CONSTRUCTORS` | CkMacros.h:160-217 | every params/request struct |
| Operators | `CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(_T)`, `CK_DECL_AND_DEF_OPERATORS(_T)`, assignment-op sets | CkMacros.h:221-263 | comparable value types |
| Ensure (runtime validation) | `CK_ENSURE`, `CK_ENSURE_IF_NOT`, `CK_TRIGGER_ENSURE(_IF)`, `CK_INVALID_ENUM`, `CK_ENSURE_VALID_IF_NOT(_MSG)`, `CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT`, `CK_PURE_VIRTUAL` | `CkCore/Public/CkCore/Ensure/CkEnsure.h:36-85`; CkMacros.h:300-306 | every precondition |
| IsValid plumbing | `CK_DEFINE_CUSTOM_IS_VALID_INLINE`, `CK_DECLARE/DEFINE_CUSTOM_IS_VALID*`, `CK_DEFINE_CUSTOM_IS_VALID_POLICY`, `CK_DELETE_CUSTOM_IS_VALID` | `CkCore/Public/CkCore/Validation/CkIsValid.h:24-235` | new validatable type |
| Formatters | `CK_DEFINE_CUSTOM_FORMATTER_ENUM`, `CK_DECLARE/DEFINE_CUSTOM_FORMATTER*` | `CkCore/Public/CkCore/Format/CkFormat.h:133-302` | every UENUM; loggable types |
| Signals | `CK_DEFINE_SIGNAL_AND_UTILS(_WITH_DELEGATE)`, `CK_SIGNAL_BIND(_PROMISE/_REQUEST_FULFILLED/_WITH_CONDITION)`, `CK_SIGNAL_UNBIND` | `CkEcs/Public/CkEcs/Signal/CkSignal_Macros.h:10-64` | events |
| Requests | `CK_REQUEST_DEFINE_DEBUG_NAME` | `CkEcs/Public/CkEcs/Request/CkRequest_Data.h:111-121` | every request struct |
| Typesafe handles | `CK_GENERATED_BODY_HANDLE_TYPESAFE(_DERIVED)`, `CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE`, `CK_DEFINE_CPP_CASTCHECKED_TYPESAFE`, `CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE` | `CkEcs/Public/CkEcs/Handle/CkHandle_TypeSafe.h:84-242` | new feature handle |
| Lifetime view filters | `CK_IGNORE_PENDING_KILL`, `CK_IF_END_PLAY`, `CK_IF_TEARING_DOWN`; `CK_IF_HANDLE_IS_PENDING_KILL` | `CkEcs/Public/CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h:37-50`; `CkEcs/Public/CkEcs/Handle/CkHandle.h:395-396` | every processor declaration |
| ECS tags | `CK_DEFINE_ECS_TAG`, `_TRANSIENT`, `_COUNTED` | `CkEcs/Public/CkEcs/Tag/CkTag.h:23-40` | lifecycle markers |
| Processor registration | `CK_REGISTER_PROCESSOR(_WITH_FACTORY)`, `CK_REGISTER_GROUP` | `CkEcs/Public/CkEcs/Scheduler/CkProcessorRegistration.h:85-95` | every processor .cpp |
| Snapshot | `CK_REGISTER_SNAPSHOTABLE`, `CK_SNAPSHOT_ANNOUNCE_EXPECTED` | `CkEcs/Public/CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h:138-152`, `CkSnapshot_Audit.h` | save/load participation |
| Records / EntityHolders | `CK_DEFINE_RECORD_OF_ENTITIES_ROUNDTRIP/_TRANSIENT`, `CK_DEFINE_ENTITY_HOLDER_ROUNDTRIP/_TRANSIENT` (+`_AND_UTILS`) | `CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h:102-120`; `CkEcsExt/Public/CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h:61-79` | child-entity collections |
| Logging | `CK_DEFINE_LOG_FUNCTIONS`, `CK_REGISTER_LOG_FUNCTIONS`, `CK_LOG_ERROR(_NOTIFY)_IF_NOT` | `CkLog/Public/CkLog/CkLog_Utils.h:106,338,408,412` | new module log |
| Debug callstack | `CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR`, `CK_CALLSTACK_RECORD(_MSG)`, `CK_CALLSTACK_CLEAR` | `CkEcs/Public/CkEcs/Handle/CkDebugCallstack_Macros.h:20-59` | request-origin tracing |
| Profiling | `CK_DEFINE_STAT`, `CK_DEFINE_PHASE_STAT`, `CK_STAT` | `CkProfile/Public/CkProfile/Stats/CkStats.h:70-109` | → `ck-performance-and-analysis` |
| AS bridge (~90 macros) | `CK_ANGELSCRIPT_*` | `CkCore/Public/CkCore/Macros/CkMacros_AngelScript.h` | never directly — internals of the families above; → `ck-angelscript-interop` |
| Utility | `CK_CONCAT`, `CK_UNIQUE_NAME`, `EXPAND(_ALL)`, `NARG_`, `NOT`, `COMMA`, `CK_SCOPE_CALL` | CkMacros.h:19-59,280-285 | building other macros |

Call-site frequency (grep over `Source/*.{h,cpp,inl}`, 2026-07-02) — the top ones you must know
cold: `CK_PROPERTY_GET` 1707 · `CK_GENERATED_BODY` 1592 · `CK_ENSURE_IF_NOT` 1448 · `CK_PROPERTY`
1144 · `CK_DEFINE_CONSTRUCTORS` 541 · `CK_REGISTER_PROCESSOR` 388 registrations (raw grep 390:
+1 commented example, +1 `#define`) · `CK_DEFINE_ECS_TAG` 236 · `CK_REQUEST_DEFINE_DEBUG_NAME` 173 ·
`CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE` 147 · `CK_SIGNAL_BIND` 131 ·
`CK_REGISTER_SNAPSHOTABLE` 121 · `CK_GENERATED_BODY_HANDLE_TYPESAFE` 91. These are RAW line
counts — they include each macro's own `#define` and commented examples; sibling skills quote
call-site-only figures (130 `CK_SIGNAL_BIND` binds, 119 snapshot registrations, 171 request
debug-names, 90 typesafe-handle declarations) — both derivations are correct.

## 2. Deep dives

### 2.1 `CK_GENERATED_BODY(T)` — CkMacros.h:67-69

Expands to exactly two things:

1. `using ThisType = T;` — consumed by `CK_PROPERTY_UPDATE` (CkMacros.h:112), by the AS property
   registration (`using ClassType = ThisType;`, CkMacros_AngelScript.h:297), and by
   `FCk_Request_Base::Request_TransferHandleToOther(const ThisType&)` (CkRequest_Data.h:92).
2. `CK_ENABLE_CUSTOM_VALIDATION()` — friends `ck::IsValid_Executor` so custom validators can read
   private members (CkMacros.h:63-65).

**Ordering requirement (mechanical):** it must appear BEFORE any `CK_PROPERTY` /
`CK_PROPERTY_UPDATE` in the class — they reference `ThisType`. It does NOT interact with UHT:
place it right after `GENERATED_BODY()` in USTRUCTs/UCLASSes (canonical:
`CkTimer/Public/CkTimer/CkTimer_Fragment_Data.h:85-88`). It is used on UCLASSes too
(`CkTimer_Utils.h:40`) — only the constructor macro (§2.3) is banned on UObjects.

### 2.2 `CK_PROPERTY` family — CkMacros.h:72-145

The `Get_X` name is **emergent token-pasting, not string magic**: `CK_CONCAT(Get, _InVar_)` on a
member named `_Duration` yields `Get_Duration`. A member named `Duration` (no underscore) would
generate `GetDuration` — breaking house naming AND script parity, because the AS registration
strips the leading `_` to build the script-facing `Get_Duration` name
(CkMacros_AngelScript.h:318-319). **Members MUST start with `_`.**

| Macro | Generates | Notes |
|---|---|---|
| `CK_PROPERTY_GET(_X)` (:72-78) | `const auto& Get_X() const` | + AS getter registration in AS builds |
| `CK_PROPERTY_GET_BY_COPY(_X)` (:80-81) | `auto Get_X() const` (by value) | for members where a ref would dangle or copies are cheap |
| `CK_PROPERTY_GET_NON_CONST(_X)` (:83-89) | `auto& Get_X()` | mutable access; no AS registration |
| `CK_PROPERTY_GET_PASSTHROUGH(_X, Getter)` (:91-92) | `decltype(_X.Getter()) Getter() const` | forwards a nested getter |
| `CK_PROPERTY_GET_STATIC(_X)` (:94-100) | `static const auto& Get_X()` | statics |
| `CK_PROPERTY_SET(_X)` (:102-109) | `auto Set_X(const decltype(_X)& InValue) -> decltype(*this)&` | **returns `*this` → fluent builder chains** (`Params{}.Set_A(..).Set_B(..)`) |
| `CK_PROPERTY_UPDATE(_X)` (:111-116) | `auto Update_X(std::function<void(decltype(_X)&)>) -> ThisType&` | in-place mutation hook; **requires `ThisType`** ⇒ `CK_GENERATED_BODY` first |
| `CK_PROPERTY(_X)` (:118-131) | const getter + **non-const getter overload** + `Set_X` + `Update_X` | more than "getter+setter" — the non-const overload and `Update_X` are part of the deal |
| `CK_PROPERTY_AND_VAR(_GET)(Type, _X)` (:135-145) | declares `private: Type _X; public:` + accessors in one shot | near-dead convenience; prefer explicit members |

**AS side-effect (AS builds only):** each property macro additionally emits a
`static void RegisterAngelScriptProperty_<line>_<var>()` member plus a `static inline bool`
registrar that defers registration to AS-compile time via
`FCkAngelScriptPropertyFunctionRegistration` (CkMacros_AngelScript.h:293-361). Consequences:
these macros are only usable **inside a class/struct body** (they emit static members), and
registration is a fallback — it checks `TypeInfo->GetMethodByName(...)` first because UHT may have
already registered the method (:330-331). Depth: `ck-angelscript-interop`.

Semantics of getter-only vs getter+setter (essential vs optional param) are a doctrine rule —
root CLAUDE.md "Encapsulation". Canonical call site: `CkTimer_Fragment_Data.h:116-120`.

### 2.3 `CK_DEFINE_CONSTRUCTORS(T, ...)` — CkMacros.h:160-212

Expands to `T() = default;` plus one positional constructor whose parameters are
`decltype(_Member) _Member` (same names as the members) with `std::move` member-init, dispatched
by arg count (`NARG_`, :40-59) to `CK_DEFINE_CONSTRUCTOR_1..9`, plus
`CK_ANGELSCRIPT_CTOR_REGISTRATION` (AS constructor for the same signature).

Constraints, each with its reason:

- **Structs only — never UObject-derived classes.** Two mechanisms: (a) the AS branch registers a
  script constructor that **placement-news the type** — `new(Address) T(...)` at
  CkMacros_AngelScript.h:230 — which is illegal for UObjects (no CDO init, no GC registration);
  (b) UHT owns UObject construction, so the emitted `T() = default;` collides with the generated
  body [half (b) INFERRED from UHT semantics; the ban itself is stated at
  `CkCore/Public/CkCore/Macros/README.md:28` and empirically absolute across 541 call sites].
- **Members must be declared BEFORE the macro** — parameter types come from `decltype(_Member)`.
- **Max 9 parameters** — `_9` is the last variant defined (NARG_ counts to 63; no `_10+` exists).
  A 10th essential param means your struct is too big; split it.
- **The 1-arg form is `explicit`** (:161); 2..9 are not. No implicit single-value conversion.
- **Parameters are by-value + move** — members must be movable; callers pass prvalues cheaply.

Canonical: `CkTimer_Fragment_Data.h:123`
(`CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Timer_ParamsData, _Duration);` — essentials only, optionals
flow through `Set_*`). To define ONLY the positional ctor (no default), use `CK_DEFINE_CONSTRUCTOR`
directly (`CkEcs/Public/CkEcs/Handle/CkHandle.h:454` — an algo functor with no default state).

### 2.4 Ensure family — CkEnsure.h + CkBuildConfig.Build.cs

`CK_ENSURE(InExpression, InString, ...)` (CkEnsure.h:36-42) is an **expression returning bool**
(true = passed). On failure it calls `ck::ensure::Do_HandleFail` (fmt-style `{}` message via
`ck::Format_UE`, expression text, file, line) and then — only if the user chose "Break" AND a
debugger is attached — executes `PLATFORM_BREAK()` **textually at the call site** (deliberately
not inside a wrapper lambda, so the debugger lands in YOUR frame — comment :34-35). Rich failure
UX (fire-once per site, ignore lists, PIE-ID attribution, C++/BP/AS stacks, modal buttons) lives
in `CkCore/Public/CkCore/Ensure/CkEnsure.cpp`.

`CK_ENSURE_IF_NOT(InExpression, InFormat, ...)` (CkEnsure.h:44-53) is **an inverted `if`** — the
`{ ... }` block after it is the failure/recovery path. It has THREE compile modes:

| Active flag | Expansion | Effect |
|---|---|---|
| `CK_DISABLE_ENSURE_CHECKS=1` | `if constexpr(false)` | predicate NOT evaluated; recovery block compiled out; execution falls through as if passed |
| `CK_DISABLE_ENSURE_DEBUGGING=1` | `if (NOT (InExpression))` | predicate evaluated, recovery block runs, zero logging/dialog |
| both 0 | `if (NOT CK_ENSURE(...))` | full diagnostics + recovery block |

The flags are injected per-config by `CkModuleRules.SetBuildConfiguration`
(`CkBuildConfig/CkBuildConfig.Build.cs:65-198`) — NOT declared in any header (the Ensure README's
claims of `CkBuild_Macros.h` + `ensureAlwaysMsgf` at `CkCore/Public/CkCore/Ensure/README.md:37,56`
are both stale; trust the Build.cs). **This skill owns the full define matrix** — sibling skills
(`ck-build-and-env`, `ck-debugging-playbook`, `ck-change-control`) carry one-line summaries and
point here. Most are `CK_DISABLE_*` flags — **0 means the feature is ON**. Per-config Build.cs
blocks: Debug/DebugGame `:87-91`, Dev-editor `:109-113`, Dev-game `:122-126`, Test `:145-149`,
Shipping `:165-169`, Profile override `:190-195`.

| Define | Debug / DebugGame | Development (Editor) | Development (game) | Test | Shipping |
|---|---|---|---|---|---|
| `CK_DISABLE_ENSURE_CHECKS` | 0 | 0 | 0 | 0 | **0** |
| `CK_DISABLE_ENSURE_DEBUGGING` | 0 | 0 | 0 | 1 | 1 |
| `CK_DISABLE_LOG_CONTEXT` | 0 | 0 | **1** | 1 | 1 |
| `CK_DISABLE_STACK_TRACE` | 0 | 0 | 0 | 1 | 1 |
| `CK_DISABLE_ECS_HANDLE_DEBUGGING` | 0 | 0 | **1** | 1 | 1 |
| `CK_ENABLE_MEMORY_TRACKING` | 0 | 0 | 0 | 0 | 0 |
| `CK_DISABLE_NET_PARAM_COPY_PER_ENTITY` | 0 | 0 | 0 | 0 | 0 |
| `CK_DISABLE_STAT_DESCRIPTION` | 0 | 0 | 0 | 1 | 1 |
| `CK_DISABLE_GAMEPLAYTAG_STALENESS_VALIDATION` | 0 | 0 | **1** | 1 | 1 |
| `CK_DISABLE_ABILITY_SCRIPT_DEBUGGING` | 0 | 0 | **1** | 1 | 1 |
| `CK_BUILD_DEBUG_DRAW` | 1 | 1 | 1 | 1 | **0** |
| `CK_BUILD_SM_GRAPH_WALK` | 1 | 1 | 1 | 0 | 0 |
| `CK_BUILD_DEBUG` | 1 | 0 | 0 | 0 | 0 |
| `CK_BUILD_DEVELOPMENT` | 0 | 1 | 1 | 0 | 0 |
| `CK_BUILD_TEST` | 0 | 0 | 0 | 1 | 0 |
| `CK_BUILD_SHIPPING` | 0 | 0 | 0 | 0 | 1 |
| `CK_BUILD_TEST_OR_SHIPPING` | 0 | 0 | 0 | 1 | 1 |

Decision-changing notes:

- **Development-Editor ≠ Development-game** (split on `Target.bBuildEditor`, `:107`). Exactly four
  defines differ — `LOG_CONTEXT`, `ECS_HANDLE_DEBUGGING`, `GAMEPLAYTAG_STALENESS_VALIDATION`,
  `ABILITY_SCRIPT_DEBUGGING` (on in editor, off in game). PIE and a packaged Development build
  legitimately differ — check this table before treating a packaged-only difference as a bug
  (then load `ck-debugging-playbook`).
- These are `PublicDefinitions` — they propagate to every module that depends on a Ck module.
- `UnrealTargetConfiguration.Unknown` (`:71-84`) sets the twelve flag defines but NOT the
  `CK_BUILD_{DEBUG,DEVELOPMENT,TEST,SHIPPING,TEST_OR_SHIPPING}` family.
- The `BuildConfiguration.Profile` branch (`:190-195`) is dead code unless the source constant
  `BuildConfigurationOverride` (`:47`) is flipped from `MatchWithUnreal`.
- `CK_DISABLE_ABILITY_SCRIPT_DEBUGGING` has zero consumers outside CkBuildConfig — vestigial
  since the Ability modules were removed (DECISIONS.md #41).

Net effect on `CK_ENSURE_IF_NOT`: Debug/DebugGame/Development (editor or game) = full diagnostics;
Test/Shipping = silent guard — the block still runs; Profile override = guards vanish entirely.

Two rules fall out mechanically:

1. **Shipping keeps the checks.** `CK_DISABLE_ENSURE_CHECKS=0` in Test AND Shipping — the
   predicate evaluates and the recovery block executes in shipped builds, just silently. The
   recovery block must therefore be *correct silent-failure behavior*, not debug-only scaffolding
   (root non-negotiable #2/#3).
2. **The recovery block must be a pure bail-out** (`return`/`continue`/`return {};`). Under the
   Profile override (selectable only by editing the hardcoded const at Build.cs:47 —
   [possibly-vestigial], DECISIONS.md #26) the block is dead code; code with side effects in the
   block would change behavior across configs.

Siblings: `CK_TRIGGER_ENSURE(fmt, ...)` = `CK_ENSURE(false, ...)` (:68-69) — unconditional fire,
use in unreachable paths; `CK_TRIGGER_ENSURE_IF(expr, ...)` fires when expr is TRUE (:73-74);
`CK_INVALID_ENUM(EnumValue)` (:76-77) for `default:` arms of exhaustive switches;
`CK_ENSURE_VALID_IF_NOT(X)` (:81-82) stringifies X into the message;
`CK_ENSURE_VALID_IF_NOT_MSG(X, fmt, ...)` (:84-85) appends a custom message;
`CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(Ctx)` (:55-66) validates the context object then its world;
`CK_PURE_VIRTUAL(func, fallback)` (CkMacros.h:300-306) — UE `PURE_VIRTUAL` replacement that fires
`CK_TRIGGER_ENSURE` instead of crashing, `__VA_ARGS__` is the fallback statement (`return 0;`).

Canonical shape (single-statement guard on one line — root doctrine):

```cpp
CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Timer Handle [{}]"), InHandle)
{ return {}; }
```

### 2.5 Signal macros — CkSignal_Macros.h

Definition macros (granular :10-28, meta :34-42):

| Macro | Generates |
|---|---|
| `CK_DEFINE_SIGNAL(_API_, Name, Payload...)` (:10-12) | `struct FFragment_Signal_<Name> : ck::TFragment_Signal<Payload...>` |
| `CK_DEFINE_SIGNAL_WITH_DELEGATE(_API_, Name, Delegate, Payload...)` (:18-22) | **TWO fragments**: `FFragment_Signal_Delegate_<Name>` (PostFire `DoNothing`) and `FFragment_Signal_Delegate_<Name>_PostFireUnbind` (PostFire `Unbind`) — the post-fire behavior is a *type-level* property |
| `CK_DEFINE_SIGNAL_UTILS` / `_WITH_DELEGATE_UTILS` (:14-16, :24-28) | `UUtils_Signal_<Name>` (+ `..._PostFireUnbind`) static classes wrapping `ck::TUtils_Signal(_Delegate)` — these expose `Bind`/`Unbind`/`Broadcast` |
| `CK_DEFINE_SIGNAL_AND_UTILS(_API_, Name, Payload...)` (:34-36) | C++-only signal (no BP delegate) |
| `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(_API_, Name, Delegate, Payload...)` (:39-42) | the standard one: C++ signal + BP dynamic-delegate parity. Payload types repeat verbatim after the delegate name |

Canonical: `CkTimer_Fragment.h:96-103` — eight signals, e.g.
`CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerDone, FCk_Delegate_Timer, FCk_Handle_Timer, FCk_Chrono, FCk_Time);`

Bind/unbind macros (:44-64):

- `CK_SIGNAL_BIND(SignalUtils, Handle, Delegate, BindingPolicy, PostFireBehavior)` (:51-55) — a
  **runtime `if` on PostFireBehavior** routing to `SignalUtils::Bind` or the token-pasted
  `SignalUtils##_PostFireUnbind::Bind`. This is WHY the two fragment flavors exist, and why the
  first argument must be a bare utils class name (`ck::UUtils_Signal_X` works — pasting applies to
  the last token).
- `CK_SIGNAL_BIND_PROMISE` (:44-45) = `FireIfPayloadInFlight` + `Unbind` — fire-once, replays any
  PAST payload (a sticky promise read).
- `CK_SIGNAL_BIND_REQUEST_FULFILLED` (:48-49) = `IgnorePayloadInFlight` + `Unbind` — fire-once,
  future broadcasts only (the deferred-request completion pattern).
- `CK_SIGNAL_BIND_WITH_CONDITION` (:57-61) adds an invocation predicate.
  `CK_SIGNAL_UNBIND(SignalUtils, Handle, Delegate)` (:63-64) → `SignalUtils::Unbind`.

Binding-policy and replay semantics (which payloads replay on Bind, last-payload-only, PostFire
`Unbind` + replay never connects) are owned by `ckecs-architecture-contract` §5, quick reference
in root CLAUDE.md §Signals. Spell the enumerators exactly as root lists them —
`FireIfPayloadInFlightThisFrame` (a past doc typo'd a lowercase `t` and taught non-compiling code).

Canonical BindTo wrapper (**copy this one**):
`CkEntityCollection/Public/CkEntityCollection/CkEntityCollection_Utils.cpp:207-215` — the
UFUNCTION takes `UPARAM(ref)` typed handle + delegate + both enums, body is one `CK_SIGNAL_BIND`,
returns the handle. **Do NOT copy CkTimer's BindTo wrappers**: `CkTimer_Utils.cpp:432-444`
(`BindTo_OnDone`) accepts an `InPostFireBehavior` parameter but calls
`ck::UUtils_Signal_OnTimerDone::Bind(...)` directly — the PostFire argument is silently ignored
(zero `_PostFireUnbind`/`CK_SIGNAL_BIND` hits in that file, verified 2026-07-02). Observed
divergence, not a pattern.

Three environments for the same bind: C++ `CK_SIGNAL_BIND(ck::UUtils_Signal_OnTimerDone, ...)`;
BP node "[Ck][Timer] Bind To OnDone" (Category "Ck|Utils|Timer"); AS
`utils_timer::BindTo_OnDone(Timer, Delegate)` or mixin `Timer.BindTo_OnDone(Delegate)`.

### 2.6 Request macros — CkRequest_Data.h

Two bases, both **one-shot** (`PopulateRequestHandle` may be called at most once — comments
:16-18, :60-62; construct a fresh request per submission):

- `ck::FRequest_Base` (:19-55) — C++-only requests.
- `FCk_Request_Base` (:63-107) — USTRUCT, for anything BP/AS-exposed. Composes a
  `ck::FRequest_Base _RequestBase` member (:106).

`CK_REQUEST_DEFINE_DEBUG_NAME(T)` (:111-121): with `CK_DISABLE_ECS_HANDLE_DEBUGGING=1` expands to
**nothing**; otherwise to `protected: auto Get_RequestDebugName() const -> FName final`. Put it on
every request struct (root doctrine), immediately after `CK_GENERATED_BODY`.

**Vtable-variance constraint (:46, :95-103):** the bases' `virtual Get_RequestDebugName` +
`virtual ~FCk_Request_Base()` exist ONLY when handle debugging is on. Cross-reference the config
matrix in §2.4: request structs are **polymorphic in Debug and Dev-editor builds, and
non-polymorphic (no vptr, different `sizeof`) in Dev-noneditor/Test/Shipping**. Never memcpy,
binary-serialize, or `static_assert(sizeof(...))` a request across configs. Documented as a
binary-compat constraint without claiming intent (DECISIONS.md #27).

Full request idiom — canonical `FCk_Request_Timer_Jump` (`CkTimer_Fragment_Data.h:150-169`):
`USTRUCT(BlueprintType)` → `GENERATED_BODY()` → `CK_GENERATED_BODY` →
`CK_REQUEST_DEFINE_DEBUG_NAME` → private `UPROPERTY(..., meta=(AllowPrivateAccess = true))`
members → `CK_PROPERTY_GET` (essentials) / `CK_PROPERTY` (optionals) →
`CK_DEFINE_CONSTRUCTORS(T, essentials)`.

Completion-signal-at-scope-exit helper: `ck::MakeRequestResultGuard<TSignal>(InRequest, [&]{...})`
(:136-168) — declare the guard AFTER the locals its payload-builder captures.

### 2.7 Typesafe-handle macros — CkHandle_TypeSafe.h

A typed handle (`FCk_Handle_Timer`) is a macro-stamped USTRUCT subclass of `FCk_Handle_TypeSafe`,
not a template. The **one-line declaration ritual** (canonical `CkTimer_Fragment_Data.h:76-78`):

```cpp
USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKTIMER_API FCk_Handle_Timer : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Timer); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Timer);
```

`CK_GENERATED_BODY_HANDLE_TYPESAFE(T)` (:84-102) generates: friend declaration for
`ck::StaticCast`; `CK_GENERATED_BODY`; `using` of base `==`/`!=`/`<`; same-type `operator==` via
`ConvertToHandle()`; defaulted default ctor; move/copy ctors routing through `FCk_Handle_TypeSafe`;
copy-assign via `Swap`; `NetSerialize` forwarding to `Super`; and — the type-safety core — a
**`private:` constructor from `const FCk_Handle&`** (:101-102). Base→derived conversion therefore
does not compile except through the friended `ck::StaticCast` (:296-307) and the feature's
`Cast`/`CastChecked` utils. Derived→base is free (public inheritance).

`CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(T)` (:134-155) — the mandatory companion,
same header, immediately after the struct: stamps the formatter (entity+debug-name, validated
WITHOUT down-casting to `FCk_Handle` "for perf reasons", comment :133), re-asserts
`sizeof(T) == sizeof(FCk_Handle)` (:144-145 — typed handles must add ZERO data members;
`StaticCast` reinterprets in place), and specializes `TStructOpsTypeTraits<T>` with
`WithIdenticalViaEquality + WithNetSerializer` (:147-155 — without it, BP Set/Map compare handles
via reflection instead of `operator==`; rationale comment :63-67).

Utils-side cast machinery:

- `CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(HandleType)` (:159-190) — goes in the Utils class body
  (`CkTimer_Utils.h:41`): templated static `Cast` (returns `{}` if fragment missing) and
  `CastChecked` (ensures with message), plus AS conversion registration.
- `CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UtilsClass, HandleType, Fragments...)` (:195-242) —
  goes in the Utils **.cpp**: defines `Has` (= `Has_All<Fragments...>`), `DoCast`,
  `DoCastChecked`. The variadic tail is the identifying-fragment list for the Has check; the
  `FCk_Handle` parameter is by-value because by-ref breaks `BlueprintAutoCast` (comment :192-194).
  Canonical: `CkTimer_Utils.cpp:136`.

`CK_GENERATED_BODY_HANDLE_DERIVED(T, ParentHandleType)` (:109-129) — **advanced/rare** (7 uses +
the `#define` itself; DECISIONS.md #28): for handle-inheriting-handle hierarchies. Parameterizes the parent so ctors
initialize it (the TYPESAFE macro hardcodes `FCk_Handle_TypeSafe`) and plants
`using MixinParentHandle = Parent;` (:127), which the AS registration walks to propagate mixin
methods (:311-325). Default to the flat TYPESAFE form.

**Placement rule:** declare typed handles in `X_Fragment_Data.h`, never `X_Fragment.h`. Verified
2026-07-02 (counting FILES): 74 files contain `CK_GENERATED_BODY_HANDLE_TYPESAFE`; 72 are
`*_Fragment_Data.h`; the two exceptions are the base header itself (the `#define`) and
`CkShapes/Public/CkShapes/CkShape_Handle.h` (a dedicated handle header — still not a
`_Fragment.h`). Counting DECLARATIONS, the same census reads 90 (89 in `_Fragment_Data.h` + 1 in
`CkShape_Handle.h`) — root doctrine and `ckecs-architecture-contract` quote that form.

### 2.8 Tag macros — CkTag.h (self-documenting header — read its comments)

| Macro | Expansion | When |
|---|---|---|
| `CK_DEFINE_ECS_TAG(FTag_X)` (:23-26) | empty struct + `static_assert(std::is_empty_v)` + a `[[maybe_unused]] static inline const bool` registrar that auto-registers the tag as **snapshotable** | default. Opt-OUT model: every tag round-trips a save unless transient |
| `CK_DEFINE_ECS_TAG_TRANSIENT(FTag_X)` (:34-36) | same struct, NO snapshot registration | one-shot restore-lifecycle done-markers. Letting those round-trip is a correctness trap: a save taken after a load captures the done marker, so the next load silently skips every ReplicateOnRestore / re-drive pass (comment :28-33) |
| `CK_DEFINE_ECS_TAG_COUNTED(FTag_X)` (:38-40) | inherits `ck::FTag_CountedTag`; asserted `sizeof == sizeof(int32)` | add/remove depth tracking |

`static inline` (not plain `inline`) because tags are defined at BOTH namespace scope and nested
in class bodies (plain inline variable in a class body = C7524); registration dedupes by entt
type-hash and is static-init-safe (comment :15-22).

### 2.9 Record / EntityHolder macros — poisoned policy-blind forms

`CK_DEFINE_RECORD_OF_ENTITIES(...)` (CkRecord_Fragment.h:102-105) and
`CK_DEFINE_ENTITY_HOLDER(...)` (CkEntityHolder_Fragment.h:61-64) are
**`static_assert(false)` tombstones**. If you hit
`"CK_DEFINE_RECORD_OF_ENTITIES is removed: choose ..."` you (or the doc you copied from) used the
removed form. Choose explicitly:

- `_ROUNDTRIP` (CkRecord_Fragment.h:115-117) — authoritative state that must survive save/load.
  Additionally emits `CK_SNAPSHOT_ANNOUNCE_EXPECTED` for the capture-time audit, and **obligates
  you** to add `CK_REGISTER_SNAPSHOTABLE` in the .cpp (§3.6). Example of why it matters:
  a TRANSIENT record of timers left restored timers as orphaned, still-ticking entities invisible
  to their owner (`CkTimer_Fragment.h:89-92`).
- `_TRANSIENT` (:119-120) — state reconstructed on restore; not captured.

Both expand through `_WITH_POLICY` to a struct inheriting
`TFragment_RecordOfEntities<HandleType, Policy>` / `TFragment_EntityHolder<HandleType, Policy>`.
`_AND_UTILS` variants (CkRecord_Utils.h / CkEntityHolder_Utils.h) also stamp the utils class.

### 2.10 `CK_REGISTER_PROCESSOR` — CkProcessorRegistration.h:89-91

Expands to a file-scope `static ck::FAutoProcessorRegistrar<T>` named via `__COUNTER__` paste. The
registrar self-registers a descriptor + factory lambda into the global `FProcessorRegistry` at
static init and deregisters in its dtor unless the engine is exiting (:12-50). One line per
processor at the top of the feature's `*_Processor.cpp` — canonical `CkTimer_Processor.cpp:10-13`.
`_WITH_FACTORY` (:93-95) forwards a custom factory; `CK_REGISTER_GROUP` (:85-87) is the same for
scheduler groups. The old ProcessorInjector mechanism is retired (DECISIONS.md #8) — any doc
mentioning it is stale.

### 2.11 `CK_REGISTER_SNAPSHOTABLE(T)` — CkSnapshot_FragmentRegistry.h:138-152

One line in the fragment's `*_Fragment.cpp`. Expands to: a `static_assert` that T satisfies one of
the two serialization tiers (§3.6), then an **anonymous-namespace** auto-registrar struct whose
name token-pastes the fragment type (deliberately not `__LINE__` — unity builds concatenate TUs
and collide line numbers, comment :134-135).

Two mechanical traps:

1. **Token-pasting can't paste `::`** — a `ck::`-qualified argument won't compile. Hoist a
   file-scope alias first: `using FSnap_Timer_Current = ck::FFragment_Timer_Current;` then
   `CK_REGISTER_SNAPSHOTABLE(FSnap_Timer_Current);` (`CkTimer_Fragment.cpp:27-34`).
2. **Never resolves `T::StaticStruct()` at registration** — registration runs during static init
   at DLL load, before the module's `/Script/<Module>` package exists; resolving then trips the
   `FoundPackage` assert. The registry defers to capture/restore time (comment :104-110). Keep
   this property if you ever touch the registrar.

**Stale-binary trap (operational, one line):** registration is a *global static* side effect of
the compiled binary — a green test run against a binary older than your edit proves nothing. Full
telling + recovery steps: `ck-debugging-playbook`. Gates and test invocation:
`ck-tests-authoring-and-running`; build: `ck-build-and-env`.

### 2.12 `CK_DEFINE_LOG_FUNCTIONS(Category)` — CkLog_Utils.h:106

Generates namespace-scope variadic template functions `Fatal/Error/Warning/Display/Log/Verbose/
VeryVerbose` (+ `FatalIf/ErrorIf/WarningIf` returning `ECk_LogResults`) forwarding to `UE_LOG`
with a `[PIE-ID %d]` prefix and fmt-style `{}` formatting via `ck::Format_UE`. Wrap it in your
module's namespace (`namespace ck::timer { ... }`) — usage note :101-104. Pair with
`CK_REGISTER_LOG_FUNCTIONS(Category)` (:338-360), which injects the category into the runtime
log map AND auto-binds Log/Display/Warning/Error/Verbose/VeryVerbose to AngelScript under the
same namespace (:373-398; namespace auto-derived via a sentinel struct + ctti — zero config).
`CK_LOG_ERROR_IF_NOT(Namespace, Expr, Fmt, ...)` (:408-409) = log-and-branch (an inverted if);
`CK_LOG_ERROR_NOTIFY_IF_NOT` (:412-450) adds an editor toast + MessageLog entry in editor builds,
degrades to plain `CK_LOG_ERROR_IF_NOT` otherwise. Remember root non-negotiable #3: a log where an
ensure belongs is a review rejection.

### 2.13 `CK_DEFINE_CUSTOM_FORMATTER_ENUM(E)` — CkFormat.h:288-300

Registers a `ck::Format`/`ck::Format_UE` formatter for the enum via
`UEnum::GetDisplayValueAsText` and specializes `ck::FEnumToString<E>`. Place directly after every
UENUM declaration (root doctrine; canonical `CkTimer_Fragment_Data.h:17-27` — enum then macro,
every time). Without it, formatting the enum in an ensure/log message is a compile error (by
design — the formatter primary template is not defined for unknown types).

### 2.14 `NOT` and the `ck::IsValid` machinery — CkMacros.h:31, CkIsValid.h

`#define NOT !` (CkMacros.h:31). It is object-like, so it also works in preprocessor context —
`#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING` (CkRequest_Data.h:46,95) is legal and used.

`ck::IsValid` dispatch (CkIsValid.h):

- Policies are empty structs deriving `ck::IsValid_Policy` (:16-20), declared via
  `CK_DEFINE_CUSTOM_IS_VALID_POLICY(P)` (:24-28). Stock policies: `IsValid_Policy_Default`
  (:30), plus `IsValid_Policy_IncludePendingKill`, `IsValid_Policy_NullptrOnly`,
  `IsValid_Policy_OptionalEngagedOnly` (`CkIsValid_Defaults.h:54-56`).
- `ck::IsValid(Obj [, Policy{}])` (:68-80) instantiates `IsValid_Executor<decayed_type, Policy>`.
  The primary template is `std::false_type` with a **deleted** `IsValid` (:44-49) — calling
  IsValid on a type with no registered validator is a compile error, not a silent true.
  `ck::Is_NOT_Valid` = `NOT IsValid` (:82-92).
- Register a validator: `CK_DEFINE_CUSTOM_IS_VALID_INLINE(Type, Policy, Lambda)` (:149-163,
  header-only, also emits the AS binding); DECLARE/DEFINE split for .h/.cpp pairs (:166+); `_T`
  templated variant; `CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T` (:108-124) teaches base-of matching
  to wrapper templates (applied to TSubclassOf/TWeakObjectPtr/TObjectPtr/… in CkIsValid_Defaults.h);
  `CK_DELETE_CUSTOM_IS_VALID` (:130-144) is a poison-pill for types that must never be validated.
- `IsValid_Policy_NullptrOnly` is a generic `T*` null-check only (`CkIsValid_Defaults.h:105-112`)
  — for RAW pointers when you deliberately skip UObject pending-kill checks. Smart pointers have
  their own overloads; pass them bare (root doctrine).
- Handles: `FCk_Handle` registers Default + IncludePendingKill inline (CkHandle.h:477-485); typed
  handles get theirs from the ISVALID_AND_FORMATTER macro (§2.7). Pending-kill idiom:
  `CK_IF_HANDLE_IS_PENDING_KILL(H)` = invalid under Default but valid under IncludePendingKill
  (CkHandle.h:395-396).

Processor lifetime filters (used in the `TProcessor<...>` parameter list, not as statements):
`CK_IGNORE_PENDING_KILL` (excludes all four destroy-phase tags — normal processors),
`CK_IF_END_PLAY`, `CK_IF_TEARING_DOWN` (teardown passes) —
`CkEntityLifetime_Fragment.h:37-50`. Lifecycle semantics: `ckecs-architecture-contract`.

## 3. Add-a-new-X checklists

All derived from the CkTimer quartet — the canonical smallest complete feature
(`CkTimer/Public/CkTimer/`: `CkTimer_Fragment_Data.h`, `CkTimer_Fragment.h/.cpp`,
`CkTimer_Processor.h/.cpp`, `CkTimer_Utils.h/.cpp`). Read it before authoring; mimic, don't
invent (root non-negotiable #1). Naming table: root CLAUDE.md "ECS naming is two-tier".

Build gate for every checklist below: compile per `ck-build-and-env`, then grep-verify each
registration line exists. Anything that needs PIE is `[EDITOR-VERIFY]` and marked as such — run
every `[EDITOR-VERIFY]` tag below via the exact Blueprint checklist in `ck-change-control`
§"Three environments" (its numbered steps cover the accessor-visibility, autocast-node,
request-node, and BindTo-node checks these checklists tag).

### 3.1 New fragment (ParamsData + runtime + alias)

1. `Ck<Feature>/Public/Ck<Feature>/Ck<Feature>_Fragment_Data.h` — the reflected config:
   - UENUMs first, each followed by `CK_DEFINE_CUSTOM_FORMATTER_ENUM(E);` (exemplar :17-27).
   - `FCk_Fragment_<Feature>_ParamsData` USTRUCT: `GENERATED_BODY()` → `CK_GENERATED_BODY` →
     (optional `using IsSnapshotable = void;` for Tier-A round-trip, §3.6) → private
     `UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))` members →
     `CK_PROPERTY_GET` for essentials / `CK_PROPERTY` for optionals →
     `CK_DEFINE_CONSTRUCTORS(T, essentials)` last (exemplar :82-124).
   - `#include "Ck<Feature>_Fragment_Data.generated.h"` is the LAST include (UHT rule).
   - Verify: UHT compiles; the struct shows in BP with Get_/Set_ accessors `[EDITOR-VERIFY]`.
2. `Ck<Feature>_Fragment.h` — the runtime side, all inside `namespace ck`:
   - Lifecycle tags via `CK_DEFINE_ECS_TAG(FTag_<Feature>_NeedsSetup);` etc. (exemplar :27-29).
   - Bridge alias: `using FFragment_<Feature>_Params = FCk_Fragment_<Feature>_ParamsData;` (:33).
   - `FFragment_<Feature>_Current` — plain struct, NOT a USTRUCT: `CK_GENERATED_BODY` → friend
     its processors + Utils → private state → `CK_PROPERTY_GET` → `CK_DEFINE_CONSTRUCTORS`
     (exemplar :37-63). Friends are the ONLY writers of `_Members` (root doctrine).
   - Verify: header compiles standalone (include it from the .cpp first).
3. `Ck<Feature>_Fragment.cpp` — snapshot registrations if any (§3.6; exemplar :27-34).
4. Before landing: this is a class-2 (additive API) change — finish via `ck-change-control`'s
   done-checklist.

### 3.2 New processor

1. `Ck<Feature>_Processor.h`, in `namespace ck` — subclass the CRTP processor:
   ```cpp
   class CK<FEATURE>_API FProcessor_<Feature>_Setup : public ck_exp::TProcessor<
       FProcessor_<Feature>_Setup,
       FCk_Handle_<Feature>,
       ck::TReadOnly<FFragment_<Feature>_Params>,
       ck::TReadWrite<FFragment_<Feature>_Current>,
       FTag_<Feature>_NeedsSetup,
       CK_IGNORE_PENDING_KILL>
   {
   public:
       using Group = FGroup_Gameplay_TimeDelta;
       using MarkedDirtyBy = FTag_<Feature>_NeedsSetup;

   public:
       using TProcessor::TProcessor;

   public:
       static auto
       ForEachEntity(
           TimeType InDeltaT,
           HandleType InEntity,
           const FFragment_<Feature>_Params& InParams,
           FFragment_<Feature>_Current& InCurrent)
           -> void;
   };
   ```
   (Exemplar `CkTimer_Processor.h:13-36`.) `MarkedDirtyBy` = the tag/requests fragment whose
   presence wakes the processor; the body MUST consume it (`InEntity.Remove<MarkedDirtyBy>();`)
   or opt out of pumping — semantics in `CkEcs/Claude.md` "Pump policy". Order relative to
   siblings with `using RunAfter = TDepList<FProcessor_<Feature>_Setup>;` (:47).
2. `Ck<Feature>_Processor.cpp` — registration at the top, then definitions:
   `CK_REGISTER_PROCESSOR(ck::FProcessor_<Feature>_Setup);` (exemplar :10-13). No other wiring —
   registration is the static registrar (§2.10).
3. Verify: `rg -n 'CK_REGISTER_PROCESSOR\(ck::FProcessor_<Feature>' Plugins/CkFoundation/Source`
   shows the line; build passes; behavior gate via an AutoTest (`ck-tests-authoring-and-running`).
4. Before landing: classify (class 2 if purely additive; class 3 if an existing processor's
   behavior changed) and finish via `ck-change-control`'s done-checklist.

### 3.3 New typesafe handle

1. `_Fragment_Data.h`: the one-line ritual + companion macro (§2.7; exemplar :76-78). Use
   `CK_GENERATED_BODY_HANDLE_DERIVED(T, Parent)` only for handle-inheriting-handle (rare).
2. Utils class header: `CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_<Feature>);` in the class
   body (exemplar `CkTimer_Utils.h:41`), plus the declared UFUNCTIONs `Has_Any`/`DoCast`/
   `DoCastChecked`/`Get_InvalidHandle` (copy the shapes at `CkTimer_Utils.h:87-121` — including
   `meta = (ExpandEnumAsExecs = "OutResult")` on DoCast and `BlueprintAutocast` on DoCastChecked).
3. Utils .cpp: `CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_<Feature>_UE,
   FCk_Handle_<Feature>, ck::FFragment_<Feature>_Current, ck::FFragment_<Feature>_Params);`
   (exemplar :136). The fragment list defines what `Has` means — pick the fragments that are
   present exactly when the feature is composed.
4. Verify: compiles; `ck::StaticCast<FCk_Handle_<Feature>>(GenericHandle)` compiles in feature
   code but a plain implicit conversion does NOT (try it, then delete it); BP shows the
   "<As<Feature>>" autocast node `[EDITOR-VERIFY]`.
5. Before landing: this is a class-2 (additive API) change — finish via `ck-change-control`'s
   done-checklist.

### 3.4 New request (+ Utils UFUNCTION surface)

0. **Feature's first request?** Create the plumbing steps 2-3 assume before following them:
   (a) in `_Fragment.h`, the `FFragment_<Feature>_Requests` fragment — private
   `RequestList _Requests` + the `RequestType` variant, friends = HandleRequests processor + Utils
   (shape: `CkTimer_Fragment.h:67-85`); (b) a `HandleRequests` processor whose view takes
   `ck::TReadWrite<FFragment_<Feature>_Requests>` and `TExclude<FTag_<Feature>_NeedsSetup>`, with
   `using MarkedDirtyBy = FFragment_<Feature>_Requests;` (exemplar `CkTimer_Processor.h:40-60`);
   (c) the drain loop: copy `_Requests`, `Reset()` the member, dispatch the copy via
   `ck::Visitor`, then `Remove<MarkedDirtyBy>()` ONLY if `_Requests` is empty afterward —
   handlers may re-enqueue, and removing the dirty marker with requests pending puts the
   processor to sleep with work queued (exemplar `CkTimer_Processor.cpp:44-66`).
1. `_Fragment_Data.h`: the request struct per the §2.6 idiom (exemplar :150-169). Base:
   `FCk_Request_Base` if BP/AS-facing (normal case), `ck::FRequest_Base` if C++-only.
2. `_Fragment.h`: add the type to the variant —
   `using RequestType = std::variant<..., FCk_Request_<Feature>_<Action>>;` inside
   `FFragment_<Feature>_Requests` (exemplar :67-85; the fragment holds a private
   `RequestList _Requests` + `CK_PROPERTY_GET(_Requests)`, friends = HandleRequests processor +
   Utils).
3. Processor: add a `DoHandleRequest(TimeType, HandleType, Current&, const Params&,
   const FCk_Request_<Feature>_<Action>&) -> void` overload (exemplar `CkTimer_Processor.h:63-85`)
   — the drain loop dispatches via `ck::Visitor` with ONE generic lambda (exemplar
   `CkTimer_Processor.cpp:48-66`; `ck::Visitor` is not a std::visit overload set —
   `Source/CLAUDE.md` "Variant dispatch").
4. Utils: the `Request_<Action>` UFUNCTION takes `UPARAM(ref) FCk_Handle_<Feature>&` (+ request
   struct or the trivial datum), enqueues, returns the handle (chainable):
   ```cpp
   CK_CALLSTACK_RECORD(ck::FFragment_<Feature>_Requests, InEntity);

   InEntity.AddOrGet<ck::FFragment_<Feature>_Requests>()._Requests.Emplace(
       FCk_Request_<Feature>_<Action>{...});

   return InEntity;
   ```
   (Exemplar `CkTimer_Utils.cpp:274-284`. `CK_CALLSTACK_RECORD` only if the feature opted into
   callstack tracing via `CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR` — `CkTimer_Fragment.h:105`.)
5. Verify all three environments: C++ call compiles; BP node "[Ck][<Feature>] Request <Action>"
   appears `[EDITOR-VERIFY]`; AS `utils_<feature>::Request_<Action>(...)` resolves after the
   generated `Script/Generated/utils_<feature>.as` refreshes (→ `ck-angelscript-interop`).
   Behavior: an AutoTest that enqueues then settles a tick (mutations are deferred).
6. Before landing: this is a class-2 (additive API) change — finish via `ck-change-control`'s
   done-checklist.

### 3.5 New signal (+ BindTo wrappers)

1. `_Fragment_Data.h`: declare the dynamic delegate — payload = typed handle + data
   (exemplar :242-247 `DECLARE_DYNAMIC_DELEGATE_ThreeParams(FCk_Delegate_Timer, ...)`).
2. `_Fragment.h`, inside `namespace ck`:
   `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CK<FEATURE>_API, On<Event>, F<Delegate>,
   <PayloadTypes...>);` — payload types verbatim, matching the delegate params (exemplar :96-103).
3. Broadcast from the processor that detects the condition:
   `UUtils_Signal_On<Event>::Broadcast(InEntity, ck::MakePayload(InEntity, ...));`
   (exemplar `CkTimer_Processor.cpp:135`).
4. Utils: `BindTo_On<Event>` / `UnbindFrom_On<Event>` UFUNCTIONs. **Copy
   `CkEntityCollection_Utils.cpp:207-215`** — body is `CK_SIGNAL_BIND(ck::UUtils_Signal_On<Event>,
   InHandle, InDelegate, InBindingPolicy, InPostFireBehavior); return InHandle;` — NOT CkTimer's
   wrappers (they drop PostFireBehavior, §2.5). Defaults on the UFUNCTION params:
   `FireIfPayloadInFlightThisFrame` + `DoNothing` (exemplar decoration `CkTimer_Utils.h:251-259`).
5. Verify: broadcast-then-bind replays per policy (AutoTest); BP "[Ck][<Feature>] Bind To
   On<Event>" node `[EDITOR-VERIFY]`; AS mixin `Handle.BindTo_On<Event>(Delegate)` resolves.
6. Before landing: this is a class-2 (additive API) change — finish via `ck-change-control`'s
   done-checklist.

### 3.6 Make a type snapshotable

Decision first — which tier (concepts: `CkEcs/Public/CkEcs/Concepts/CkSnapshot_Concepts.h:29-50`):

| Tier | Applies to | You write |
|---|---|---|
| A | USTRUCT fragments whose bare UPROPERTYs fully describe them | `using IsSnapshotable = void;` in the struct — UHT reflection serializes it (exemplar `CkTimer_Fragment_Data.h:90-92`) |
| B/C | plain `ck::` fragments, or anything holding entity handles | `using IsSnapshotable = void;` + a member `auto SerializeSnapshot(FArchive& InAr, ck::FSnapshotContext& InCtx) -> void;` — route every handle through `InCtx.Snapshot_Handle(InAr, Handle)` so it remaps on restore (exemplar `CkTimer_Fragment.h:59-62` + `CkTimer_Fragment.cpp:38-47`; handle-remap example `CkRecord_Fragment.h:73-97`) |

Then, in the fragment's `.cpp`:

1. Include `CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h` AND the archive writer/reader headers
   (the closures need complete types — `CkTimer_Fragment.cpp:9-13`).
2. Hoist a file-scope alias for any `ck::` type (token-paste, §2.11), then
   `CK_REGISTER_SNAPSHOTABLE(FSnap_<Feature>_Current);`.
3. Tags: nothing to do — `CK_DEFINE_ECS_TAG` auto-registers (opt-OUT). If the tag is a one-shot
   restore done-marker, it must be `CK_DEFINE_ECS_TAG_TRANSIENT` instead (§2.8).
4. Records/holders: if the child entities round-trip, the record must too — use the `_ROUNDTRIP`
   define AND register it (§2.9; `CkTimer_Fragment.cpp:33-34`).
5. The static_assert names the exact fix if you got the tier wrong ("provides no serialization
   path: declare ... OR make ... a USTRUCT").
6. Verify: **rebuild before trusting any green test** — registration is a global static (§2.11
   stale-binary trap). Then run the snapshot tests (`ck-tests-authoring-and-running`); the
   `CK_SNAPSHOT_ANNOUNCE_EXPECTED` audit flags an announced-but-unregistered roundtrip type.
7. Before landing: class 2 for registering your own new type — but touching the snapshot
   core/format itself is class 4. Finish via `ck-change-control`'s done-checklist.

## Common mistakes

1. **Member without leading `_`** → accessors come out `GetFoo` and the AS name-strip does
   nothing. Rename the member, not the macro.
2. **`CK_PROPERTY_UPDATE`/`CK_PROPERTY` before `CK_GENERATED_BODY`** → "ThisType: undeclared
   identifier". Move `CK_GENERATED_BODY` up.
3. **`CK_DEFINE_CONSTRUCTORS` on a UCLASS/AActor** → default-ctor redeclaration errors and (in AS
   builds) placement-new codegen on a UObject. Structs only (§2.3).
4. **Recovery block with side effects after `CK_ENSURE_IF_NOT`** → behavior differs across build
   configs (§2.4). Pure bail-out only.
5. **Assuming ensures vanish in Shipping** → they don't (CHECKS=0 in Shipping); only the
   diagnostics do. Don't write recovery blocks as "unreachable".
6. **`CK_REGISTER_SNAPSHOTABLE(ck::FFoo)`** → token-paste error on `::`. File-scope alias first.
7. **Copying `CK_DEFINE_RECORD_OF_ENTITIES(...)` from an old doc** → `static_assert(false)`
   tombstone. Pick `_ROUNDTRIP` or `_TRANSIENT` (§2.9).
8. **Hand-writing `BindTo_*` that forwards to `Utils::Bind` directly** → PostFireBehavior silently
   ignored (the Unbind flavor is a different generated class). Use `CK_SIGNAL_BIND` (§2.5).
9. **Trusting a green test from a binary older than your registrar edit** → global static
   registration means the old binary registered the old set. Rebuild, re-run (§2.11).
10. **memcpy/static_assert(sizeof) on request structs** → vptr comes and goes with
    `CK_DISABLE_ECS_HANDLE_DEBUGGING` (§2.6).
11. **Adding data members to a typed handle** → `static_assert(sizeof == sizeof(FCk_Handle))`
    fires (twice, deliberately). Typed handles are views, never containers (§2.7).
12. **New UENUM without `CK_DEFINE_CUSTOM_FORMATTER_ENUM`** → first `{}`-format of it fails to
    compile. Add the macro right below the enum.
13. **Typed handle declared in `_Fragment.h`** → breaks the UHT-facing/runtime split. It goes in
    `_Fragment_Data.h` (72/74 files comply; §2.7).

## Provenance and maintenance

Campaign 2026-07-02. Everything above was read from source that day (engine:
UnrealEngine-Angelscript 5.7.4; EnTT 3.16.0 — per root CLAUDE.md). Macro definitions move;
re-verify before trusting long after that date. From `d:\Repos\BusterBlock` (PowerShell or
Git Bash; the Grep tool is fine for `Source/` but use `rg --no-ignore` under `Script/`):

- Census: `rg -c '#define CK_' Plugins/CkFoundation/Source --glob '*.{h,inl,hpp}'` (sum ≈ 273);
  unique names: `rg -o '#define (CK_[A-Za-z0-9_]+)' -r '$1' Plugins/CkFoundation/Source --glob '*.{h,inl,hpp}' --no-filename | sort -u | wc -l` (≈ 247).
- Any macro's current definition: `rg -n 'define <NAME>' Plugins/CkFoundation/Source -A5`.
- Ensure config matrix: `rg -n 'CK_DISABLE_ENSURE' Plugins/CkFoundation/Source/CkBuildConfig/CkBuildConfig.Build.cs`.
- Call-site frequencies: `rg -o '<MACRO>\(' Plugins/CkFoundation/Source --glob '*.{h,cpp,inl}' --no-filename | wc -l`.
- Typed-handle placement: `rg -ln 'CK_GENERATED_BODY_HANDLE_TYPESAFE' Plugins/CkFoundation/Source | rg -v '_Fragment_Data\.h'` (expect only the base header + CkShape_Handle.h).
- Binding-policy enumerators: `rg -n 'enum class ECk_Signal_BindingPolicy' -A10 Plugins/CkFoundation/Source/CkEcs`.
- Tombstones still poisoned: `rg -n 'static_assert\(false' Plugins/CkFoundation/Source/CkRecord Plugins/CkFoundation/Source/CkEcsExt`.
- CkTimer wrapper divergence (drop this skill's warning if fixed):
  `rg -c 'CK_SIGNAL_BIND|_PostFireUnbind' Plugins/CkFoundation/Source/CkTimer/Public/CkTimer/CkTimer_Utils.cpp` (0 hits = still divergent).
- Stale READMEs called out here: `rg -n 'ensureAlwaysMsgf|CkBuild_Macros' Plugins/CkFoundation/Source/CkCore/Public/CkCore/Ensure/README.md` (hits = still stale).
