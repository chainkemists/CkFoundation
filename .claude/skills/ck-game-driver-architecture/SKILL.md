---
name: ck-game-driver-architecture
description: Use when building the orchestration layer of a CkFoundation game — a world-singleton
  or per-scope "driver"/director entity script that spawns subordinates, discovers world-placed
  entities by tag, routes requests, and signals readiness. Triggers - "who spawns the managers",
  "how do I find the <X> singleton", "my tag scan found entities from another building/test",
  "discovery handler runs every frame", "consumer bound before the driver existed and nothing
  fired", "EntityTagQuery re-delivers the full set", driver-ready gating, subordinate roster,
  Acquire ticket / Promise_OnReady. For BUILDING games ON CkFoundation. Not for choosing an
  entity archetype in general (ck-game-entity-composition-patterns), replication owner rules and
  rep-notify ordering (ck-game-replication-patterns), or the per-feature file checklist
  (ck-game-feature-recipe); for framework signal/query internals, see ckecs-architecture-contract.
---

# Driver architecture — the Controller layer of a CkFoundation game

`[PROMOTED FROM CORPUS 2026-07-03 — maintainer-endorsed]` This pattern appears only in the
BusterBlock corpus (the older Venus project predates it), but the framework maintainer explicitly
endorsed it as the standard on 2026-07-03: **"It follows the MVC architecture where the Driver is
the Controller."** Treat it as settled doctrine for new games.

The MVC mapping, in framework terms:

| MVC role | In a CkFoundation game | Owns |
|---|---|---|
| **Model** | Feature fragments + state on world entities (Params/State/Requests/Signals structs, attributes) | Data and mechanism — composed by `utils_<feature>::Add` |
| **View** | Renderers (Iskm/ISM proxies), UMG/world-space widgets, debugger pages, DevViz | Presentation — reads Model, never owns policy |
| **Controller** | The **Driver** entity script | Orchestration and policy: what exists, when it spawns, how requests route, when the world is "ready" |

A **driver** is an entity script (see `ck-game-entity-composition-patterns` for the archetype
census) that acts as the *context root for a scope of gameplay*: it spawns and owns subordinate
entity scripts, discovers world-placed entities by tag, wires their signals together, exposes a
readiness gate, and routes requests (console/UI/gameplay) to whichever subordinate owns the
mechanism. Subordinates own *mechanism*; the driver owns *policy*.

Corpus examples (BusterBlock, verified 2026-07-03): `UBb_StoreDriver_EntityScript` (per-store
director, ~11 subordinates — `Script/StoreDriver/BB_StoreDriver_EntityScript.as`),
`UBb_MissionDriver_EntityScript`, `UBb_DayCycle_EntityScript`, `UBb_EventFeed_EntityScript`,
`UBb_CollectiblesDriver_EntityScript`, `UBb_RentnetKioskDriver_EntityScript`,
`UBb_DayNightLampDriver_EntityScript` (client-local).

## When NOT to use this skill

| You are actually doing | Go to |
|---|---|
| Choosing which entity archetype a new thing should be | ck-game-entity-composition-patterns |
| Deciding lifetime owner for a replicated spawn, rep-notify bind order, authority split | ck-game-replication-patterns |
| Writing the feature itself (Feature/Utils/Processor files, discovery-tag stamping rules) | ck-game-feature-recipe |
| AS idioms, hot-reload, mixin call forms | ck-game-angelscript-gameplay |
| Why EntityTagQuery / signals behave the way they do (framework internals) | ckecs-architecture-contract (framework skill) |
| Testing a driver (readiness gating in tests, contamination) | ck-game-testing-discipline |

## 1. What a driver IS — and is NOT

A driver **is**:

- **An entity script, never an Actor or a UE subsystem.** BusterBlock deleted its
  `UScriptWorldSubsystem` drivers and moved the spawns into the GameState precisely because
  post-login entity-script spawning removed 0.5s tag-race timers
  (`Script/WorldSettings/Gameplay/BB_Gameplay_GameState.as:9-11`, verified). The No-Actors
  doctrine applies (root doctrine: `Plugins/CkFoundation/CLAUDE.md`).
- **A scoped context root.** One per world (MissionDriver, DayCycle) or one per gameplay scope
  (StoreDriver: one per store — multi-store maps have several). Its subordinates and discovered
  dependencies are reachable *through it*, and it is itself found through a *scoped* lookup
  (§3), never a global.
