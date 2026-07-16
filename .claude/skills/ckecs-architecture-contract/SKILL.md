---
name: ckecs-architecture-contract
description: "Use when reasoning about why CkEcs is structured as it is or which invariants a change must preserve across handles, requests, signals, fragments, replication, and GC."
---

# CkEcs architecture contract — why it is shaped this way

This skill records the load-bearing architectural decisions of CkFoundation's ECS, the reasons
behind them (with the commits and file:lines that prove them), the invariants any change must
preserve, and the known-weak points stated plainly. Terminology (Entity/Fragment/Processor/
Handle/Request/Signal) is defined in the root doctrine's Lingo table — `Plugins/CkFoundation/CLAUDE.md`.
All file paths below are relative to `Plugins/CkFoundation/Source/`; all facts verified 2026-07-02
against submodule HEAD `7330c1bab` unless labeled INFERRED.

## When NOT to use this skill

| You want to... | Load instead |
|---|---|
| Mechanically add a fragment/processor/handle/request (skeletons, macro checklists) | `ck-macros-and-codegen` |
| EnTT theory, registry/storage internals, entity↔actor lifetime, GC interaction depth | `ckecs-domain-reference` |
| Debug a build/UHT/AS-compile failure | `ck-debugging-playbook` |
| Work the teardown/unbind defect cluster | `ck-lifecycle-teardown-campaign` |

---

## 1. The reflection boundary — two-tier fragments

Every feature splits its data across a reflected tier and a runtime tier (naming table: root
CLAUDE.md "ECS naming is two-tier"):

| Tier | Type | Reflected? | Holds | Exemplar |
|---|---|---|---|---|
| Config | `FCk_Fragment_X_ParamsData` (USTRUCT) | Yes — BP/AS/editor-authorable | Designer-set values, provider refs | `CkTimer/Public/CkTimer/CkTimer_Fragment_Data.h:82-124` |
| Runtime | `ck::FFragment_X_Current` / `_Requests` (plain C++ struct in `ck` namespace) | No | Mutable live state, native-only types | `CkTimer/Public/CkTimer/CkTimer_Fragment.h:37-63, 67-85` |
| Bridge | `using FFragment_X_Params = FCk_Fragment_X_ParamsData;` | — | — | `CkTimer_Fragment.h:33` |

The alias is not cosmetic: the reflected struct **is stored directly as the params fragment** —
`InNewEntity.Add<ck::FFragment_Timer_Params>(InParams);` (`CkTimer/Public/CkTimer/CkTimer_Utils.cpp:58`).
One struct serves as editor-facing config AND as the immutable ECS fragment.

**Why the split exists** (confirmed structurally):

1. Runtime fragments hold types UHT cannot represent at all: `std::variant` request lists
   (`CkTimer_Fragment.h:77-78`), `entt::sigh`/`entt::sink` plus `TOptional<std::tuple<...>>`
   payloads in signal fragments (`CkEcs/Public/CkEcs/Signal/CkSignal_Fragment.h:54-79`),
   `FCk_Chrono` internals. Reflecting the runtime tier is not a style choice you could make —
   it is impossible for the real state these fragments carry.
2. Reflection means authorability. A UPROPERTY is editable in details panels and writable from
   BP/AS. Config wants that; live state must not be mutable behind the processors' backs — the
   whole mutation contract (§3, §4) assumes writes flow through requests and friends.
3. Snapshot participation differs by tier: a USTRUCT ParamsData opts into save/restore with one
   line (`using IsSnapshotable = void;`, reflection-serialized — `CkTimer_Fragment_Data.h:90-92`);
   a plain runtime fragment must hand-write `SerializeSnapshot` (`CkTimer_Fragment.h:59-62`).
4. INFERRED (stated intent, consistent with 1-3): keeping the hot iteration tier out of UHT also
   avoids reflection/codegen tax on types that change often during feature work.

**What breaks if you violate it:**

- *Runtime state in a USTRUCT fragment* → the editor/BP can now mutate live ECS state out-of-band,
  bypassing requests and friend-gating; and the moment that state needs a variant/sink/optional-tuple
  you are stuck — UHT rejects it.
- *Config in a runtime fragment* → designers cannot see or set it (no details panel, no BP/AS
  access, cannot ride EntityScript spawn params or data assets), and it loses free reflection-based
  snapshot serialization.

---

