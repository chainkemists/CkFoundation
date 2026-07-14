# CLAUDE.md — CkFoundation (doctrine of record)

CkFoundation is the Chainkemists ECS framework for Unreal Engine. This file is the always-loaded
doctrine for the whole Ck plugin suite: **CkGameplayDebugger and CkTests defer to this file** for
everything not plugin-specific. Style rules live HERE and only here; topology lives in
[Source/CLAUDE.md](Source/CLAUDE.md); AngelScript language rules live in
[Script/CLAUDE.md](Script/CLAUDE.md); depth (runbooks, rationale, incident history) lives in the
skill library (index at the bottom). Facts below marked with a date were verified against
code/disk on that date.

## Identity (verified 2026-07-02)

- **Engine:** UnrealEngine-Angelscript **5.7.x** (Hazelight AngelScript fork; 5.7.4 on disk). NOT
  stock UE — AngelScript is a first-class consumer of every public API.
- **ECS backend:** EnTT **3.16.0**, vendored at `Source/CkThirdParty/Public/CkThirdParty/entt-3.16.0/`
  (also vendored: JoltPhysics, fmt, cleantype, ctti, delegate, bitwise-enum). Note: `CkHandle.h:71-77`
  globally specializes `entt::component_traits` with `in_place_delete = true` — every fragment pool
  is tombstone-mode (pointer-stable, owning groups unavailable), inverting EnTT's swap-and-pop
  default. This is deliberate design (`06938bba3`, "fragments are always pointer stable") — see
  `.claude/reports/DECISIONS.md` §45. Details: `ckecs-domain-reference` skill; perf implications:
  `ck-feature-frontier` candidate 5.
- **Scale:** 101 modules in `CkFoundation.uplugin` (72 Runtime, 25 UncookedOnly, 4 Editor —
  CkVat/CkVatEditor added 2026-07-09) + 3
  unlisted support dirs (CkBuildConfig, CkSettings, CkScripts). C++20 via the shared `CkModuleRules`
  base (`Source/CkBuildConfig/CkBuildConfig.Build.cs`).
- **AngelScript is optional at build time:** the uplugin dependency is `Optional: true`;
  `CkModuleRules` auto-detects the engine's Angelscript plugin and sets `WITH_ANGELSCRIPT_CK` 1/0.
  Code that touches AS bindings must compile both ways.

## Non-negotiables

These are the rules the maintainer actively enforces in review. Violating them is the most common
failure mode of incoming engineers and models — read them twice.

1. **Research before writing.** Read the neighboring feature before authoring anything: for ECS work
   that means one full feature quartet (`CkTimer` is the canonical small one) plus the target
   module's `Claude.md`. Mimicry of adjacent code beats invention. If your change doesn't resemble
   the code around it, you have not researched enough.
2. **`CK_ENSURE_IF_NOT` — never stock `ensure`/`ensureMsgf`/`check` for recoverable validation.**
   The Ck ensure logs with context, breaks at the call site, fires once, and stays active in
   Shipping (checks are NOT compiled out by default — see `ck-macros-and-codegen` skill).
3. **Never silently handle an error.** A `Warning`/`Error` log-and-continue where validation failed
   is a review rejection: logs get ignored, ensures do not. Fire `CK_ENSURE_IF_NOT` with a
   recovery block that is a *correct* silent-failure path, or let it be loud. No fallbacks that
   hide problems, ever, unless explicitly requested.
4. **Three environments.** Every public API must work — and be verified — in C++, Blueprint, AND
   AngelScript. "Works in C++" is one third of done.
5. **Requests are deferred.** Mutations to ECS state go through request fragments handled by
   processors. Utility functions enqueue; processors mutate.
6. **Unwritten-norm forks: ask the maintainer.** If code and docs are silent and two reasonable
   conventions exist, do not invent policy — ask, or add the fork to
   [.claude/reports/ADJUDICATIONS.md](.claude/reports/ADJUDICATIONS.md).
7. **Measure before claiming.** No performance claims without a benchmark; no "fixed" without the
   gate re-run. Agents cannot launch the editor or PIE — anything only verifiable there is labeled
   `[EDITOR-VERIFY]` with exact manual steps for a human.
8. **Clarity over cleverness.** If a reader needs a comment to follow the code, rename or restructure
   instead (see comment rules below).

## Lingo