- **The readiness authority for its scope.** Consumers do not poll world state; they ask the
  driver "are you ready" via a promise (§5).

A driver is **NOT**:

- **A global service locator.** The maintainer names anything requiring GLOBAL ACCESS the worst
  debt in the ecosystem (ruling, 2026-07-03). No static singleton accessor, no
  "GetDriverManager()" on the GameInstance. Every sanctioned access form in §3 is scoped by a
  world-context handle and degrades to an explicit, promise-gated wait.
- **A god object.** Mechanism stays in feature Utils (`utils_<feature>::Add`, accessors, signal
  binders); the driver only decides *which* features exist, *where* they anchor, and *how*
  requests route. §7 has the smell test.
- **A Tick host.** Drivers are signal- and promise-driven. The corpus drivers contain no per-tick
  logic; the one perf incident they had came from a signal that fires per-frame (§4, delta-gating).

## 2. Spawn topology — who spawns a driver, and under what owner

Three verified spawn patterns. Pick by scope and replication:

| Driver kind | Spawned by | Lifetime owner | Replication | Corpus example |
|---|---|---|---|---|
| World singleton, replicated | GameState `EcsConstructionScript`, **server-only** | **ActorRelay channel entity** | `Replicates` | MissionDriver, DayCycle, EventFeed (`BB_Gameplay_GameState.as:66-157`) |
| World singleton, server-only | Same GameState path | ActorRelay channel entity (same anchor is fine) | `DoesNotReplicate` | CollectiblesDriver, ItemOverflowDriver (`BB_Gameplay_GameState.as:131-156`) |
| Scoped director (one per store/region/match) | **Level-placed** via an entity-spawner placer BP, or spawned by a parent driver | Placer/parent; acquires its **own** ActorRelay channel for its replicated subordinates | `Replicates` | StoreDriver (`BB_StoreDriver_EntityScript.as:23-33`) |
| Client-local presentation driver | Spawns on **every** machine | `ck::TransientEntity()` | `DoesNotReplicate` | DayNightLampDriver (`Script/ECS/DayNightLampDriver/BB_DayNightLampDriver_Subsystem.as:6` — legacy filename; it is an entity script, not a UE subsystem) |

The owner rule (why the channel, not `ck::TransientEntity()`, for anything replicated) is owned
by **ck-game-replication-patterns** — one line here: a `Replicates` entity script spawned under a
non-replicating owner is silently broken and floods `[REP_DEBUG]`
(`Script/ECS/MissionDriver/BB_MissionDriver_EntityScript.as:4-6`, verified).

GameState spawn sketch — server-only acquire → promise → spawn each driver under the channel
entity (full canonical skeleton: `ck-game-replication-patterns` §2):

```angelscript
// EcsConstructionScript (server-only — guard with System::IsServer()):
_PendingChannel = utils_actor_relay::Request_AcquireChannel(GameplayTags::ActorRelay_Generic);
_PendingChannel.Promise_OnAcquired(FCk_Delegate_ActorRelay_Acquired(this, n"OnChannelAcquired"));
// OnChannelAcquired: validate + cache Get_ChannelEntity() as _ChannelLifetimeOwner, then per driver:
_PendingMyDriver = utils_entity_script::Request_SpawnEntity(
    _ChannelLifetimeOwner, U<Prefix>_MyDriver_EntityScript, U<Prefix>_MyDriver_EntityScript::Params());
utils_pending_entity_script::Promise_OnConstructed(
    _PendingMyDriver, FCk_Delegate_EntityScript_Constructed(this, n"OnMyDriverConstructed"));
```

**Sibling-driver dependency = inject, don't re-discover.** When driver B needs driver A's handle,
spawn B *from A's construction promise* and pass the handle through `Params(...)`. Corpus: the
RentnetKioskDriver is spawned only inside `OnDayCycleConstructed` so a valid clock handle is
injected (`BB_Gameplay_GameState.as:116-119,183-201`, verified). Injecting a not-yet-Ready handle
is fine when the receiver Ready-gates on it internally.

## 3. Reaching a driver — the sanctioned forms (no globals)