## 2. Typesafe handles — compile-time feature proof

`FCk_Handle` is the generic entity reference. `FCk_Handle_Timer` etc. are macro-stamped USTRUCT
subclasses that add **zero data** and one guarantee: *you cannot get one without passing a
fragment-presence gate*.

**The machinery** (`CkEcs/Public/CkEcs/Handle/CkHandle_TypeSafe.h`):

- `CK_GENERATED_BODY_HANDLE_TYPESAFE(T)` (:84-102) generates a **private constructor from
  `const FCk_Handle&`** (:101-102) and friends exactly one converter — `ck::StaticCast` (:85-89).
  Implicit `FCk_Handle → FCk_Handle_Timer` does not compile; only `ck::StaticCast<T>` (which the
  feature's `Cast`/`CastChecked` wrap) can mint a typed handle. Derived→base stays free (public
  inheritance), so typed handles flow into generic APIs.
- **Zero-size invariant, asserted twice**: `static_assert(sizeof == sizeof(FCk_Handle))` at the
  base (:76-80, comment "DO NOT REMOVE") and per-type inside
  `CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE` (:144-145). It is what makes the
  in-place reinterpret forms legal (`ck::details::StaticCast`, :250-273) and value casts cheap
  (:296-307 constructs `T{InHandle}` through the private ctor).
- Gate semantics differ deliberately: `CastChecked` fires `CK_ENSURE_IF_NOT` naming the handle
  type it failed to mint and returns `{}` (:168-172); `Cast` returns `{}` silently (:177-189). Both check
  `Has(...)` — the fragment list supplied to `CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE` in the
  Utils .cpp (:195-242; exemplar `CkTimer/Public/CkTimer/CkTimer_Utils.cpp:136`).

**What it buys:** a `FCk_Handle_Timer` parameter is compile-time proof the entity passed a Timer
`Has` gate — downstream code skips re-validation, and API signatures document their fragment
requirements. **What it costs:** per-handle boilerplate (declaration + ISVALID_AND_FORMATTER +
`CK_DEFINE_CPP_CASTCHECKED_TYPESAFE` in the Utils header + `CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE`
in the .cpp) and the hard rule that handles never carry data.

**Where they are declared and why:** in `X_Fragment_Data.h`, next to the feature's ParamsData and
requests — never `X_Fragment.h`. Reasons: the declaration is a USTRUCT (UHT must see it; BP and AS
consume it) and `_Fragment.h` would drag fragment implementation internals into every consumer plus
invite UHT circular-include problems (`CkEcs/Claude.md` anti-pattern 6). Measured 2026-07-02:
90 typed-handle declarations; 89 in `*_Fragment_Data.h`, 1 sanctioned exception —
`CkShapes/Public/CkShapes/CkShape_Handle.h`, a dedicated handle header (still not a `_Fragment.h`).

**AngelScript caveat — parent up-conversion is UNCHECKED.** The AS bindings register `opImplConv`
methods whose bodies are pass-throughs (`return InOther;` —
`CkEcs/Public/CkEcs/Handle/CkHandle_TypeSafe_AngelScript.h:48-57`): a derived handle flows into any
parent-typed parameter with **no fragment-presence check at the boundary**; failure surfaces later,
inside whatever util touches state. Use an explicit cast when provenance is uncertain
(`Script/CLAUDE.md` §6, lines 160-172).

