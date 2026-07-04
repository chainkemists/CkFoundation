---
name: ck-game-entity-composition-patterns
description: Use when deciding HOW to structure a game entity on CkFoundation — which archetype
  fits (driver, world object, pawn-less NPC, item, behavior micro-entity), who its lifetime owner
  is, where its context root points, whether a feature needs its own child entity, and when an
  Actor bridge is legitimate. Triggers — "should this be an EntityScript or an Actor",
  "Request_OverrideToSelf stole my context", "AtStore never fires / wrong entity resolved from
  overlap", "As_Interactable returns invalid", "my tag query grabbed another building's
  entities", "who should own this spawned entity". For BUILDING games ON CkFoundation. Not for
  the feature-file quartet itself (ck-game-feature-recipe), driver internals
  (ck-game-driver-architecture), or replication mechanics (ck-game-replication-patterns); for
  the framework's own ECS invariants, see ckecs-architecture-contract.
---

# Entity composition patterns — archetypes, ownership, lifetime

The design vocabulary for structuring game entities on CkFoundation: which **archetype** a new
thing should be, how entities are **composed** from framework features, and the
**ownership/lifetime rules** that decide who destroys whom and who answers `ck::Ctx`. Every
pattern here is backed by consumer-corpus evidence (BusterBlock `Bb_`, Venus `Vns_`) with the
incidents that proved the rules. Placeholders: `<Game>` = your project name, `<Prefix>` = your
project prefix (BusterBlock uses `Bb_`/`bb::`, Venus uses `Vns_`/`vns::`).

Jargon used throughout (one-line each; full definitions in `Plugins/CkFoundation/CLAUDE.md`
Lingo table): an **Entity** is an ECS id addressed via a typesafe `FCk_Handle`; a **Fragment**
is a data component on an entity; a **feature** is a reusable capability composed onto an entity
via `utils_<feature>::Add(...)`; an **EntityScript** is a UObject class (usually AngelScript)
whose `DoConstruct` composes an entity — the spawnable/placeable unit of gameplay content;
`ck::Ctx(h)` resolves a handle's **context root** (the "who am I part of" answer);
the **lifetime owner** is the entity whose destruction cascade-destroys yours.

## When NOT to use this skill

| You want to... | Load instead |
|---|---|
| Author the feature files themselves (Feature/Utils/Processor/EntityScript quartet, definition of done) | `ck-game-feature-recipe` |
| Driver internals — init choreography, discovery queries, subordinate wiring, readiness gating | `ck-game-driver-architecture` |
| Replication mechanics — ActorRelay channels, rep-notify bind order, authority routing | `ck-game-replication-patterns` |
| AS idioms, hot-reload loop, when to drop to C++ | `ck-game-angelscript-gameplay` |
| WHY the framework's ECS is shaped this way (fragments, handles, requests, signals) | `ckecs-architecture-contract` |
| EnTT/handle internals, `Request_DestroyEntity` teardown order, TransientEntity mechanics | `ckecs-domain-reference` |
| The in-flight destroy-mid-interaction framework defect cluster | `ck-lifecycle-teardown-campaign` |

---

## 1. The composition mental model

An entity is not a class hierarchy — it is a **composition of framework features** stacked onto
one handle (plus child entities) at construction time:

```angelscript
UFUNCTION(BlueprintOverride)
ECk_EntityScript_ConstructionFlow DoConstruct(FCk_Handle& InHandle)
{
    auto TransformHandle = utils_transform::Add(InHandle, SpawnTransform, ECk_Replication::Replicates);
    utils_<feature>::Add(TransformHandle, Params);   // your Tier-2 gameplay feature
    // + visuals, widgets, more features...
    return ECk_EntityScript_ConstructionFlow::Finished;
}
```

Corpus example (BusterBlock): that is the entire body of
`Script/ECS/Entryway/BB_Entryway_EntityScript.as:26-33` — transform first, feature second, done.
The transform goes on FIRST because most feature `Add`s take the *transform-typed* handle.

Rules of the model:

1. **EntityScripts are the placeable/spawnable unit** — the "spawn vehicle". The gameplay
   feature itself lives in `utils_<feature>::Add` (asset-free, test-composable); the
   EntityScript wraps it with replication stance, visuals, and world bring-up. Corpus example
   (BusterBlock), stated as the contract in `Script/ECS/Trashcan/BB_Trashcan_EntityScript.as:1-6`:
   "tests/headless callers can compose the feature directly … designers/spawners that want a
   placed trashcan with a mesh spawn this entity script instead."