Verified BusterBlock reality: drivers are reached by **tag scan scoped through the
lifetime-owner chain**, or by an **Acquire ticket + promise**, or by **injection** (§2). Note:
`ck::Ctx` is how SM/task micro-entities resolve their *own subject entity* — it is **not** the
driver-lookup mechanism (verified by grep: all `ck::Ctx` call sites are SM tasks/conditions
resolving their host).

Ranked, strongest first:

1. **Injection** — the dependency arrives in `Params(...)` at spawn. Zero discovery, zero races.
   Use whenever the spawner already holds the handle.
2. **Acquire ticket + promise** — for consumers that may run before the driver exists. The
   feature's Utils exposes `Acquire<Driver>(WorldContext)` returning a pending handle whose
   `Promise_OnReady` fires *now* if the driver is already ready, else is late-resolved by a flush
   processor. This is the guaranteed-delivery form; teach it to every consumer of your driver.
3. **Scoped tag scan** — `TryFind_<Driver>(Context)` for opportunistic, may-return-invalid reads.

The Acquire-ticket recipe (three pieces, all in the driver's feature dir — verified against
`Script/ECS/DayCycle/BB_DayCycle_Utils.as:56-93` and
`Script/StoreDriver/BB_StoreDriver_Utils.as:112-150` + `BB_StoreDriver_Processor.as:6-36`):

```angelscript
namespace utils_my_driver
{
    // (a) Scoped scan. Descendant-preferring: first lifetime-descendant of InContext
    //     wins (disambiguates multi-instance maps and isolates concurrent tests);
    //     global fallback covers consumers outside the lifetime chain (HUD widgets).
    FCk_Handle_MyDriver TryFind_Driver(const FCk_Handle& InContext)
    {
        auto FirstAny = FCk_Handle_MyDriver();
        for (auto RO : utils_entity_tag::ForEach_Entity(InContext, n"TAG_<Prefix>MyDriver"))
        {
            auto Driver = RO.As_MyDriver(ECk_SanityCheck::UnChecked);
            if (ck::Is_NOT_Valid(Driver))
            { continue; }
            if (Is_DescendantOf(RO, InContext))
            { return Driver; }
            if (ck::Is_NOT_Valid(FirstAny))
            { FirstAny = Driver; }
        }
        return FirstAny;
    }

    // (b) Ticket factory. The driver need not exist yet.
    FCk_Handle_PendingMyDriver AcquireMyDriver(const FCk_Handle& InWorldContext)
    {
        auto PendingEntity = utils_entity_lifetime::Request_CreateEntity(InWorldContext);
        PendingEntity.Add_Fragment(F<Prefix>_Feature_PendingMyDriver());
        auto& Frag = PendingEntity.AddOrGet_Fragment(F<Prefix>_Fragment_PendingMyDriver);
        Frag.Driver = TryFind_Driver(InWorldContext);
        return PendingEntity.As_PendingMyDriver();
    }
}

// (c) Promise mixin: fire inline if already ready, else stamp Unresolved so the
//     flush processor (dirty-gated on that tag) retries the find and fires later.
mixin void Promise_OnReady(FCk_Handle_PendingMyDriver& Self, F<Prefix>_Delegate_MyDriver_OnReady InDelegate)
{
    auto& Frag = Self.AddOrGet_Fragment(F<Prefix>_Fragment_PendingMyDriver);
    Frag.Promise = InDelegate;
    if (ck::IsValid(Frag.Driver) && Frag.Driver.Get_IsReady())
    { InDelegate.ExecuteIfBound(Frag.Driver); return; }
    Self.AddOrGet_Fragment(F<Prefix>_Tag_PendingMyDriver_Unresolved);
}
```

Scoping details that earn their keep (all verified in `BB_StoreDriver_Utils.as:50-107`):

- `Is_DescendantOf` walks `Get_LifetimeOwner` upward, depth-capped, stopping at the transient
  entity (which has no owner — walking past it ensures).
- Provide a `_Strict` variant that rejects non-descendants; tests pass strict so a stale
  still-Ready sibling-test driver can't resolve their promise with the wrong handle.
- The flush processor resolves through the **ticket's lifetime owner**, not the ticket entity,
  so the retry path scopes identically to the original call.
- Singleton tags are **opt-in per instance**, not baked into `Add`: BB's DayCycle stamps
  `TAG_BbDayCycle_Global` only via an explicit `Mark_AsGlobal` "so gym/test entities don't crowd
  out each other under one shared tag" (`BB_DayCycle_Utils.as:24-25`, verified). Give your
  singleton drivers the same split: a feature tag stamped by `Add`, a global/instance tag stamped
  deliberately by the production spawner.