| Term | Meaning here |
|---|---|
| Entity | EnTT entity, addressed only through handles |
| Fragment | ECS component (UE already owns the word "Component") |
| Processor | ECS system; iterates entities via `ForEachEntity` |
| Handle | `FCk_Handle` — entity + registry ref; typesafe subtypes `FCk_Handle_[Feature]` |
| Request | Deferred mutation struct, queued on a `_Requests` fragment |
| Signal | Ck event (fragment-based), bound via `CK_SIGNAL_BIND` / `BindTo_On*` |
| Probe | Spatial-query trigger volume (`CkSpatialQuery/Probe`, Jolt-backed) |
| Record | Fragment holding child/related entity collections (`CkRecord`) |
| Utils / BPFL / BFL | `UCk_Utils_[Feature]_UE` BlueprintFunctionLibrary — the ONLY public API surface of a feature (BPFL and BFL are the same thing) |
| ParamsData | Reflected `FCk_Fragment_[Feature]_ParamsData` USTRUCT (BP/AS-facing config) |
| Quartet | The four files every feature ships: `X_Fragment_Data.h`, `X_Fragment.h`, `X_Processor.h/.cpp`, `X_Utils.h/.cpp` |
| CDO | Class Default Object — UE's per-class template instance; AS `default` statements write CDO values |
| EntityScript | `UCk_EntityScript_UE` — data-driven entity logic unit (C++/BP/AS) |
| ContextOwner | Entity's DI-style context root (`UCk_Utils_ContextOwner_UE`); CkProvider = data-asset value providers. There is no module named "DI" — these two are it |
| UHT / UBT | UnrealHeaderTool / UnrealBuildTool |
| AS | AngelScript (Hazelight fork dialect) |
| Gym / AutoTest / Gauntlet | CkTests' interactive / headless-PIE / process-level test layers |

## Code style

Runtime modules are machine-uniform on these rules (counts from the 2026-07-02 sweep; editor
modules are visibly laxer — that is observed history, not license for new code).

**Function shapes.** Trailing return types everywhere EXCEPT UFUNCTION declarations (UHT rejects
them there — the concrete return type goes on its own line). Definitions split across lines:

```cpp
// UFUNCTION declaration (.h) — concrete type, own line, no trailing return:
UFUNCTION(BlueprintCallable,
          Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
          DisplayName="[Ck][Timer] Add New Timer")
static FCk_Handle_Timer
Add(
    UPARAM(ref) FCk_Handle& InHandle,
    const FCk_Fragment_Timer_ParamsData& InParams);

// Any definition (.cpp) and any non-UFUNCTION declaration — trailing return:
auto
    FProcessor_Timer_Setup::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InTimerEntity,
        const FFragment_Timer_Params& InParams,
        FFragment_Timer_Current& InCurrentComp)
    -> void
{
    ...
}
```
(Exemplars: `CkTimer_Utils.h:50-56`, `CkTimer_Processor.cpp:19-27`.) Allman braces, 4-space
indent, CRLF, `#pragma once`; includes ordered own-header → Ck module paths → engine → `*.generated.h` last.
`// ----…----` separator lines between top-level declarations.

**Validation & flow.** Early-out with the ensure macro (it IS an inverted if), single-statement
guards on one line:

```cpp
CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Timer Handle [{}]"), InHandle)
{ return {}; }

if (InParams.Get_CountDirection() == ECk_Timer_CountDirection::CountUp)
{ TimerChrono.Reset(); }
```

`ck::IsValid` / `ck::Is_NOT_Valid` for all validity; `NOT` macro instead of `!`;
`ck::IsValid_Policy_NullptrOnly{}` is for RAW pointers only (smart pointers have their own
overloads — pass them bare).

**`auto` aggressively**, including typed-nullptr casts: `auto Canvas = static_cast<UCanvas*>(nullptr);`.
`{}` construction everywhere EXCEPT UFUNCTION parameter defaults, which use `()` (UHT limitation);
no `= {}` default-init inside UFUNCTION signatures.