2. **No-Actors doctrine (maintainer-settled, 2026-07-03).** Do NOT author `AActor` subclasses
   for gameplay things. EntityScripts replace ~95% of Actor use. Actors persist only for: the
   player pawn (CMC + possession), physics/Chaos props (bowling pins, fractured wrecks,
   throwable items), and engine-boundary needs (the GameMode/GameState/PC/PS framework chain,
   which bridges INTO ECS — see archetype E). Venus's free Actor usage is old-era; do not copy it.
3. **Construction is a flow, not a constructor.** Return `Finished` when synchronous, or
   `Continue` + call `DoFinishConstruction()` later when downstream `Promise_OnConstructed`
   listeners must only ever see a fully-set-up entity. The feature-arc details (deferral,
   readiness) are owned by `ck-game-feature-recipe`.
4. **Child entities are cheap and idiomatic.** Features routinely spawn their own children
   (triggers, probe nodes, scene nodes for visuals/widgets). A "thing" in the world is usually
   a small tree of entities, not one fat entity. §3 governs who owns and who contextualizes
   that tree.

Why the framework pushes you this way — data-oriented fragments, deferred requests, typed
handles — is `ckecs-architecture-contract`'s story; cite it, don't re-derive it.

---

## 2. The archetype catalogue

Match your new thing to a row BEFORE writing code. Composition-shape column reads as
"what DoConstruct stacks". Replication column is the corpus-normal default, not a law —
`ck-game-replication-patterns` owns the reasoning.

| # | Archetype | When to use | Replication | Corpus exemplar (BusterBlock unless noted) |
|---|---|---|---|---|
| A | World-singleton driver | One-per-world orchestrator (missions, day cycle, event feed) | Replicates (server-spawned) or DoesNotReplicate (client-local visuals) | `Script/ECS/MissionDriver/BB_MissionDriver_EntityScript.as` |
| B | Director + subordinate roster | A driver that discovers placed dependencies and spawns manager children | Replicates | `Script/StoreDriver/BB_StoreDriver_EntityScript.as` |
| C | Placeable world object | Level-placed interactive furniture/props (door, shelf, kiosk) | Replicates | `Script/ECS/Entryway/BB_Entryway_EntityScript.as` |
| D | Pawn-less agent / NPC | Autonomous characters with AI, movement, visuals — no APawn | DoesNotReplicate (simulate-everywhere) — project choice | `Script/Npc/BB_Npc_EntityScript.as` |
| E | Actor-bridged station | A real Actor exists for physics/engine reasons; entity script pairs with it | Actor replicates; entity varies | `Script/ECS/Bowling/BB_BowlingStation_Actor.as` |
| F | Item actor | World-presence inventory items (thrown, dropped, scattered) | Replicates | `Script/Inventory/ItemActors/BB_ItemActor.as` |
| G | Objective / mission entity | Trackable goal state spawned by a driver | Replicates | `Script/Objectives/Tutorial/BB_Objective_OpenStore.as` |
| H | Behavior micro-entity | SM states/tasks, GOAP actions, cues, camera layers — nodes ARE entities | Inherits host context | 173 `UCk_SmTask_EntityScript` + 32 `UCk_GoapAction_EntityScript` subclasses (census) |
| I | Ephemeral helper host | Fire-and-forget per-event workers (damage applicator, deposit orchestrator) | DoesNotReplicate | `Script/ECS/Damage/BB_Damage_Utils.as:126` |
| J | Editor-composable trait entity `[SINGLE-EXEMPLAR]` `[UNDER ADJUDICATION — ADJUDICATIONS.md A5]` | Designer-tunable per-instance capability list on a host entity | Via replication driver | Venus `Script/ECS/WeaponTraits/Vns_Base_WeaponTrait.as` |

### A. World-singleton drivers