**Handle-inheriting-handle** (`CK_GENERATED_BODY_HANDLE_DERIVED(T, Parent)`, :109-129) exists so a
derived handle's constructors initialize the direct parent and AS mixin methods propagate down the
chain (`using MixinParentHandle`, :127). It is rare/advanced — 7 declarations total as of
2026-07-02 (FCk_Handle_Inventory_Spatial/_DataOnly, the four `FCk_Handle_Shape*` types,
FCk_Handle_SmState_UnderConstruction). Do not reach for it by default (DECISIONS.md #28).

---

## 3. Requests — the deferred-mutation contract

Root non-negotiable #5: **utility functions enqueue; processors mutate.** A `Request_*` UFUNCTION
appends a request struct to the feature's `ck::FFragment_X_Requests` fragment; the feature's
`HandleRequests` processor consumes it at its scheduled slot.

**Why deferral, concretely:**

1. *The registry is locked during iteration.* Structural changes mid-`ForEachEntity` are unsafe —
   the same reason entity creation inside processors goes through the deferred API
   (`CkEcs/Claude.md` anti-pattern 5).
2. *One mutation point per feature per frame = deterministic ordering.* Callers run at arbitrary
   times (BP events, signal handlers, AS ticks); the state change lands only at the HandleRequests
   processor's slot in the scheduler, so every downstream processor in the frame sees one
   consistent state regardless of who requested what, when.
3. *Net gating for free.* Processors declare `NetModeRequirement` (ClientOnly/ServerOnly), so the
   same `Request_*` API is safe to call anywhere; where the mutation actually runs is a processor
   property, not a call-site concern.
4. *Auditability.* Requests are named one-shot objects — debug names
   (`CK_REQUEST_DEFINE_DEBUG_NAME`), optional per-request callstack fragments
   (`CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR`, `CkTimer_Fragment.h:105`), and completion signals all
   hang off the request itself.

**Struct anatomy** (canonical: `FCk_Request_Timer_Jump`, `CkTimer_Fragment_Data.h:150-169`): one
struct per request type; derives `FCk_Request_Base`; `CK_REQUEST_DEFINE_DEBUG_NAME` on every one;
essential params in `CK_DEFINE_CONSTRUCTORS` (you cannot construct an incomplete request),
optionals via the fluent `Set_*` setters (rationale + shapes: root CLAUDE.md "Requests"). Requests
are **one-shot**: `PopulateRequestHandle` may be called at most once per struct — construct a fresh
request per submission (`CkEcs/Public/CkEcs/Request/CkRequest_Data.h:16-18, 60-62`). Two bases
exist: `ck::FRequest_Base` (:19, C++-only) and `FCk_Request_Base` (:63, USTRUCT/BP-facing, which
composes a `ck::FRequest_Base` member at :106) — use the USTRUCT base for anything BP/AS may build.

**The consumed-exactly-once idiom** (`CkTimer/Public/CkTimer/CkTimer_Processor.cpp:48-66`):

```cpp
const auto RequestsCopy = InRequestsComp._Requests;
InRequestsComp._Requests.Reset();

algo::ForEachRequest(RequestsCopy, ck::Visitor(
[&](const auto& InRequest) -> void
{
    DoHandleRequest(InDeltaT, InTimerEntity, InCurrentComp, InParamsComp, InRequest);

    if (InRequest.Get_IsRequestHandleValid())
    {
        InRequest.GetAndDestroyRequestHandle();
    }
}), policy::DontResetContainer{});

if (InRequestsComp._Requests.IsEmpty())
{
    InTimerEntity.Remove<MarkedDirtyBy>();
}
```

Copy-then-reset makes handling re-entrancy-safe: a request enqueued *while handling* lands in the
live array (not the copy), survives to the next pass, and the dirty tag is only removed when the
live list is empty. Never iterate `_Requests` in place.

**Completion delegates — the request-handle mechanism.** A caller who wants "tell me when my
deferred request finished" gets it via a signal bound to a request-scoped entity:

- Utils side: `CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_X, InRequest.PopulateRequestHandle(InOwner), InDelegate);`
  (`CkInventory/Public/CkInventory/Inventory/Spatial/CkInventory_Spatial_Utils.cpp:102`; a
  request-entity variant exists at `CkActor/Public/CkActor/ActorModifier/CkActorModifier_Utils.cpp:22-27`).
  19 call sites as of 2026-07-02. The macro (`CkEcs/Public/CkEcs/Signal/CkSignal_Macros.h:48-49`)
  = `IgnorePayloadInFlight` + `Unbind`: correct because the bind always precedes processing (the
  request is deferred, nothing can be in flight yet) and completion is one-shot.
- Processor side: broadcast on `GetAndDestroyRequestHandle()` when done — manually (Timer above) or
  via the scope-exit guard `ck::MakeRequestResultGuard<TSignal>(InRequest, [&]{ ... });`
  (`CkRequest_Data.h:136-168`; live at `CkInventory_Spatial_RequestTraits.cpp:45`). The guard's
  payload builder runs at destruction so it can capture the result enum by reference — declare the
  guard AFTER the locals it captures (invariant stated at `CkRequest_Data.h:131-132`).

The same API in all three environments (Timer jump):

```cpp
// C++
UCk_Utils_Timer_UE::Request_Jump(TimerHandle, FCk_Request_Timer_Jump{JumpTime});
```
Blueprint: node `[Ck][Timer] Request Jump` (returns the handle, chainable).
```angelscript
// AngelScript (utils are handle mixins)
TimerHandle.Request_Jump(FCk_Request_Timer_Jump(JumpTime));
```

---

## 4. Friend-gated encapsulation — a grep-able write surface

Runtime fragments declare their mutators as friends and expose reads through generated getters:

```cpp
// CkTimer_Fragment.h:37-54 (abridged)
struct CKTIMER_API FFragment_Timer_Current
{
    friend class FProcessor_Timer_Setup;
    friend class FProcessor_Timer_HandleRequests;
    friend class FProcessor_Timer_Update;
    friend class FProcessor_Timer_Update_Countdown;
    friend class FProcessor_Timer_Replicate;

private:
    FCk_Chrono _Chrono;

public:
    CK_PROPERTY_GET(_Chrono);
};
```

**Why:** the friend list at the top of the fragment IS the complete write-surface documentation.
"Who can mutate timer state?" is answered by reading five lines — or by grepping `_Chrono` knowing
every hit outside those friends is read-only `Get_*`. Measured 2026-07-02 across `*_Fragment.h`:
384 `friend class FProcessor_*` + 105 `friend class UCk_Utils_*`. Utils friendship exists for
composition-time writes (`Add` builds the fragments — `CkTimer_Utils.cpp:58-59`) and for enqueuing
into `_Requests` (`CkTimer_Fragment.h:73-74`); steady-state mutation still belongs to processors
(§3). Adding a friend to a fragment is an architectural statement — expect review scrutiny.

---

## 5. Signals — fragment-based events with replay semantics

A signal is not a global bus: it is a **fragment on the entity** (`ck::TFragment_Signal<...>`,
`CkSignal_Fragment.h:33-91`) holding the last payload, its frame number, and two `entt::sigh`
channels (persistent + fire-once). `Bind`/`Broadcast` `AddOrGet` that fragment on the handle
(`CkSignal_Utils.inl.h:49, 89`). Signal payload args may not be references or raw pointers —
static-asserted (`CkSignal_Fragment.h:47-48`).

**Binding policies — decided by one line** (`CkEcs/Public/CkEcs/Signal/CkSignal_Utils.inl.h:92-94`):

```cpp
const auto ShouldFirePayloadInFlight = ck::IsValid(Signal._Payload) &&
    (Signal._PayloadFrameNumber == UCk_Utils_Time_UE::Get_FrameCounter() ||
    T_PayloadInFlightBehavior == ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
```

| Policy (`ECk_Signal_BindingPolicy`, CkSignal_Fragment_Data.h:10-19) | On bind, if a payload was already broadcast... |
|---|---|
| `FireIfPayloadInFlightThisFrame` | replay it only if broadcast **this same frame** |
| `FireIfPayloadInFlight` | replay the last stashed payload **from any frame** (promise-read) |
| `IgnorePayloadInFlight` | never replay; future broadcasts only |

Only the **last** payload is ever replayed — if the signal fired N times before you bound, N-1
payloads are gone; use an Array payload if you need all (enum comments, `CkSignal_Fragment_Data.h:12-16`).

**Origin of the split** (verified from git): commit `7f38dad33` (2023-11-08) added
`FireIfPayloadInFlightThisFrame` as a distinct policy because promise-style binds ("Futures") were
missing payloads broadcast on earlier frames — payloads were being treated as stale. Post-fix:
promises bind `FireIfPayloadInFlight` (any-frame replay, what `CK_SIGNAL_BIND_PROMISE` uses —
`CkSignal_Macros.h:44-45`); the this-frame policy is the default for ordinary binds
(`CkTimer_Utils.h:258`).

**PostFire `Unbind` is a separate generated fragment type.** `CK_DEFINE_SIGNAL_WITH_DELEGATE`
emits TWO fragments — `FFragment_Signal_Delegate_<Name>` (PostFire `DoNothing`) and
`FFragment_Signal_Delegate_<Name>_PostFireUnbind` (`CkSignal_Macros.h:18-22`) — and
`CK_SIGNAL_BIND` routes between them with a runtime `if` that token-pastes `##_PostFireUnbind`
onto the utils class name (:51-55). Consequence: the macro's first argument must be the utils
class name itself (real call sites pass `ck::UUtils_Signal_X`).

**Unbind + replay never connects** (`CkSignal_Utils.inl.h:104-106`): if a bind with PostFire
`Unbind` replays an in-flight payload, the candidate is invoked immediately and **never attached**
to the signal — a fire-once bind that already fired leaves no connection to leak. On the
BP-delegate path, a replayed payload fires **only into the delegate being bound right now** —
previously bound delegates are stashed and restored around the replay
(`EnsurePayloadInFlightIsOnlyFiredOnLatestDelegate`, inl.h:318-346).

**Broadcast stashes first, publishes from the stash** (inl.h:53-76): the payload tuple is moved
into the fragment, then published from the stored tuple — publishing from the in-flight arguments
once shipped a real bug (listeners observed moved-from, empty TArrays; GOAP autotest repro recorded
in the comment at inl.h:55-63). `FTag_PayloadInFlight` is added to the handle after broadcast (:78).

**Pointer stability (`in_place_delete`) — load-bearing, deliberate.**
`entt::sigh`/`sink` hold pointers into the signal fragment; if EnTT relocated fragments on
swap-and-pop deletion, connections dangled — signals "randomly disconnected". Commit `2c8319c1c`
(2023-11-09) turned on the pointer-stability guarantee for signal fragments as an explicit
workaround (in-class `static constexpr auto in_place_delete = true;` members), its message
promising "a proper fix upcoming". The chain that followed (re-derived 2026-07-02):

- Signal fragment types still carry `static constexpr auto in_place_delete = true;`
  (`CkSignal_Fragment.h:44, 99`) — now shadowed by the global trait below.
- `745507381` (2024-03-07) introduced a **global** `entt::component_traits<Type>` partial
  specialization forcing `in_place_delete = true` for every fragment type, with `page_size = 0`
  for empty types (tags keep the empty-type optimization) — born INSIDE the handle-debugging
  `#if` gate; `6b54d2e384` (2024-04-12) merely relocated the still-gated block.
- **`06938bba3` (2026-02-17, "feat: fragments are always pointer stable") deliberately removed
  the gate** — the unconditional form at `CkEcs/Public/CkEcs/Handle/CkHandle.h:71-77` is a
  considered storage-policy decision (DECISIONS.md §45).

Net effect today: **all non-empty fragment storage in Ck registries is pointer-stable** — deletion
leaves tombstones instead of compacting. Treat "fragment references stay valid until entity
destruction completes" as a load-bearing invariant, and tolerate tombstones when touching raw EnTT
iteration. This is settled design (DECISIONS.md §45); the owning-groups performance ceiling it
implies is tracked as `ck-feature-frontier` candidate 5. The lifecycle campaign treats it as
fenced context, not a target.

Binding in all three environments (Timer done-signal):

```cpp
// C++ (macro form, inside Utils)
CK_SIGNAL_BIND(ck::UUtils_Signal_OnTimerDone, InTimerEntity, InDelegate,
    ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
    ECk_Signal_PostFireBehavior::DoNothing);
```
Blueprint: node `[Ck][Timer] Bind To OnDone` (Category `Ck|Utils|Timer`, policy/post-fire pins).
```angelscript
// AngelScript
TimerHandle.BindTo_OnDone(FCk_Delegate_Timer(this, n"OnTimerDone"),
    ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
    ECk_Signal_PostFireBehavior::DoNothing);
```

---

## 6. Dependency scoping — there is no DI module

Nothing in the framework is named "DependencyInjection" (zero matches in Source). The two real
dependency channels:

| Channel | What it is | Use it for |
|---|---|---|
| **ContextOwner** (`CkEcs/ContextOwner/CkContextOwner_Utils.h:29-48`) | Every entity's DI-style context root: `Get_ContextOwner(InHandle)` (BP compact node "CTX"), `Request_Override`, `Request_OverrideToSelf` | Runtime scoping along the entity graph — "which entity is my logical owner" (an ability's entity is owned by the character entity). Walk to the owner, then query features on it. |
| **CkProvider** (`CkProvider/Public/CkProvider/CkProvider_Data.h`) | Abstract data-asset value providers (`UCk_Provider_PDA` + typed `UCk_Provider_Bool_PDA`, `_Float_PDA`, ... as `EditInlineNew` assets) | Designer-authored configurable values resolved at runtime with entity context: `InParams.Get_MyProvider()->Get_Value(InHandle)` (pattern: `Source/CLAUDE.md` "Reading values from providers") |