**Naming.**
- Members `_PascalCase` (no `m_`); params `In*` / `Out*`; locals PascalCase; no `b` prefix on bools
  (engine-forced trait fields like Iris' `bHasSerialize` are the only exemption).
- `Get_` getters (may-fail variants `TryGet_*` return invalid handles), `Request_` mutators,
  `Do*` private helpers, `INTERNAL__` BP-internal plumbing.
- No UFUNCTION overloads (UHT physics) — disambiguate with the house suffixes: `_ByName`, `_ByTag`,
  `_Simple`, `AddOrReplace`. A plain C++ overload MAY share a UFUNCTION's name.
- Enums over bool options (`ECk_EnableDisable`, `ECk_SucceededFailed` + `ExpandEnumAsExecs` for BP
  exec pins). Optionality = enum-mode + value pair, not `TOptional`, in reflected surfaces
  (newest modules diverge — see ADJUDICATIONS A1 before following them).
- No anonymous namespaces and no file-local `static` helpers — unity builds concatenate TUs and
  collide them. Use a filename-derived named namespace (`namespace ck_timer_processor`).
- `MoveTemp`, never `std::move`; no `std::vector` intermediaries in TArray code.

**ECS naming is two-tier** (the single most misunderstood convention):

| Thing | Name | Lives in |
|---|---|---|
| Reflected config struct | `FCk_Fragment_[Feature]_ParamsData` (USTRUCT) | `X_Fragment_Data.h` |
| Runtime fragment | `ck::FFragment_[Feature]_[Type]` (plain C++, `ck` namespace) | `X_Fragment.h` |
| Bridge alias | `using FFragment_X_Params = FCk_Fragment_X_ParamsData;` | `X_Fragment.h` |
| Tag | `ck::FTag_[Feature]_[Purpose]` via `CK_DEFINE_ECS_TAG` | `X_Fragment.h` |
| Typesafe handle | `FCk_Handle_[Feature]` | `X_Fragment_Data.h` — NEVER `X_Fragment.h` (89/90 declarations comply; the 1 exception, `CkShape_Handle.h`, is infrastructure — the 91st grep hit is the macro definition itself) |
| Request | `FCk_Request_[Feature]_[Action] : FCk_Request_Base` | `X_Fragment_Data.h` |
| Processor | `ck::FProcessor_[Feature]_[Phase]` | `X_Processor.h/.cpp` |
| Utils | `UCk_Utils_[Feature]_UE` | `X_Utils.h/.cpp` |

Processor phase vocabulary (observed census): `Setup`, `HandleRequests`, `EndPlay`, `Replicate`,
`Update`, `ReplicateOnRestore`, `FireSignals`, `SyncReplication`, `RecomputeAll`, `MinMaxClamp`,
`ComputeAll`, `Exit`, `Destructor`. (`Teardown` is NOT house vocabulary — entity cleanup is
`EndPlay`/`Destructor`.) Processors self-register in their .cpp:
`CK_REGISTER_PROCESSOR(ck::FProcessor_Timer_Setup);` — the old ProcessorInjector mechanism is
retired; any doc mentioning it is stale.

**Encapsulation.** Private `_Members` + `CK_PROPERTY`/`CK_PROPERTY_GET` generated accessors.
Reads always via `Get_*`; direct `_Member` writes ONLY inside classes the fragment declares as
friends (its processors and Utils). Canonical fragment-data shape:

```cpp
USTRUCT(BlueprintType)
struct CKTIMER_API FCk_Fragment_Timer_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Timer_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _Duration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Timer_CountDirection _CountDirection = ECk_Timer_CountDirection::CountUp;

public:
    CK_PROPERTY_GET(_Duration);        // essential → getter only
    CK_PROPERTY(_CountDirection);      // optional → getter + fluent setter

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Timer_ParamsData, _Duration);  // essentials only
};
```

**Requests.** One struct per request type; essentials in the `CK_DEFINE_CONSTRUCTORS` ctor,
optionals via the fluent `Set_*` setters; `CK_REQUEST_DEFINE_DEBUG_NAME` on every request.
UFUNCTION surface takes `UPARAM(ref) FCk_Handle_X&` + the request struct and returns the handle
(chainable). Trivial single-datum mutations may take the value directly (`Request_ChangeCountDirection`).

**UObject refs in fragments — ownership split:** `TStrongObjectPtr` when the entity owns the
object's lifetime (spawned components, render targets); `TWeakObjectPtr` for non-owning
observation. Both are correct; pick by ownership. `TObjectPtr` only in UPROPERTY/reflected
contexts. UE GC does NOT trace fragment members — an object only a fragment points at WILL be
collected unless something roots it (see `ckecs-domain-reference` skill for the incident history).
UObjects never use `CK_DEFINE_CONSTRUCTORS` (UHT owns their construction).

**Signals.** Defined via `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(...)`; bound via
`CK_SIGNAL_BIND(ck::UUtils_Signal_OnX, Handle, Delegate, BindingPolicy, PostFireBehavior)` /
`CK_SIGNAL_UNBIND(...)` or the generated `BindTo_OnX` / `UnbindFrom_OnX` UFUNCTIONs.
Binding policies (verbatim — a past doc typo'd this and taught non-compiling code):
`ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame` (replay same-frame payload),
`FireIfPayloadInFlight` (replay last payload from any frame), `IgnorePayloadInFlight` (future
fires only). PostFire: `DoNothing` | `Unbind`. Semantics and lifecycle: `ckecs-architecture-contract` skill.

**Logging.** Per-module `namespace ck::<feature>` free functions generated by
`CK_DEFINE_LOG_FUNCTIONS` — `Fatal/Error/Warning/Display/Log/Verbose/VeryVerbose`, fmt-style `{}`
(never `%s`), values bracketed:

```cpp
timer::VeryVerbose(TEXT("Handling Reset Request for Timer with Entity [{}]"), InTimerEntity);
```

**Comments.** No *what*-comments — names, named lambdas, and `constexpr auto ResetOnActivate = true;`
extractions carry meaning (every bool argument at a call site gets a named constexpr). *Why*-comments
and `/** contract */` blocks on public Utils API and data shapes are house style, not a violation
(e.g. the why-no-Remove rationale at `CkTimer_Utils.h:74-77`).

## Macro quick reference

Full expansions, constraints, and add-a-new-X checklists: `ck-macros-and-codegen` skill.

| Macro | Purpose | One constraint worth knowing |
|---|---|---|
| `CK_GENERATED_BODY(T)` | ThisType alias + formatter/AS plumbing | Must precede macros that need `ThisType` |
| `CK_PROPERTY(_X)` / `CK_PROPERTY_GET(_X)` | Accessor generation (`Get`+`_X` by token-paste) | Member MUST start with `_` or names break |
| `CK_DEFINE_CONSTRUCTORS(T, ...)` | Default + essential-param ctors | Structs only — never UObjects; max 9 params; 1-arg ctor is `explicit` |
| `CK_ENSURE_IF_NOT(expr, fmt, ...)` | Ensure + inverted-if early-out | Active in Shipping by default; recovery block must be a correct silent path |
| `CK_DEFINE_ECS_TAG(_COUNTED)` | Tag fragment | Counted variant tracks add/remove depth |
| `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE` | Signal + BP delegate + Bind/Unbind utils | PostFire `Unbind` is a distinct generated fragment type |
| `CK_SIGNAL_BIND / CK_SIGNAL_UNBIND` | (Un)subscribe | Unbind + replay never connects — order matters |
| `CK_REQUEST_DEFINE_DEBUG_NAME(T)` | Request debug identity | On every request struct |
| `CK_GENERATED_BODY_HANDLE_TYPESAFE(T)` | Typesafe handle body | Declare in `_Fragment_Data.h`; pair with `CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE` |
| `CK_REGISTER_PROCESSOR(T)` | Processor self-registration | Top of the processor .cpp |
| `CK_REGISTER_SNAPSHOTABLE(T)` | **REMOVED** (Model-A purge, 2026-07-13) | The Model-A snapshot registry + fidelity oracle were deleted whole. Save/load is now v3 rebuild+hydrate only: a feature persists by registering a Produce/Apply handler on `FCk_ReplicatedFragmentHandlerRegistry` (`CkEcs/Net/ReplicatedFragmentContainer`, save subset via `Get_SaveHandlerTypes`); entities rebuild from spawn recipes on load. There is no per-fragment registration macro. Holder/record `_ROUNDTRIP`/`_TRANSIENT` policy classes are retained inert pending the Option-A macro retirement. |
| `CK_DEFINE_CUSTOM_FORMATTER_ENUM(E)` | fmt support for enums | Expected on every UENUM |

## Where things live

- [Source/CLAUDE.md](Source/CLAUDE.md) — module topology: decision tree ("I need to X → module Y"),
  tier table, cross-module patterns, module-authoring rules.
- [Script/CLAUDE.md](Script/CLAUDE.md) — AngelScript language deltas, `utils_*` layer, dynamic
  handles, generated-script hygiene. Required before editing any `.as`.
- `Source/<Module>/Claude.md` — 89 per-module docs (purpose, key API, anti-patterns). Read the
  target module's before coding in it. Some are stale (see `.claude/reports/DECISIONS.md` §15);
  trust code over doc on conflict and note the drift.
- `Source/EDITOR_MODULES.md` — editor-module reference. Runtime code must never depend on Tier-5.
- `CkEcs.natvis` (plugin root) — debugger visualizers for handles/registry.
- `.claude/reports/DECISIONS.md` + `ADJUDICATIONS.md` — doctrine change-log and open forks.
- `.claude/scripts/` — `sync-skills` junction script for consuming superprojects.

## Skill index — "for X, load Y"

Skills live in `.claude/skills/` here, in CkTests, and in CkGameplayDebugger (sync into a
superproject with `.claude/scripts/sync-skills.ps1`).

| Task | Skill (home) |
|---|---|
| Use/compose an EXISTING feature (attributes, timers, …) | no skill needed — [Source/CLAUDE.md](Source/CLAUDE.md) decision tree (+ Script/CLAUDE.md §5 for AS) |
| Classify/gate a change; what "done" requires | `ck-change-control` (CkFoundation) |
| Any build/UHT/linker/AS-compile failure, packaged-only crash | `ck-debugging-playbook` (CkFoundation) |
| "Has this been tried before?" — incidents, reverts, dead ends | `ck-failure-archaeology` (CkFoundation) |
| Why the architecture is shaped this way; invariants | `ckecs-architecture-contract` (CkFoundation) |
| ECS/EnTT theory, entity↔actor lifetime, GC interaction | `ckecs-domain-reference` (CkFoundation) |
| Any `CK_` macro; add fragment/processor/handle/request | `ck-macros-and-codegen` (CkFoundation) |
| Set up engine/plugins/build from scratch; env traps | `ck-build-and-env` (CkFoundation) |
| Write or run tests (AutoTest/net/C++/Gauntlet/gym) | `ck-tests-authoring-and-running` (CkTests) |
| Expose/verify anything in AngelScript; AS breaks silently | `ck-angelscript-interop` (CkFoundation) |
| Add a debugger view/overlay/inspector | `ck-gameplaydebugger-extension` (CkGameplayDebugger) |
| Write or modify any Slate UI (widgets, styles, list rows, editor-viewport interaction) | `ck-slate-tools` (CkGameplayDebugger) |
| The teardown/unbind lifecycle campaign (live defect: teardown mid-interaction, signals never fire after destroy, unbind leaks) | `ck-lifecycle-teardown-campaign` (CkFoundation) |
| Profile/benchmark processors; perf claims | `ck-performance-and-analysis` (CkFoundation) |
| "What should we build next" — vetted frontier | `ck-feature-frontier` (CkFoundation) |
| Long/multi-session task discipline; PROMPT/PHASE/PROGRESS docs | `ck-methodology` (CkFoundation) |
| Publish dev work — commit/fetch/rebase/regate/push across superproject + submodules | `ck-ship-dev` (CkFoundation) |

## Collaboration protocol

- **Research → Plan → Implement.** Never jump to code. Plans list files touched, approach, risks.
- **Long tasks use the phase-gate system** (PROMPT.md / Gate_N.md / living PROGRESS.md) — templates
  and triggers in `ck-methodology` (the owner of the doc-set naming). Reality checkpoints: after each feature, before each new
  component, before declaring done.
- **Stuck protocol:** STOP → delegate investigation → step back → simplify → ask "[A] vs [B]?".
  Two failed attempts means stop and present options, not a third attempt.
- **Editor-dependent verification** is a human step: label it `[EDITOR-VERIFY]` with exact clicks.
- Edit source files in place with your file tools; never paste code into chat as the deliverable.

## Provenance and maintenance

Re-verify volatile facts before trusting them long after 2026-07-02:
- Engine: `Get-Content D:/Repos/UnrealEngineAngelscript/Engine/Build/Build.version`
- EnTT: `ls Source/CkThirdParty/Public/CkThirdParty/ | grep entt`
- Module count: `(Get-Content CkFoundation.uplugin | Select-String '"Type"').Count` → 99 (one
  `"Type"` per module entry; counting `"Name"` overcounts — the 10 plugin-dependency entries carry it too)
- Binding policies: `rg -n 'enum class ECk_Signal_BindingPolicy' -A 10 Source/CkEcs`
- Processor registration: `rg -c 'CK_REGISTER_PROCESSOR' Source` (0 hits for `ProcessorInjector` expected)
- Tooling caveat: the Grep/Glob tools can silently miss files under this plugin (superproject
  `.ignore` covers `Script/`, `docs/`, `Content/`; Glob has returned false-empties even in
  `Source/`). Zero matches ⇒ re-check with `rg --no-ignore --files` or `Get-ChildItem` before
  concluding absence.