One per world; spawned by the authoritative GameState in its ECS construction hook, never
level-placed. Replicated ones spawn under an ActorRelay channel entity (the replicated-spawn
lifetime-owner rule — one line here, `ck-game-replication-patterns` owns it in full); client-local
ones (lamp visuals, day/night presentation) spawn on every machine under `ck::TransientEntity()`.
The driver pattern's INTERNALS — MVC framing, discovery, init choreography, subordinate
spawning, readiness — are `ck-game-driver-architecture`'s subject; go there before building one.
Corpus example (BusterBlock): `ABb_Gameplay_GameState` spawns six drivers under one channel
(`Script/WorldSettings/Gameplay/BB_Gameplay_GameState.as:66-157`); the MissionDriver header
states the anchor rule verbatim (`BB_MissionDriver_EntityScript.as:1-6`, verified 2026-07-03).

### B. Director + subordinate roster

A driver that additionally *discovers placed dependencies by entity tag* and *spawns a roster of
subordinate manager entity scripts* under its own channel. Corpus example (BusterBlock): the
StoreDriver collects entryways/counters/gondolas into a Dependencies fragment, then spawns ~11
subordinates (RentalManager, EmployeeManager, ...). Everything about how — including the two
discovery anti-patterns it survived (§4 rows 1-2) — lives in `ck-game-driver-architecture`.

### C. Placeable world objects — the "spawn vehicle" pattern

The bread-and-butter archetype: `U<Prefix>_<Feature>_EntityScript : UCk_GenericEntityScript_UE`,
`default _Replication = ECk_Replication::Replicates;`, `UPROPERTY(ExposeOnSpawn)` transform +
params, thin `DoConstruct` per §1's snippet. BusterBlock has ~25 of these (Door, Shelf,
Trashcan, DeliveryTruck, CheckoutCounter, ...) — all the same shape.

**Thin asset-variant subclasses**: content variants (9 gondola types, 3 vendors) are subclasses
in a sibling `<Prefix>_<Feature>_Assets.as` file that ONLY set property defaults — no logic.
Corpus example (BusterBlock): `Script/ECS/RetailGondola/BB_RetailGondola_Assets.as:174-428`.
For richer preset/archetype needs, note `[UNDER ADJUDICATION — see CkFoundation
.claude/reports/ADJUDICATIONS.md A4]`: interim stance is EntityScript spawn params + CkProvider.

### D. Pawn-less agents / NPCs

Autonomous characters with **no Actor, no AIController, no Character**. Two-entity topology
(verified 2026-07-03 in `Script/Npc/BB_Npc_EntityScript.as`):

- **Script entity** (context root): AI (planners, SM), inventory, combat — the "brain".
- **Agent child entity**: the crowd-agent feature carrying the MOVING transform
  (`utils_crowd_agent::Add`), plus everything that must follow the body.

**How a pawn-less entity gets visuals + animation** — all on the entity holding the moving
transform, registered under `utils_presentation::Add`:

| Visual need | Mechanism | Corpus cite (BusterBlock) |
|---|---|---|
| Skeletal mesh + AnimBP | `Iskm` renderer + proxy features; shared AnimBP driven off the ECS Velocity fragment (no pawn required) | `BB_Npc_EntityScript.as:443-447` — "Added to the AGENT entity because that's where the moving transform lives" |
| Static mesh instance | ISM proxy feature with per-instance offset | `BB_Trashcan_EntityScript.as:203-220` |
| Bespoke movable component | `utils_unreal_component::Add` from a `NewObject(this, ...)` archetype on a scene-node child (the entity script is the UObject outer) | `BB_Door_EntityScript.as:116-149` |
| World-space UI | world-space-widget feature attached to a scene node | `BB_Trashcan_EntityScript.as:184-199` |

Collision is a **probe-node child** of the agent (capsule pill via
`utils_prefab::Create_ProbeNode_Capsule`). Whether NPCs replicate at all is a project-level
choice — BusterBlock deliberately runs them simulate-everywhere (`DoesNotReplicate`), an open
design question in its tracker, not framework doctrine.

### E. Actor-bridged stations — when bridging IS right

When the thing genuinely needs engine machinery (physics bodies, Chaos destruction, CMC), author
a real Actor AND pair it with an entity script: the Actor spawns its
`U<Prefix>_..._EntityScript` under `ck::TransientEntity()` at BeginPlay. Corpus example
(BusterBlock, verified 2026-07-03): `ABb_BowlingStation : AActor` owns 10 physics pins + ball,
then `utils_entity_script::Request_SpawnEntity(ck::TransientEntity(), UBb_BowlingStation_EntityScript, ...)`
(`Script/ECS/Bowling/BB_BowlingStation_Actor.as:1-12, 88-96`). The framework chain
(GameMode/GameState/PC/PS extending `ACk_*_UE`) bridges the same way. This is the sanctioned
~5% — everything that CAN be a pure entity, should be.