Everything else is plain composition: features arrive via `UCk_Utils_X_UE::Add(...)` and read
their config from their own params fragment. If you find yourself wanting a service locator or a
global singleton, you are usually looking for either the ContextOwner chain (runtime) or a
provider on the params (config).

---

## 7. Replication — the deferred-Apply contract

Client-side application of replicated fragment-container data is **fully deferred** — net receive
and driver link only mark entries pending; one processor applies them.

- **Registry**: `FCk_ReplicatedFragmentHandlerRegistry`
  (`CkEcs/Public/CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h:40-104`).
  Features register in their `_Fragment.cpp` via `RegisterLazy` (type resolved on first use —
  safe during static init, :66-70) or `RegisterFallback` (runtime-typed payloads, e.g. dynamic
  fragments, :72-80).
- **Handler contract** (comment at :45-52): `Apply(Entity, New, TOptional Old) -> Applied | NotReady`.
  Runs AFTER OnConstructed-driven composition, never inline during net receive. Return `NotReady`
  while the target feature is not composed yet. **Never compose the feature from inside Apply** —
  composition belongs to construction. `Old` is unset on the first application, otherwise the last
  APPLIED data (coalesced receives diff against what was applied, not last received).
- **Dispatch + timeout**: `FProcessor_ReplicatedFragments_Dispatch` (ClientOnly, drains
  `FTag_RepFragments_PendingApply`) retries `NotReady` entries each tick; past
  `PendingApplyTimeoutSeconds` — **5s (non-Shipping) / 2s (Shipping)** — it drops the entry with a
  `CK_TRIGGER_ENSURE` naming the type and entity
  (`CkReplicatedFragmentContainer_Processor.cpp:16-20, 79-93`). A perpetual timeout means the
  feature is never composed on the client — fix composition, don't widen the timeout.
