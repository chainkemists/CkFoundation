# Add-a-new-X checklists

All derived from the CkTimer quartet — the canonical smallest complete feature
(`CkTimer/Public/CkTimer/`: `CkTimer_Fragment_Data.h`, `CkTimer_Fragment.h/.cpp`,
`CkTimer_Processor.h/.cpp`, `CkTimer_Utils.h/.cpp`). Read it before authoring; mimic, don't
invent (root non-negotiable #1). Naming table: root CLAUDE.md "ECS naming is two-tier".

Build gate for every checklist below: compile per `ck-build-and-env`, then grep-verify each
registration line exists. Anything that needs PIE is `[EDITOR-VERIFY]` and marked as such — run
every `[EDITOR-VERIFY]` tag below via the exact Blueprint checklist in `ck-change-control`
§"Three environments" (its numbered steps cover the accessor-visibility, autocast-node,
request-node, and BindTo-node checks these checklists tag).

### 3.1 New fragment (Spec + runtime fragments)

1. `Ck<Feature>/Public/Ck<Feature>/Ck<Feature>_Fragment_Data.h` — the reflected authoring struct:
   - UENUMs first, each followed by `CK_DEFINE_CUSTOM_FORMATTER_ENUM(E);` (exemplar :17-27).
   - `FCk_<Feature>_Spec` USTRUCT: `GENERATED_BODY()` → `CK_GENERATED_BODY` →
     (optional `using IsSnapshotable = void;` for Tier-A round-trip, §3.6) → private
     `UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))` members →
     `CK_PROPERTY_GET` for essentials / `CK_PROPERTY` for optionals →
     `CK_DEFINE_CONSTRUCTORS(T, essentials)` last (exemplar :82-124).
   - `#include "Ck<Feature>_Fragment_Data.generated.h"` is the LAST include (UHT rule).
   - Verify: UHT compiles; the struct shows in BP with Get_/Set_ accessors `[EDITOR-VERIFY]`.
2. `Ck<Feature>_Fragment.h` — the runtime side, all inside `namespace ck`:
   - Lifecycle tags via `CK_DEFINE_ECS_TAG(FTag_<Feature>_NeedsSetup);` etc. (exemplar :27-29).
   - `FFragment_<Feature>` — the primary state fragment (bare noun; Timer/Transform precedent):
     plain struct, NOT a USTRUCT: `CK_GENERATED_BODY` → friend its processors + Utils → private
     state → `CK_PROPERTY_GET` → `CK_DEFINE_CONSTRUCTORS`. Friends are the ONLY writers of
     `_Members` (root doctrine). Additional mutable state gets purpose-named siblings
     (`_State`/`_Result`/`_Pending*`/`_Cooldowns`/`_Previous`), never a `_Current` monolith.
   - `FFragment_<Feature>_Params` — ONLY if some Spec field is read at steady state (root
     CLAUDE.md § Spec unpacking): a small residue struct holding exactly those fields
     (exemplar: `FFragment_Timer_Params{_Behavior}`). An all-hot feature may alias the whole
     Spec instead (`using FFragment_Tween_Params = FCk_Tween_Spec;` — Tween/StateMachine shape).
     Start-values NEVER live here — they seed the state fragments / tags at `Add()`.
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
       ck::TReadWrite<FFragment_<Feature>>,
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