### F. Item actors

`U<Prefix>_ItemActor_EntityScript : UCk_EntityScript_WithActor_UE` paired with a physics Actor —
items need real bodies for throwing/scattering. Items are *passive*: the collector side binds
probe overlap. The item ENTITY lives inside a capacity-1 holder so its runtime state survives
drop/pickup via entity-preserving transfers (corpus: `Script/Inventory/ItemActors/BB_ItemActor.as:19-40`).

### G. Objective / mission entities

Replicated trackable-goal entities spawned by a mission/objective driver against its root owner;
cue subclasses attach presentation per objective. Thin archetype — a driver-subordinate
specialization; corpus: `Script/Objectives/` (10 objective + 22 cue subclasses).

### H. Behavior micro-entities

Every behavior-graph node is itself an entity script composed under its host: SM states and
tasks, GOAP actions ("pure declarations" — side effects live in the SM), cues, camera layers.
You author dozens of these per feature; they inherit the host's context root — which is exactly
why §3 Rule 2 exists. Census (BusterBlock, 2026-07-03): 173 SmTask + 153 SmState + 32 GoapAction
+ 37 GenericCue subclasses.

### I. Ephemeral helper hosts

Per-event throwaway entities spawned under `ck::TransientEntity()` — a damage applicator per
hit, an orchestrator per checkout session. `DoesNotReplicate` always. Know the leak trade-off:
TransientEntity-rooted subtrees persist as fragment-only ghosts until level end — a documented,
accepted cost (corpus: `Script/ECS/CheckoutCounter/BB_CheckoutCounter_DepositOrchestrator.as:104-110`,
verified 2026-07-03, including the inverse warning: parenting such a host under a *replicating*
item entity makes its children inherit replication tracking and fire OutermostActor ensures).

### J. Editor-composable trait entities `[SINGLE-EXEMPLAR — Venus only]` `[UNDER ADJUDICATION — see CkFoundation .claude/reports/ADJUDICATIONS.md A5]`

Venus's WeaponTraits (verified 2026-07-03, `D:\Repos\Venus\Script\ECS\WeaponTraits\Vns_Base_WeaponTrait.as:24-44`):
the host EntityScript exposes `UPROPERTY(EditDefaultsOnly) TArray<UVns_Base_WeaponTrait_ConstructionScript> Traits`
(`Script/Weapons/Vns_Base_Weapon.as:50-51`); each trait becomes a **child entity** wired with three calls —
`utils_entity_extension::Add(HostEntity, InHandle)` + `utils_context_owner::Request_Override(InHandle, HostEntity)`
+ `utils_gameplay_label::Add(InHandle, TraitName)` — plus a back-pointer fragment, discoverable
by label. Designer-composable per-instance capability lists with per-trait enable/disable and
per-trait processors. BusterBlock has **zero** `utils_entity_extension` uses (verified 2026-07-03,
`rg --no-ignore`) — it composes features in code inside `Add()`. Treat trait entities as an
available pattern when designers need per-placement capability tuning, not as the standard.

---

## 3. Ownership & lifetime rules (load-bearing)

Two different questions are answered by two different mechanisms. Conflating them causes the
worst class of composition bug:

| Question | Mechanism | Set by |
|---|---|---|
| "Who destroys me?" | **Lifetime owner** — destruction cascades down the lifetime tree | The owner handle you pass at spawn/`Add` time |
| "Who am I part of?" | **Context root** — what `ck::Ctx(handle)` resolves to | Inherited from spawner; re-rooted by `Request_OverrideToSelf()` / `utils_context_owner::Request_Override` |

### Rule 1 — choose the lifetime owner deliberately, at spawn

The lifetime owner is the cascade-destroy root. Options, in order of preference:

1. **The logical parent entity** (`InHandle` of the composing script) — the default for
   non-replicating children; they die with you, no cleanup code needed.
2. **A replicated anchor** — required for any `_Replication = Replicates` entity-script spawn.
   One line: never TransientEntity, never the player — use the project's replicated anchor
   (ActorRelay channel in BusterBlock; `ck-game-replication-patterns` owns this rule and its
   [REP_DEBUG]-flood failure signature).