- **Ordering — the two-signal client lifecycle**: the dispatcher's scheduling contract is written
  in its own header (`CkReplicatedFragmentContainer_Processor.h:13-23`): it runs after
  `FProcessor_EntityScript_FinishConstruction` in `FGroup_Gameplay_Script`, which precedes
  `FGroup_Replication` where `OnReplicationComplete` broadcasts. Therefore:
  **`OnConstructed` = composed, NOT values-applied.** Compose features there; read replicated
  values (team, attributes, SM state) only from
  `UCk_Utils_EntityReplicationDriver_UE::Promise_OnReplicationComplete`
  (`CkEcs/Public/CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h:117`) — it
  fires retroactively if bound late (payload-in-flight semantics). Pinned by the
  `Ck.Attribute.Net.*` tests named in `CkEcs/Claude.md`.

---

## 8. Invariants — what must hold

Each line is a contract; breaking one is an architecture change, not a refactor.

1. **A typed handle is exactly `sizeof(FCk_Handle)`** — never add members
   (`CkHandle_TypeSafe.h:76-80, 144-145`).
2. **Typed handles are minted only through `Cast`/`CastChecked`/`ck::StaticCast`** — the private
   ctor + friend is the whole point (§2). AS parent up-conversion is the one unchecked path.
3. **Typed handles are declared in `X_Fragment_Data.h`** (89/90 as of 2026-07-02; §2).
4. **All non-empty fragment storage is pointer-stable** (`CkHandle.h:71-77`) — fragment references
   survive until destruction completes; raw-EnTT code must tolerate tombstones (§5, A3).