## 4. Discovery done right — the mechanics

Discovery/composition timing is the #1 failure mode the maintainer flags (ruling 4, 2026-07-03).
**ck-game-feature-recipe owns the canonical warning and the producer-side rule (stamp the
discovery tag LAST, after the feature finishes composing).** This section owns the consumer
(driver) side. The StoreDriver is the reference implementation
(`BB_StoreDriver_EntityScript.as:1-21` header, whole file verified 2026-07-03).

### 4.1 Never construct-time-scan and stop

A one-shot scan in `DoConstruct` misses everything that composes later: async
EntitySpawnParams placers, runtime-placed fixtures, entities whose tag lands a frame after
yours. Corpus fix commits for exactly this: `66ac804db` (defer-rescan so late-composing entities
still bind), `1ca589b0d` (async-composed gondolas). The verified two-axis model:

- **Axis 1 — always-on wall-clock window** (`DiscoveryTimeoutSeconds`, production default 5.0s;
  tests pass ~0.5s; `<= 0` collapses to immediate-ready for degenerate scenes). Run the sync
  bind sweep at `DoConstruct` **and again at window close** to catch entities tagged during the
  window.
- **Axis 2 — optional minimums contract** via `CkEntityTagQuery` (`MinExpected<Category>` /
  `Require<Thing>`): the query fast-paths Ready the moment requirements are met (cancelling the
  timer); if the window closes unmet, emit per-category warnings, stamp a diagnostic tag, and
  **still mark Ready so consumers don't hang** (`BB_StoreDriver_EntityScript.as:3-11, 719-745`).

Categories split into **readiness-gating essentials** (things the scope cannot function without)
vs **fixtures** discovered opportunistically forever and never gating Ready (StoreDriver:
managers/signs gate; gondolas/entryways/counters are fixtures with a persistent runtime watcher —
`:462-475`). Decide the split up front; changing it later strands dead `MinExpected*` fields kept
only for `::Params()` positional stability (`:146-181` — a real scar, see
ck-game-entity-composition-patterns on ExposeOnSpawn append-only discipline).

### 4.2 Per-category binders: dedupe, validate, no signal binding

Every discovery path (sync scan, query fire, runtime watcher, explicit refs) funnels into one
idempotent `BindOne_<Category>` that dedupes against a Dependencies fragment
(`:862-873`):

```angelscript
private void BindOne_Widget(FCk_Handle& InContext, FCk_Handle_Widget& InWidget)
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }
    auto& Deps = InContext.AddOrGet_Fragment(F<Prefix>_Fragment_MyDriver_Dependencies);
    if (Deps.Widgets.Contains(InWidget))
    { return; }
    Deps.Widgets.Add(InWidget);
}
```

- **Validate the tag→feature contract on every scan hit** and name the producer-side failure in
  the message: `ck::EnsureIfNot(ck::IsValid(Widget), "...tagged but not a valid Widget — tag
  stamped before the feature finished composing?")` (`:856-858` and six siblings, verified).
  This ensure is your early-warning system for producer features violating the tag-last rule.
- **Explicit ExposeOnSpawn refs and AutoDiscover are mutually exclusive per category** — refs
  win and skip the scan (`:841-860`). Cardinality-1 categories are first-wins with a warning on
  extras telling the designer to pass an explicit ref (`:1046-1072`).
- **COLLECT and BIND are separate phases**: discovery only fills the Dependencies fragment;
  signal binding happens once, in the Ready-state tasks, after the set is final (`:837-840`).

### 4.3 Delta-gate every persistent query handler

`EntityTagQuery` **OnContinuousUpdate fires every Evaluate pass — every frame — by design** (the
query processor is not dirty-gated; tested framework contract, see ckecs-architecture-contract).
And **All-mode OnSatisfied re-delivers the full match set** on each fire. Two consequences,
both with corpus incidents:

1. A handler that rebuilds/diffs its tracked set unconditionally costs real frame time —
   BusterBlock's three driver discovery handlers combined for **~250ms/frame** before commit
   `531b0c956` (verified diff). The fix pattern:

```angelscript
private bool _HasReconciled = false;

UFUNCTION()
private void OnPopulationChanged(FCk_Handle_EntityTagQuery InQuery, bool InIsSatisfied,
                                 const TArray<FCk_EntityTagQuery_Result>&in InResults)
{
    if (InResults.Num() == 0)
    { return; }
    // Skip no-delta frames AFTER a guaranteed first reconcile — the bind-time
    // FireIfPayloadInFlight pass carries the full population with EMPTY deltas,
    // so gating on deltas alone would drop the initial seed.
    if (_HasReconciled
        && InResults[0]._Added.Num() == 0 && InResults[0]._Removed.Num() == 0)
    { return; }
    _HasReconciled = true;
    // ... reconcile from InResults[0]._Handles (full set), dedupe via your Bound set ...
}
```

2. Even with the gate, keep the reconcile itself idempotent (the `BindOne_` dedupe) — re-delivery
   of the full set must be harmless (`:697-715`).

### 4.4 SCOPE the discovery — tag scans are registry-wide

`utils_entity_tag::ForEach_Entity` sees **every** tagged entity in the world. On any map with
more than one instance of your scope, an unscoped scan claims foreign entities. Corpus incident
(commit `287ee6601` + `Prune_ForeignEntryways`, `BB_StoreDriver_EntityScript.as:621-695`,
verified): the StoreDriver's registry-wide entryway scan bound *every* building's doors — 94% of
customer "entered" occupancy adds came from foreign doors, and NPCs were lured to the wrong
building. Scoping mechanisms, in preference order:

1. **Explicit refs** (ExposeOnSpawn) — the designer names exactly which instances belong.
2. **Lifetime-descendant filtering** — if the driver spawns/owns the entities, scope by
   `Is_DescendantOf` (this is what makes multi-store and concurrent tests work, §3).
3. **Spatial pruning from an INTERIOR anchor** — for level-placed entities with no ownership
   link, prune anything beyond a radius measured from an anchor *deep inside* the scope
   (StoreDriver: centroid of its store managers; anchoring on the front sign forced the radius
   up into foreign-door range — `:626-635`). Fail **open** (keep all) when no anchor resolves.
4. **Per-scope tags** — a distinct tag per instance; heavier authoring cost, use when 1–3 don't fit.

Run the prune once, after discovery closes and **before** Ready is stamped.

## 5. Readiness and ordering — consumers gate, drivers signal

Drivers spawn **after** boot (post-login GameState spawn; entity-spawner placers run async). Any
consumer that reads a driver at its own construct time is racing. The contract:

- **The driver stamps a Ready tag exactly once**, when its init counter hits zero:
  `Progress.Pending = <enabled subordinate spawns> + 1 (discovery)`, each step decrements. The
  seed **must** match the decrementers exactly — under-count hangs Init forever, over-count
  stamps Ready early (`BB_StoreDriver_EntityScript.as:372-386`, an in-code INVARIANT comment).
  Every abort path still decrements (`Abort(...)` in the spawn tasks — `BB_StoreDriver_Hfsm_Tasks.as:1196-1204`).
- **Construction-order invariants** inside the driver's `DoConstruct` (all verified, `:394-408`):
  InitParams written → readiness counter seeded → SM added (with **deferred** AutoStart — a task
  entering before `DoConstruct` returns would miss inline decrements, `:417-421`) → discovery
  begun → **async promises armed LAST**. Promises can resolve INLINE (listen server: the relay
  channel already exists at login) — everything a callback touches must already exist.
- **Consumers use the Acquire ticket** (§3), which fires now-or-later against `Get_IsReady`. The
  MissionDriver is the model consumer: it `AcquireStoreDriver(...).Promise_OnStoreDriverReady(...)`
  in `DoBeginPlay` and only binds Economy signals inside the callback
  (`BB_MissionDriver_EntityScript.as:59-97`, verified).
- **Two readiness levels when replicated**: server-side `Ready` (discovery is authority-gated —
  a client never reaches it) vs client-reachable `SubordinatesReady` (all client-relevant
  replicated carriers filled, reconciled via rep-notify). Expose separate promises and route UI
  consumers to the client-reachable one (`BB_StoreDriver_Utils.as:152-195`, verified). The
  rep-notify half is owned by ck-game-replication-patterns.