3. **`ck::TransientEntity()`** — world-scoped owner for **non-replicating** fire-and-forget
   spawns only (archetypes E, I). Accept the fragment-ghost leak (§2-I); it null-derefs without
   a resolvable world (hot-reload/editor contexts).

### Rule 2 — context root ≠ lifetime owner

`Request_OverrideToSelf()` re-roots `ck::Ctx` resolution ONLY; the lifetime tree is untouched.
Corpus proof (BusterBlock, verified 2026-07-03, `Script/Npc/BB_Npc_EntityScript.as:391-396`):

```angelscript
// Claim ourselves as context root before creating children: SM
// states/conditions/tasks resolve their subject via ck::Ctx, which would
// otherwise inherit the spawner (e.g. a gym station) that lacks our AI
// fragments. Lifetime owner is unchanged, so we still cascade-destroy.
InHandle.Request_OverrideToSelf();
```

Do this **before creating children** on any entity that hosts behavior micro-entities
(archetype H) — otherwise every SM task asks `ck::Ctx` and gets your SPAWNER (a gym station, a
driver) instead of you.

### Rule 3 — self-overriding features go on their own child entity

Some features call `Request_OverrideToSelf()` internally on whatever handle you give them
(Interactable does — verified at `Script/ECS/Interactable/BB_Interactable_Utils.as:42`; so does
BusterBlock's FlyerRecipient). Composing such a feature directly onto a shared entity **re-roots
that entity's context** and silently breaks every other ck::Ctx consumer on it.

**The incident that proved it** (verified 2026-07-03, `BB_Npc_EntityScript.as:566-576`): the
FlyerRecipient was once added to the NPC's agent entity; its internal override made
`ck::Ctx(collision pill)` resolve to the agent instead of the NPC — store occupancy then
reported the wrong entity and "AtStore never fires (tourists pile up at the door)". The in-code
rule now reads: "MUST be a child node, never the agent itself." Fix shape:

```angelscript
auto FeatureNode = utils_scene_node::Create(HostTransform, FTransform::Identity);
_Feature = utils_<feature>::Add(FeatureNode.As_Transform(), FeatureParams);
// expose the child handle to consumers via a fragment on the host
```

### Rule 4 — interactables live on child probe-node entities; resolve owners back via ck::Ctx

The corpus interactable pattern (verified 2026-07-03,
`Script/ECS/Interactable/BB_Interactable_Utils.as:12-60`): `Create` builds the interactable on a
**child probe node** (spatial focus) or plain scene node (non-spatial), stamps the feature +
state there, calls `Request_OverrideToSelf()`, and adds per-channel InteractTarget children.
Consequences for consumers:

- `As_Interactable()` on the OWNER handle fails — the feature isn't on the owner. Use the
  feature's accessor that returns the child (corpus:
  `mixin FCk_Handle_Interactable Get_Interactable(...)`, `BB_CheckoutCounter_Utils.as:264`).
- Overlap payloads hand you the **probe child**; resolve the owning character/entity via
  context: `auto Owner = ck::Ctx(OtherEntity);` (corpus:
  `Script/ECS/CollectiblePickup/BB_CollectiblePickup_Processor_Setup.as:55-58` — "The
  overlapping pill is a child probe node; resolve to the owning character entity").

Generalize: any feature whose focus/collision is spatial gets a probe-node child; any consumer
of overlap events must expect children and resolve up.

### Rule 5 — teardown: unbind what you bound, release what you joined, destroy what you created standalone

Cascade-destroy handles your lifetime-owned children. Everything OUTSIDE the lifetime tree is
yours to clean in `DoEndPlay`:

1. **Release cross-entity memberships** (queues, wait-lines, rosters) so they don't hold
   dead-entity slots.
2. **Unbind cross-entity signals** you bound (a singleton's time signal, a driver's event) so
   the binding doesn't fire on a dead script.
3. **`Request_DestroyEntity` on children you created under someone ELSE'S owner** (standalone
   scene nodes, widgets) — then reset the handle you stored.

Corpus example doing all three (verified 2026-07-03,
`Script/Npc/BB_Npc_EntityScript.as:856-907`): queue `Request_Leave`, `UnbindFrom_OnTimeChanged`
on the DayCycle, destroy the info-card widget + its node, each guarded by `ck::IsValid` with
handle-reset-after-destroy.

