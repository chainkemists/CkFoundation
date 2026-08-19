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
- **Scale:** 123 modules in `CkFoundation.uplugin` (89 Runtime, 26 UncookedOnly, 7 Editor, 1 DeveloperTool) + 3
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
   diagnostic and put the *correct* recovery in its body. No fallbacks that hide problems, ever,
   unless explicitly requested.
   The ensure condition itself must be safe for malformed, default, stale, and partially
   initialized input: validate outer prerequisites first, then deeper invariants in separate
   guards. The failure body may record an explicit failure/result state, but
   must immediately terminate the operation. Never touch the rejected value, continue through a mutable sentinel,
   or leave accepted partial state runnable. Registration, admission, composition, and
   configuration are atomic: one rejected required declaration invalidates the whole sequence.
   Compute a side-effect-safe validity value once and reuse it for the ensure and the explicit
   failure branch. `CK_DISABLE_ENSURE_CHECKS` can compile the entire ensure condition and body out;
   therefore load-bearing validation and recovery control flow must never live only in the macro.
   Every new validation boundary requires a focused invalid-input test proving rejection, zero
   downstream callbacks or mutation, no partial state, and no crash; also inspect a fresh startup
   log for ensures and script errors instead of relying only on process completion.
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
9. **Reuse existing modular features before building a bespoke one.** If a piece of data or behavior
   would be useful outside the one feature currently asking for it (a range/fade calculation, a
   display/config blob, anything that isn't intrinsic to that feature's own identity), extract it as
   its own small composable module instead of adding it as a field on the feature that happens to
   need it first. Before adding a field, ask "does an existing module already own this, or should
   this become its own module?" — check the [Source/CLAUDE.md](Source/CLAUDE.md) decision tree first.
   Case study: `CkPoi` v2 refactor (`Source/CkPoi/REFACTOR_MultiProjectorPoi.md`) — a Poi fragment
   that grew Category/Priority/VisibleRange/DisplayAsset fields directly couldn't express "one Poi,
   multiple projector-specific presentations"; those fields belonged in `CkLabel`, `CkEntityTag`, a
   new `CkVisibleRange`, and a new `CkPoiDisplayDefinition` all along.
10. **The Resilience Tenets bind all gameplay-system work.** [docs/RESILIENCE_TENETS.md](docs/RESILIENCE_TENETS.md)
   is the doctrine for building and fixing anything entities wait on or hold (claims, queues, locks,
   lifecycles): no silent early-returns, no band-aids without declared consent, cleanup scope-bound
   through the StateMachine (RAII — cross-cutting modes are transitions, never side-channel flags),
   waited-on resources must converge from arbitrary state (lease + reconciliation), no split-brain
   mirrors without a reconciler, fail-closed only with a bounded escape, systemic fixes over one-off
   patches. Read it before touching any such system; violations require the maintainer's explicit,
   informed consent.

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
          Category = "Ck|Utils|Timer",
          DisplayName="[Ck][Timer] Add Multiple New Timers")
static TArray<FCk_Handle_Timer>
AddMultiple(
    UPARAM(ref) FCk_Handle& InHandle,
    const FCk_Fragment_MultipleTimer_ParamsData& InParams);

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
(Exemplars: `CkTimer_Utils.h:66-72`, `CkTimer_Processor.cpp:19-27`.) Allman braces, 4-space
indent, CRLF, `#pragma once`; includes ordered own-header → Ck module paths → engine → `*.generated.h` last.
`// ----…----` separator lines between top-level declarations.

**Validation & flow.** Compute a side-effect-safe condition once, then diagnose AND recover in the
ensure macro's own body — it is an inverted `if`, so that body IS the failure path:

```cpp
const auto HandleIsValid = ck::IsValid(InHandle);
CK_ENSURE_IF_NOT(HandleIsValid, TEXT("Invalid Timer Handle [{}]"), InHandle)
{ return {}; }

if (InParams.Get_CountDirection() == ECk_Timer_CountDirection::CountUp)
{ TimerChrono.Reset(); }
```