- **Test/harness gating generalizes the same check**: BusterBlock's Gauntlet helper
  `Is_StoreReady()` returns false until the driver entity exists **and** `Get_IsReady()` —
  "an order broadcast before this reaches no listener"
  (`Plugins/BusterBlockTests/Script/Gauntlet/BB_GauntletStoreHelpers.as:36-48`, verified). Give
  every driver-owned flow an equivalent one-line gate.

Drivers with **no** async dependencies should stamp Ready in `DoConstruct` on every machine so
consumers never wait (MissionDriver, `BB_MissionDriver_EntityScript.as:7-9,49`).

## 6. Driver ↔ subordinate topology

Verified shape (StoreDriver → EmployeeManager, `BB_StoreDriver_Hfsm_Tasks.as:1087-1213`):

- **The driver spawns subordinates under its ActorRelay channel entity** (`IP.LifetimeOwner`),
  never under itself when they replicate — lifetime still cascades with the world, replication
  anchors correctly (owner rule: ck-game-replication-patterns).
- **Each subordinate spawn is a small state-machine task** that: resolves the driver from its
  owning SM → checks the `WillSpawn<X>` intent + authority → *peeks* resolved async inputs
  (`ChannelResolved` / `<Dep>Resolved` flags in InitParams) and arms a wakeup signal bind if not
  yet there → spawns with `Request_SpawnEntity(Channel, Class, Params(...))` → on constructed,
  fills the driver's replicated carrier handle and decrements the init counter. The
  peek-then-bind discipline exists because the resolving broadcast may have fired before the
  task entered.
- **Subordinates receive dependencies by injection, not the driver handle.** EmployeeManager's
  Params take a `FCk_Handle_DayCycle` + wage config — not `FCk_Handle_StoreDriver`
  (`Script/ECS/EmployeeManager/BB_EmployeeManager_EntityScript.as:17-42`, verified). No corpus
  subordinate holds a driver backlink (corpus convention — BB-only, not stated in-code);
  consumers reach subordinates *through* the driver's
  accessors (`Driver.Get_EmployeeManager()`), and subordinates stay driver-agnostic (testable by
  composing them directly). If a subordinate needs to notify upward, it exposes a signal the
  driver binds — same direction as every other dependency.
- **Tag conventions**: one feature tag per driver/feature (`TAG_<Prefix><Feature>`), stamped by
  `utils_<feature>::Add` post-composition; plus an opt-in global/instance tag for singleton
  lookup (§3). Subordinates spawned (not discovered) need no discovery tag at all — the driver
  holds their handles in carriers.
- **When a subordinate should itself become a driver**: when it acquires its *own* roster of
  spawned/discovered entities, its own readiness gate, and consumers that want to reach it
  without going through the parent. Corpus example: TrashPickupDriver is a StoreDriver
  subordinate that is itself a driver over trashcans (persistent tag query + delta-gated
  reconcile). The parent then treats it exactly like any subordinate — spawn, inject, carrier.

Console/UI/gameplay requests route **in through the driver**, which forwards to the owning
subordinate — callers never hunt for subordinates themselves. Signals flow **out** from the
driver for scope-level events; feature-level events stay on the feature (Model) entities.

## 7. When NOT to use a driver

| Situation | Use instead |
|---|---|
| A single self-contained world object (door, trashcan, vending machine) | Plain world-object entity script — ck-game-feature-recipe / ck-game-entity-composition-patterns |
| Pure shared data (catalogs, tuning tables) | Provider/definition assets (`asset ... of UCk_...`) — no runtime orchestrator needed |
| Exactly one consumer, no roster, no readiness problem | Compose the feature directly on the consumer; a driver adds indirection for nothing |
| "I need somewhere to put helper functions" | Feature Utils namespace — a driver is not a function bag |

**Anti-pattern: driver-as-god-object.** The smell test — for each piece of logic ask "is this
*policy about the scope* or *mechanism of a feature*?":

| Belongs in the driver (policy) | Belongs in feature Utils / subordinates (mechanism) |
|---|---|
| Which subordinates exist, spawn order, injected config | How the feature composes fragments, its request handling |
| Discovery scope + minimums contract + prune rules | Stamping its own discovery tag (last!) |
| Readiness definition and the Ready stamp | Per-feature `Promise_OnReady` late-binders |
| Routing a request to the owning subordinate | Executing the request |
| Cross-subordinate wiring (bind A's signal to B's input) | The signals and inputs themselves |