**Live landmine**: destroying an entity mid-interaction is a known framework defect cluster —
`OnInteractionFinished` may never fire and interaction entities leak. Before shipping any
destroy-during-gameplay path, read `ck-lifecycle-teardown-campaign` (the defect map) and
`ckecs-domain-reference` (the `Request_DestroyEntity` teardown pipeline order).

---

## 4. Anti-patterns — with the incidents that proved them

Maintainer ruling (2026-07-03): the two worst debt classes in consumer code are **anything
requiring global access** and **band-aid bootstraps**. Rows 1 and 4 are those, respectively.

| # | Anti-pattern | Incident evidence (verified unless marked) | Correct pattern |
|---|---|---|---|
| 1 | **Registry-wide tag discovery grabs foreign entities.** A global `TAG_<X>` scan on a map with two+ instances of your context claims the other one's entities. | BusterBlock commit `287ee6601` "fix(storedriver): scope door discovery to the store footprint"; in-code post-mortem at `BB_StoreDriver_EntityScript.as:621-657` — foreign entryways "both lure shoppers to the wrong building and thrash this store's occupancy"; fixed by interior-anchored radius pruning. | **Scope discovery to your context**: explicit dependency refs > per-scope tags > radius from an interior anchor. Never leave a raw global scan in shipping code. |
| 2 | **Discovery tag stamped before composition finishes.** Consumers race and see half-built entities. | Every StoreDriver binder carries the ensure: "tag stamped before the feature finished composing?" (`BB_StoreDriver_EntityScript.as:856-858`). | Stamp the discovery tag LAST inside `utils_<feature>::Add`, after all fragments/children exist. This is the #1 lesser-model failure mode — `ck-game-feature-recipe` owns the canonical warning. |
| 3 | **Self-overriding feature composed on a shared entity.** | The FlyerRecipient-on-agent bug — §3 Rule 3, `BB_Npc_EntityScript.as:566-576`. | Own scene-node child per such feature. |
| 4 | **WS-dirty bootstrap band-aid.** GOAP planners replan only on WorldState-dirty; seeding fake WS writes at spawn so the first plan happens is a band-aid, not a pattern. | `BB_Npc_EntityScript.as:925-934`: "planners only replan on WS-dirty — a tourist with no store … would otherwise park in Idle_StandWatch forever", hence a `WantsToRoam=true` bootstrap, with an `INTERIM`-marked sibling seed right below it. | Name it **DEBT** and backlog it. If you must bootstrap, mark the write `INTERIM` with the real observer named, exactly as the corpus does. Never teach it forward. |
| 5 | **EntityTagQuery All-mode consumed without delta-gating.** The query re-delivers the FULL match set every evaluate pass, by design. | Fix commit `531b0c956` "perf(ecs): gate driver discovery handlers on population delta" (verified in git log 2026-07-03). Frame-cost magnitude (~250ms/frame full-population rebuild) is `[INFERRED — session-memory only, not re-measured]`. | Consume `Result.Get_Added()` / `Get_Removed()` deltas (corpus: `Script/Objectives/Tutorial/BB_Objective_RewindRental.as:71-75`), or dedupe against your own bound-set fragment. |
| 6 | **Arming async promises before the entity is ready for an INLINE resolution.** Promises can resolve synchronously (dependency already ready); a handler that touches not-yet-written state hides in tests and breaks in production ordering. | `BB_StoreDriver_EntityScript.as:398-421` — "Arm the async inputs LAST (ck-promise-inline-resolution discipline)" + an `INVARIANT — DO NOT change` block on the deferred-SM-start hang mode; `BB_Npc_EntityScript.as:397-401` — cache `_CachedSelfEntity` BEFORE arming a promise that "fires … SYNCHRONOUSLY when the driver is already ready". | In DoConstruct: state → SM/dispatch → cached handles → arm promises LAST. Assume every promise can fire inline, this frame. |
| 7 | **Expecting an enter event when spawning inside a trigger volume.** | `[INFERRED — session-memory autotest gotcha; no in-code cite found in the 2026-07-03 sweep. Confirm against CkSpatial/Trigger sources before relying on it.]` | After spawning inside a volume, query current overlap state explicitly rather than waiting for OnEntityEntered. |
| 8 | **Replicated entity script spawned under a non-replicating owner.** | One line: [REP_DEBUG] flood, silently-broken tests — `ck-game-replication-patterns` owns the rule, evidence, and fix. | Spawn under the project's replicated anchor. |