Do **not** write the older `{}` + separate `if (NOT Cond)` shape. It exists in git history because
`CK_ENSURE_IF_NOT` expands to `if constexpr(false)` under `CK_DISABLE_ENSURE_CHECKS`, which would
delete a body containing the recovery. **That configuration is unreachable**: `CkBuildConfig.Build.cs`
pins `BuildConfigurationOverride` to `MatchWithUnreal` as a compile-time `const` (the `Profile` case
is dead code — the file even flags it unreachable), and every reachable branch sets
`CK_DISABLE_ENSURE_CHECKS=0`.

⚠️ **Before ever enabling `Profile`, change the macro first.** ~2,300 call sites now put recovery
inside the ensure body; flipping that `const` without first making `CK_ENSURE_IF_NOT` preserve
control flow (expand to `if (NOT (InExpression))`, exactly as the `CK_DISABLE_ENSURE_DEBUGGING`
branch already does) would silently delete every one of those early-outs at once.

`ck::IsValid` / `ck::Is_NOT_Valid` for all validity; `NOT` macro instead of `!`;
`ck::IsValid_Policy_NullptrOnly{}` is for RAW pointers only (smart pointers have their own
overloads — pass them bare).

**Negated validity is `ck::Is_NOT_Valid(X)`, never `NOT ck::IsValid(X)`.** The two compile the same;
only the first is greppable as a single token and reads as one predicate rather than a negation of
another. Same rule for the typed variants.

> **One exception — the two-argument `(entity, context)` overload.** `ck::IsValid(Entity, Context)`
> validates an entity *against a context handle*; `Is_NOT_Valid`'s second parameter is the
> validation **policy**, so a context handle silently binds as `T_Policy` and fails to compile
> (or worse, resolves to the wrong check). Write `NOT ck::IsValid(Entity, Context)` there — see
> `CkEntityLifetime_Utils.cpp:156`. A second argument that IS a policy (`IsValid_Policy_*{}`,
> `T_ValidationPolicy{}`) is fine on either spelling.

**Never put an `if` and its body on the same line.** The brace block goes on the next line at the
same indent, even when the body is a single statement:

```cpp
// ✅
if (Resolved.IsValid())
{ Def.Preconditions.Add(Resolved); }

// ❌
if (Resolved.IsValid()) { Def.Preconditions.Add(Resolved); }
```

This keeps every branch body on a line of its own, so a diff shows which branch changed and a
breakpoint can be set on the body independently of the condition.

**Typesafe handle conversion: `UCk_Utils_X_UE::CastChecked` / `::Cast` — NEVER
`ck::StaticCast<FCk_Handle_X>`.** `ck::StaticCast` is the unchecked primitive those two are built on;
calling it directly skips the feature's `Has` check, so a wrong handle converts silently and blows up
later with no breadcrumb. `CastChecked` when the feature is guaranteed (view-filtered, just added,
already `Has`-checked), `Cast` when absence is legitimate. Full rule + table:
[Source/CLAUDE.md](Source/CLAUDE.md) § "Converting a handle to a typesafe handle".

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

**`UPROPERTY()` goes on its OWN line — never inline with the member, and never column-aligned.**
One property per two lines, declaration order preserved:

```cpp
// ✅ Correct
UPROPERTY()
FName _SlotName;

UPROPERTY()
TArray<uint8> _ScreenshotPng;

// ❌ Wrong — inline specifier, padded to a phantom column
UPROPERTY() FName               _SlotName;
UPROPERTY() TArray<uint8>       _ScreenshotPng;
```

This holds for a bare `UPROPERTY()` too, not just the multi-specifier ones that obviously need
wrapping. The alignment padding is the worse half: any new member whose type is wider than the
current column re-pads the whole block, so a one-line change lands as a twenty-line diff and the
real edit vanishes into the noise.

Some older headers still carry the inline form (`CkSnapshot/SaveGame/CkSnapshot_Header.h`,
`CkSnapshot/Snapshot/CkSnapshot_LoadReport.h`). They are the exception, not the pattern — do not
copy from them.

