# The archetype catalogue

Reference for `ck-game-entity-composition-patterns`: every sanctioned entity archetype with its composition, owner, and the case it exists for.

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