5. **Entity destruction is a multi-frame pipeline** — Initiate → EndPlay → Teardown → Await →
   Finalize over ~3 frames (diagram: `CkEcs/Public/CkEcs/EntityLifetime/CkEntityLifetime_Fragment.cpp:30-70`);
   normal processors exclude dying entities via `CK_IGNORE_PENDING_KILL`
   (`CkEntityLifetime_Fragment.h:37-41`); cleanup work runs in `CK_IF_END_PLAY`/`CK_IF_TEARING_DOWN`
   passes. Never destroy fragments/components inside a normal `ForEachEntity`.
6. **Requests are drained exactly once per HandleRequests pass via copy-then-reset**; re-entrant
   enqueues survive to the next pass; the dirty tag is removed only when the live list is empty
   (`CkTimer_Processor.cpp:48-66`).
7. **Request structs are one-shot** — `PopulateRequestHandle` at most once per instance
   (`CkRequest_Data.h:16-18`).
8. **Every request struct carries `CK_REQUEST_DEFINE_DEBUG_NAME`** (root doctrine; measured
   2026-07-02: 171 of 184 `FCk_Request_*` comply — the gap is legacy, not license).
9. **Signal payload args: no references, no raw pointers** (`CkSignal_Fragment.h:47-48`);
   broadcast publishes from the stored payload, never the in-flight arguments (inl.h:55-63).