**Requests.** One struct per request type; essentials in the `CK_DEFINE_CONSTRUCTORS` ctor,
optionals via the fluent `Set_*` setters; `CK_REQUEST_DEFINE_DEBUG_NAME` on every request.
UFUNCTION surface takes `UPARAM(ref) FCk_Handle_X&` + the request struct and returns the handle
(chainable). Trivial single-datum mutations may take the value directly (`Request_ChangeCountDirection`).

**Request completion.** Every deferred `Request_*` ends with a trailing
`const FCk_Delegate_Request_OnCompleted& InDelegate` (`CkEcs/Request/CkRequest_Completion.h`)
carrying `meta = (AutoCreateRefTerm = "InDelegate")` and **no C++ default** — UHT cannot parse a
delegate default value (`"C++ Default parameter not parsed"`). Optionality per environment:
Blueprint gets it from `AutoCreateRefTerm`; AngelScript gets an emitted `= FCk_Delegate_Request_OnCompleted()`
default in the generated `utils_*.as` wrapper (CkAngelscriptGenerator emits defaults for
AutoCreateRefTerm delegate params); C++ callers that don't want completion pass `{}`.
A bound delegate is GUARANTEED to fire exactly once with
one of `ECk_Request_OperationResult::{Succeeded, Failed, Failed_NotEnqueued, Failed_Cancelled}`.
The transport is the request struct itself — `FCk_Request_Base` carries a non-reflected
`_CompletionDelegate`, so the delegate rides the queued request and there is no request entity and
no signal in the default case:
- **Utils boundary** — `if (InDelegate.IsBound()) { Request.Set_CompletionDelegate(InDelegate); }`
  on a named request local, then enqueue that same local. A caller that passes nothing pays one
  `IsBound()` check. Synchronous rejection before enqueue fires `Failed_NotEnqueued` directly via
  `InDelegate.ExecuteIfBound`.
- **Handler** — `ck::MakeCompletionGuard(InRequest, InOwner, Result)` declared AFTER the `Result`
  local it holds by reference; the drain sets `Succeeded` on the success path. The guard's
  destructor calls `TryFireCompletion`, which unbinds after executing — that unbind IS the
  exactly-once guarantee, so every completion path may fire unconditionally.
- **Immediate mutators** — a `Request_*` that mutates inline and enqueues nothing has no request
  struct and no handler; it fires `InDelegate.ExecuteIfBound(Owner, Succeeded)` synchronously after
  the mutation, so the caller contract never depends on a feature's internal deferral shape.
- **Teardown** — a converted feature's request-drain view excludes owners already tagged
  `ck::FTag_DestroyEntity_Initiate` (added synchronously by `Request_DestroyEntity`) on top of
  `CK_IGNORE_PENDING_KILL`, so destroy-then-drain is never a race: the queue survives to the
  feature's `FGroup_EndPlay` processor, which calls `ck::request::FireCancelledForPending` over its
  `_Requests` fragment and completes every entry with `Failed_Cancelled`.
Completion is LOCAL-machine: it reports the outcome of local processing, never a remote peer's.
Canonical reference: `CkTimer`. Every feature module carries the contract.

**Result semantics.** `Succeeded` means the request was processed AND the caller's intent now holds
— so an idempotent no-op (pause an already-paused entity, disable an already-disabled one) reports
`Succeeded`, not `Failed`. Reserve `Failed` for an intent that does not hold afterwards and that
retrying will not fix (a terminal-state target, a missing asset, a rejected transition).

**Never strand a caller.** A `Request_*` that early-returns must still complete: hoist the condition
to a local and fire `Failed_NotEnqueued` from inside the `CK_ENSURE_IF_NOT` body before returning.
(Earlier revisions of this file forbade that on the grounds the body compiles out under
`CK_DISABLE_ENSURE_CHECKS` — see *Validation & flow* above: that configuration is unreachable, and
the split shape it prescribed has been removed from the codebase.)