If the driver file accretes feature mechanism, extract it: the corpus precedent is Door growing
two responsibilities and being split, with the extracted Entryway feature then *discovered* by
the driver (commits `582bddd26` → `43a14ef5f` → `46e530dbd`).

## Common mistakes

| Mistake | Symptom | Fix |
|---|---|---|
| One-shot construct-time tag scan | Late-composing/async-placed entities never bound | Two-axis discovery: window + re-sweep at close (§4.1) |
| No delta gate on a persistent query handler | Frame cost scales with population every frame (~250ms/frame corpus incident) | `_Added/_Removed` gate after a first-reconcile seed (§4.3) |
| Delta-gating from the very first fire | Initial population never bound (bind-time pass has empty deltas) | The `_HasReconciled` bool (§4.3) |
| Unscoped registry-wide scan on a multi-instance map | Driver claims foreign entities; cross-scope state thrash | Explicit refs → descendant filter → interior-anchored radius prune → per-scope tags (§4.4) |
| Consumer reads the driver in its own `DoConstruct` | Works in gyms, breaks in production boot order; broadcasts reach nobody | Acquire ticket + `Promise_OnReady` (§3, §5) |
| Readiness counter ≠ decrementers | Init hangs forever, or Ready stamps early | Seed-must-match-decrementers invariant; aborts still decrement (§5) |
| Async promises armed before SM/InitParams exist | Inline resolution (listen server) fires into a half-built driver | Arm promises LAST in `DoConstruct` (§5) |
| Replicated driver/subordinate under a non-replicating owner | Silent breakage + `[REP_DEBUG]` flood; tests may still pass | ActorRelay channel owner — ck-game-replication-patterns |
| Global static accessor for the driver | Untestable, multi-instance-hostile; maintainer-named worst debt | Scoped `TryFind_` + Acquire ticket (§3) |
| UI/client consumer waiting on server-only Ready | Client promise never fires | Client-reachable `SubordinatesReady` promise (§5) |

## Provenance and maintenance

- Authored 2026-07-03 against BusterBlock superproject HEAD `52a75e13d` (detached, tracks dev).
  Maintainer MVC endorsement and rulings dated 2026-07-03 (campaign brief; treat as settled).
- All corpus citations verified by direct read on that date: `Script/StoreDriver/BB_StoreDriver_EntityScript.as`
  (full), `BB_StoreDriver_Utils.as:1-210`, `BB_StoreDriver_Hfsm_Tasks.as:1087-1213`,
  `Script/WorldSettings/Gameplay/BB_Gameplay_GameState.as` (full),
  `Script/ECS/MissionDriver/BB_MissionDriver_EntityScript.as` (full),
  `Script/ECS/DayCycle/BB_DayCycle_Utils.as:1-110`,
  `Script/ECS/EmployeeManager/BB_EmployeeManager_EntityScript.as:17-49`,
  `Plugins/BusterBlockTests/Script/Gauntlet/BB_GauntletStoreHelpers.as:36-48`; commits
  `531b0c956` (delta-gating, diff read), `287ee6601` (foreign-door scoping, message read).
- Re-verify volatile claims (Git Bash from the BusterBlock repo root; the repo-root `.ignore`
  hides `/Script` from plain ripgrep — keep `--no-ignore`):

```bash
# Driver census + spawn sites
rg --no-ignore -n "Driver_EntityScript" Script/ -g "*.as" -l
rg --no-ignore -n "Request_SpawnEntity" Script/WorldSettings/Gameplay/BB_Gameplay_GameState.as
# Two-axis discovery + prune + tag-last ensures
rg --no-ignore -n "Prune_ForeignEntryways|tag stamped before the feature finished composing" Script/StoreDriver/
# Delta gate + Acquire tickets
rg --no-ignore -n "_Added.Num\(\) == 0" Script/ECS/ -g "*.as"
rg --no-ignore -n "Acquire(StoreDriver|DayCycle|MissionDriver|EventFeed)" Script/ -g "*.as"
git log --oneline 531b0c956 287ee6601 -n 1
```

- Pattern status: BB-only in corpus, `[PROMOTED FROM CORPUS 2026-07-03 — maintainer-endorsed]`.
  If a second consumer game adopts it, replace BB citations with the strongest exemplar per
  section. The dead `MinExpected*` fields cited in §4.1 are scheduled for deletion in BB — if
  gone on re-verify, keep the lesson, update the line refs.