10. **A fire-once bind that replays never connects** (`CkSignal_Utils.inl.h:104-106`) — code must
    not assume a bind left a connection behind.
11. **Every UENUM gets `CK_DEFINE_CUSTOM_FORMATTER_ENUM`** (root doctrine; measured 318 formatter
    sites vs 380 UENUMs — close the gap when touching a file, don't widen it).
12. **Processors self-register exactly once, in their own `.cpp`** (`CK_REGISTER_PROCESSOR`,
    388 registrations measured 2026-07-02 (+1 commented example, +1 `#define`); the old
    ProcessorInjector mechanism is retired — root doctrine).
13. **Replication `Apply` never composes; `OnConstructed` ≠ values-applied** (§7).
14. **UE GC does not trace fragment members** — any UObject only a fragment points at WILL be
    collected unless rooted; pick `TStrongObjectPtr` (entity owns lifetime) vs `TWeakObjectPtr`
    (observation) per the root doctrine's ownership split (§9 below).
15. **Direct `_Member` writes only inside a fragment's declared friends** (§4) — everything else
    reads `Get_*` or enqueues a request.

---

## 9. Known-weak points — stated plainly

1. **GC blind spot (mitigated, not solved).** Fragment members are invisible to UE's GC; the
   ownership-split rule (TStrong/TWeak) is the mitigation, and packaged builds once crashed on
   exactly this class of bug — diagnosed `d77810096`, root-caused with a pre-GC rooting pass
   `feb08ee94`, tripwire-hardened `a8a93baac` (tripwire = detection, not the fix). Full incident
   history: `ck-failure-archaeology`.
2. **Teardown/unbind debt cluster (live defect).** `CkInteraction/Public/CkInteraction/InteractTarget/CkInteractTarget_Processor.cpp:222`
   carries the verbatim TODO *"This processor doesn't get called, can cause issues if teardown is
   mid interaction!!!"*; `CkRelationship/Public/CkRelationship/Team/CkTeam_Utils.cpp:376, 402, 428`
   carry three copies of *"figure out a bullet-proof way to remove the FTag_TeamListener if ALL the
   delegates have been unbound"*. This is the campaign target of `ck-lifecycle-teardown-campaign`
   — coordinate there before touching teardown paths.
3. **Request vtable config-variance.** `FCk_Request_Base`/`ck::FRequest_Base` are polymorphic only
   when `CK_DISABLE_ECS_HANDLE_DEBUGGING` is off (`CkRequest_Data.h:46-54, 95-103`) — request
   structs have a vptr (different `sizeof`) in Debug/Dev-editor and none in Test/Shipping. No code
   currently depends on request layout across configs; never memcpy, static_assert sizes, or
   serialize requests by layout (DECISIONS.md #27 documents this as a constraint, not endorsed
   design).
4. **`in_place_delete` costs iteration density framework-wide — by design.** Signal-local pointer
   stability (2c8319c1c, "proper fix upcoming") was later subsumed by the global
   `component_traits` specialization — introduced debug-gated (745507381, 2024-03-07),
   deliberately made unconditional in `06938bba3` (2026-02-17, "fragments are always pointer
   stable"; DECISIONS.md §45). The cost is real: owning groups are unavailable and tombstone
   storage can mask dangling-view bugs that swap-delete would surface — now a prioritization
   question (`ck-feature-frontier` candidate 5), no longer an open doctrine fork.
5. **`TOptional` in reflected surfaces is contested.** The doctrine teaches enum-mode + value; the
   newest modules (CkAudio, CkPmg) ship UPROPERTY `TOptional`s. Open as ADJUDICATIONS.md **A1** —
   match the file you are editing; do not churn either direction.

---

## Common mistakes

- Declaring a typed handle in `X_Fragment.h` — it belongs in `X_Fragment_Data.h` (§2).
- Treating a silent `Cast` failure as impossible — `Cast` returns an invalid handle without
  ensuring; only `CastChecked` is loud (§2).
- Trusting an AS parent-typed parameter to have validated the handle — up-conversion is unchecked
  (§2); the ensure fires later, deeper.
- Binding a late listener with `IgnorePayloadInFlight` and wondering why it never fires — a
  promise-read wants `FireIfPayloadInFlight` (§5, the 7f38dad33 lesson).
- Iterating `_Requests` in place or re-dispatching the copy without `DontResetContainer` — breaks
  the exactly-once/re-entrancy contract (§3).
- Reading team/attribute/SM values in `OnConstructed` on a client — values apply later; use
  `Promise_OnReplicationComplete` (§7).
- Composing a feature inside a replication `Apply` handler — return `NotReady` and let
  construction do it (§7).
- Storing a UObject only in a fragment and expecting it to survive GC (§9.1).
- Mutating another feature's `_Member` by adding yourself as a friend for convenience — the friend
  list is the audited write surface (§4).

---

## Provenance and maintenance

Verified 2026-07-02 against CkFoundation submodule HEAD `7330c1bab` (working tree: Source/ clean).
Re-verification commands (Git Bash, cwd `d:\Repos\BusterBlock\Plugins\CkFoundation`):

- Handle machinery lines: `rg -n "private:" Source/CkEcs/Public/CkEcs/Handle/CkHandle_TypeSafe.h`
  (private ctor at the end of the TYPESAFE macro); `rg -n "static_assert" Source/CkEcs/Public/CkEcs/Handle/CkHandle_TypeSafe.h`
- Typed-handle placement: `rg -c "CK_GENERATED_BODY_HANDLE_TYPESAFE\(" Source --glob '*.h'` then
  `rg -l ... | grep -v _Fragment_Data.h` (expect the base header + CkShape_Handle.h only)
- Derived handles: `rg -n "CK_GENERATED_BODY_HANDLE_DERIVED\(" Source --glob '*.h'` (7 uses + 1 definition)
- Signal policy line: `rg -n "ShouldFirePayloadInFlight" Source/CkEcs/Public/CkEcs/Signal/CkSignal_Utils.inl.h`
- Global pointer stability: `rg -n "component_traits" Source/CkEcs/Public/CkEcs/Handle/CkHandle.h`
- Origin commits: `git show --stat 7f38dad33 2c8319c1c 745507381 6b54d2e384 06938bba3 d77810096 feb08ee94 a8a93baac`
- Request one-shot + vtable variance: `rg -n "CK_DISABLE_ECS_HANDLE_DEBUGGING" Source/CkEcs/Public/CkEcs/Request/CkRequest_Data.h`
- Completion-bind sites: `rg -c "CK_SIGNAL_BIND_REQUEST_FULFILLED\(" Source --glob '*.{h,cpp}'` (19 + definition header)
- Friend counts: `rg -c "friend class FProcessor_" Source --glob '*_Fragment.h'` (384) and
  `"friend class UCk_Utils_"` (105)
- Request debug-name coverage: `rg -c "CK_REQUEST_DEFINE_DEBUG_NAME\(" Source --glob '*.h' | grep -v CkRequest_Data`
  (171) vs `rg -o "struct \w+ FCk_Request_\w+" Source --glob '*.h' -N | sort -u | wc -l` (184)
- Processor registrations: `rg -c "CK_REGISTER_PROCESSOR\(" Source --glob '*.cpp'` (sums to 388; a commented example in `CkAStar_Processor.h:45` and the `#define` are the only non-.cpp hits)
- Dispatch timeout: `rg -n "PendingApplyTimeoutSeconds" Source/CkEcs/Public/CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer_Processor.cpp`
- Weak-point TODOs: `rg -n "teardown is mid interaction" Source/CkInteraction` and
  `rg -n "bullet-proof way to remove the FTag_TeamListener" Source/CkRelationship`
- Formatter/UENUM coverage: `rg -c "CK_DEFINE_CUSTOM_FORMATTER_ENUM\(" Source --glob '*.h'` vs `rg -c "^UENUM" Source --glob '*.h'`
- Open forks: `.claude/reports/ADJUDICATIONS.md` (A1; A3 is resolved → DECISIONS.md §45) — re-read before repeating interim stances.

Tooling caveat (root doctrine): Grep/Glob tools can silently miss files under this plugin — on any
zero-match, re-check with `rg --no-ignore` before concluding absence.