**Not every `Request_*` is in scope.** The contract applies to functions that enqueue an
`FCk_Request_Base`-derived struct onto a `_Requests` fragment, plus immediate mutators on an entity
handle. It does NOT apply to subsystem/BPFL helpers that merely share the prefix but take a
`UObject*`/`UPrimitiveComponent*` and return synchronously (`CkActorRelay`, parts of `CkPhysics`,
`CkCore`), nor to processor-internal enqueues with no external caller. Those have no owner handle to
report against — never invent one.

**Additive where a bespoke channel already carries RESULTS.** `CkInventory` and `CkResolver` keep
their bespoke result enums and per-operation signals (`PopulateRequestHandle` +
`CK_SIGNAL_BIND_REQUEST_FULFILLED` + `ck::MakeRequestResultGuard`) and gain the generic delegate
alongside; the two are derived from the same value and must never disagree. `FCk_Request_Eqs_RunQuery`
and `FCk_Request_DialogEmitter_Query` likewise KEEP their `_OnComplete` members: those carry query
results, and for the deferred path they are the only channel the caller can reach.

**The delegate is always the LAST parameter.** If a preceding parameter had a C++ default, that
default is dropped — moving the delegate earlier breaks AngelScript, which permits defaults only on
trailing parameters, and silently rebinds existing positional callers. Dropping a default is a
source-breaking change for AngelScript callers that omitted the argument, so grep `Script/` for them.

**UObject refs in fragments — a fragment ref is NEVER the GC root.** UE GC does not trace fragment
members, so an object whose only reference is a fragment is collected mid-life regardless of its
Outer or of the pointer type wrapping it. `TStrongObjectPtr` in a fragment does not fix that — it
merely hides it, keeping one object alive off-registry while nothing keeps its dependencies alive
(incident history: `ckecs-domain-reference` skill). The rule is therefore about **who roots**,
not about pointer strength:

| The fragment's ref is… | Hold it as | Rooted by |
|---|---|---|
| an object the feature **observes** (an actor, a component, a world-context object it did not create) | `TWeakObjectPtr`, resolved on read | whoever owns it; a destroyed referent must read as gone, not as a recycled address |
| an object the feature **creates/owns** (components, widgets, runtime data objects) | `TWeakObjectPtr` | `UCk_Utils_Object_UE::Request_CreateNewObject` — the ObjectPooling subsystem **pins** every instance it hands out; the pin is the root |
| an **asset the feature loads** (mesh, material, curve, sound, Niagara system, anim) | `TSoftObjectPtr` in params/requests + an `FCk_ResourceLoader_RootedAssetBatch` in `Current` | the batch's streamable handle; resetting the batch releases the assets |

`TObjectPtr` only in UPROPERTY/reflected contexts (there GC does trace, and the UPROPERTY is the
root). UObjects never use `CK_DEFINE_CONSTRUCTORS` (UHT owns their construction).

Both rooting mechanisms and the composition/EndPlay order they imply:
[Source/CLAUDE.md](Source/CLAUDE.md) § "Objects and assets a fragment holds". Never hand-roll a
third: no bare `NewObject` stored strongly in a fragment, no `AddToRoot`, no synchronous
`LoadObject`/`.LoadSynchronous()` on a params path, and no hard `TObjectPtr` on an
`EditAnywhere`/`BlueprintReadWrite` asset field — a hard ref there force-loads the asset with every
DataAsset or Blueprint that merely names it, which is the authoring cost the soft-ref sweep exists
to remove.

