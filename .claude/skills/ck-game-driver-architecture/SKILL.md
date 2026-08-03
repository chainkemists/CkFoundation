---
name: ck-game-driver-architecture
description: 'Use when designing a CkFoundation game driver or director that owns subordinates, scopes tag discovery, routes requests, and exposes readiness to consumers.'
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


## Reference files — load only what the task needs

Section numbers cited elsewhere in this skill point into these files.

| Topic | Read |
|---|---|
| Reaching a driver, and discovery mechanics | `references/reaching-and-discovery.md` |

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