---

## 5. Pre-spawn decision checklist

Answer these nine questions before writing `DoConstruct` for anything new:

1. **Which archetype?** (§2 table.) If none fits, you are probably building two things — split.
2. **Entity or Actor?** Pure entity unless physics/CMC/engine-boundary forces a bridge (§1
   rule 2, §2-E).
3. **Does it replicate?** Decide now; it constrains the next answer. →
   `ck-game-replication-patterns`.
4. **Who is the lifetime owner?** Logical parent / replicated anchor / TransientEntity — §3
   Rule 1. Chosen at spawn, hard to change later.
5. **Does it host behavior micro-entities (SM/GOAP/cues)?** Then `Request_OverrideToSelf()`
   before creating children — §3 Rule 2.
6. **Do any composed features self-override context (Interactable-like)?** Each gets its own
   scene-node/probe-node child — §3 Rules 3-4.
7. **When is the discovery tag stamped?** LAST, after composition — §4 row 2; canonical warning
   in `ck-game-feature-recipe`.
8. **How does it get visuals?** Iskm (skeletal) / ISM proxy (static) / component-on-scene-node /
   world-space widget — §2-D table. On the entity holding the MOVING transform.
9. **What must DoEndPlay undo?** Memberships, cross-entity binds, standalone children — §3
   Rule 5. Write the teardown list while you write the construct list.

---

## Common mistakes

- Reading `ck::Ctx` in a behavior node and assuming it is "my NPC" when the host never
  self-overrode — you get the gym station / driver that spawned it (§3 Rule 2).
- Calling `As_Interactable()` (or any self-overriding feature's cast) on the OWNER handle and
  treating the invalid result as "feature missing" — the feature is on a child; use the
  `Get_<Feature>()` accessor (§3 Rule 4).
- Toggling an interactable's target but not its PROBE when disabling — they are the same child
  entity; disable at the probe (corpus: `Script/ECS/Fixture/BB_Fixture_Utils.as:303-327`).
- Copying Venus's Actor-tick processors or free Actor placeables as "the other valid style" —
  old-era, ruled out (§1 rule 2).
- Putting visuals/asset refs in the feature's `utils_::Add` instead of the EntityScript —
  breaks asset-free test composition (§1 rule 1).
- Storing a child handle, destroying the child, and not resetting the handle — later
  `ck::IsValid` guards pass on a stale handle (§3 Rule 5 corpus does reset-after-destroy).

---

## Provenance and maintenance

Authored 2026-07-03 against BusterBlock superproject HEAD `52a75e13d` (detached, tracks dev) and
Venus `feature/state-machine` (CkFoundation pinned 2026-03-25). Primary research:
phase-0 discovery reports (entity-composition, venus-recon, feature-lifecycle); every
load-bearing citation above re-verified by reading the cited lines on 2026-07-03. Class-census
counts (173/153/64/37/32/14 subclasses) and the GameState driver-spawn line range are taken from
the phase-0 census on trust; rows 5 (frame-cost magnitude) and 7 (born-inside-trigger) are
explicitly `[INFERRED]`.

Re-verify volatile claims (Git Bash, from the BusterBlock repo root — note the repo-root
`.ignore` hides `Script/` from plain ripgrep, hence `--no-ignore`):

```bash
# Context-override + FlyerRecipient incident + WS bootstrap + DoEndPlay teardown
rg --no-ignore -n "Request_OverrideToSelf|MUST be a child node|park in Idle_StandWatch" Script/Npc/BB_Npc_EntityScript.as
# Probe-node interactable topology
rg --no-ignore -n "Create_ProbeNode|Request_OverrideToSelf" Script/ECS/Interactable/BB_Interactable_Utils.as
# Foreign-discovery prune + tag-stamp ensure
rg --no-ignore -n "Prune_ForeignEntryways|tag stamped before the feature finished composing" Script/StoreDriver/BB_StoreDriver_EntityScript.as
# Incident commits
git log --oneline -1 287ee6601 531b0c956
# Venus trait entities (single exemplar) + BB's absence of the pattern
rg --no-ignore -n "utils_entity_extension" /d/Repos/Venus/Script Script
```

If a re-verify fails, the corpus moved — update the citation or demote the claim to
`[INFERRED]` before teaching it.