⚠️ **Residual `TStrongObjectPtr` fragment members exist and are NOT the pattern** — the sweep that
established the above (`e2f43a098`…`cfb24ae94`, 2026-07) converted CkAnimation, CkFx, CkAudio,
CkIskmRenderer, CkJolt, CkPmg, CkRenderTarget, CkTween params, CkVat, CkWorldSpaceWidget,
CkRaySense, CkCamera, CkActor, CkEntitySpawner. Untouched holders remain (`CkVfx`'s
`FFragment_VfxCue_Current::_NiagaraComponent`, `CkTween`'s resolved curve drives, CkVoiceChat,
CkCrowd, CkDebugScene). Do not copy them; convert one when you are already changing its feature,
and say so in the commit.

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
(e.g. the why-no-Remove rationale at `CkTimer_Utils.h:74-77`). **Never leave a process breadcrumb in
shipped code** — a comment naming a Gate, Phase, PROMPT, campaign, or "the refactor" is always noise;
that history belongs in the campaign docs, not the source. Default to *no* comment: add one only when
it carries a *why* a good name cannot. Every implementation closes with the comment audit below.

## Macro quick reference

Full expansions, constraints, and add-a-new-X checklists: `ck-macros-and-codegen` skill.

| Macro | Purpose | One constraint worth knowing |
|---|---|---|
| `CK_GENERATED_BODY(T)` | ThisType alias + formatter/AS plumbing | Must precede macros that need `ThisType` |
| `CK_PROPERTY(_X)` / `CK_PROPERTY_GET(_X)` | Accessor generation (`Get`+`_X` by token-paste) | Member MUST start with `_` or names break |
| `CK_DEFINE_CONSTRUCTORS(T, ...)` | Default + essential-param ctors | Structs only — never UObjects; max 9 params; 1-arg ctor is `explicit` |
| `CK_ENSURE_IF_NOT(expr, fmt, ...)` | Diagnostic inverted-if syntax | Body IS the failure path — put the recovery in it; hoist `expr` to a local so it evaluates once |
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
| Author or port a VFX (CkParticles behavior, CkUsf look, cadence row, A/B parity in the gym) | `ck-vfx-authoring` (CkFoundation) |
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
- **Batch the edits, not the gate.** Non-negotiable #7 requires the gate re-run before you claim
  *fixed* — it does NOT mean re-running after every edit. A CK editor build is 5-30 min and a full
  suite ~10-23 min, so a planned series of related changes is made in full, then verified in **one**
  run scoped to the modules it touched (`--test-pattern`); the unscoped full suite is the
  **end-of-work gate**, run once before claiming done. Break the batch when a change is novel or
  risky enough to want immediate feedback, when one change's correctness gates the next, or while
  debugging. A red batch does not localize its cause — bisect it, don't guess. Capture the baseline
  counts and failing-test *names* before the first change, and always report which pattern you ran.
  Mechanics and flags: the `/build-test` skill.
- **Comment audit before done (mandatory closing step).** Re-read your own diff and delete every
  comment a good name would carry: gate/phase/campaign/PROMPT breadcrumbs, restatements of the code,
  and any *what*-comment. Keep only load-bearing *why*-comments and `/** contract */` blocks (Code
  style → Comments). If the diff added a comment you cannot justify as a *why*, remove it.
- Edit source files in place with your file tools; never paste code into chat as the deliverable.

## Provenance and maintenance

Re-verify volatile facts before trusting them long after 2026-07-02:
- Engine: `Get-Content D:/Repos/UnrealEngineAngelscript/Engine/Build/Build.version`
- EnTT: `ls Source/CkThirdParty/Public/CkThirdParty/ | grep entt`
- Module count: `(Get-Content CkFoundation.uplugin | Select-String '"Type"').Count` → 122 (one
  `"Type"` per module entry; counting `"Name"` overcounts because plugin-dependency entries carry it too)
- Binding policies: `rg -n 'enum class ECk_Signal_BindingPolicy' -A 10 Source/CkEcs`
- Processor registration: `rg -c 'CK_REGISTER_PROCESSOR' Source` (0 hits for `ProcessorInjector` expected)
- Tooling caveat: the Grep/Glob tools can silently miss files under this plugin (superproject
  `.ignore` covers `Script/`, `docs/`, `Content/`; Glob has returned false-empties even in
  `Source/`). Zero matches ⇒ re-check with `rg --no-ignore --files` or `Get-ChildItem` before
  concluding absence.
