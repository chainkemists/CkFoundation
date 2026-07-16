---
name: ck-game-replication-patterns
description: 'Use when making CkFoundation gameplay multiplayer-safe: replication stance, ActorRelay ownership, rep-notify ordering, authority routing, or client readiness.'
---

# Replication patterns for CkFoundation games

Multiplayer misuse is the single largest post-ship incident category in the reference corpus
(BusterBlock, ~7000 commits): features that work perfectly in single-player PIE and on the
listen-server host, then break on a real client. This skill owns the consumer-side patterns
that prevent those incidents: which replication stance each entity archetype takes, the
ActorRelay-channel lifetime-owner rule for replicated spawns, rep-notify bind ordering, the
server/client authority split, and readiness gating.

Jargon used here: an **entity script** is the AngelScript class (subclass of
`UCk_GenericEntityScript_UE`) that composes an entity — the placeable/spawnable unit of a Ck
game. A **fragment** is an ECS data component. A **lifetime owner** is the entity a spawn is
parented under for cascade-destroy. A **cue** is a fire-and-forget replicated gameplay event
(`UCk_GenericCue_EntityScript`). **Authority** = the machine UE considers the owner of state,
i.e. the server. See the Lingo table in `Plugins/CkFoundation/CLAUDE.md` for the rest.

All corpus citations verified 2026-07-03 against BusterBlock dev tip `52a75e13d`.

## When NOT to use this skill

| You want to... | Load instead |
|---|---|
| Entity archetypes, lifetime/ownership rules in general, trait composition | `ck-game-entity-composition-patterns` |
| How replicated fragments Apply on clients (NotReady, timeouts, OnReplicationComplete internals) | `ckecs-architecture-contract` §7 |
| Driver MVC pattern, discovery, driver↔subordinate topology | `ck-game-driver-architecture` |
| Authoring/running net autotests, test mechanics | `ck-game-testing-discipline` |
| Iris/engine net config, project ini wiring | `ck-game-project-bootstrap` |
| PIE-vs-packaged divergence triage beyond the net axis | `ck-game-debugging-playbook`, `ck-debugging-playbook` §6 |
| Why the signal/replication machinery is shaped this way | `ckecs-architecture-contract` |

---

## 1. Mental model — what replicates in Ck-land

Three things carry state across the network in a CkFoundation game:

1. **Replicated entity scripts.** `default _Replication = ECk_Replication::Replicates;` on the
   class. The server spawns it; the entity (and its replicated composition) arrives on every
   client, where `DoConstruct` runs again locally. This is the unit of "this thing exists on
   all machines".
2. **Replicated fragments/features.** Most framework feature composers take an
   `ECk_Replication` argument (`utils_float_attribute::Add(Handle, Tag, Value,
   ECk_Replication::Replicates, ...)`). Replicated values ride fragment containers that are
   applied on clients by a dedicated processor — **after** construction. The framework contract
   (Apply/NotReady, the 5s/2s pending-apply timeout, and the rule *OnConstructed = composed,
   NOT values-applied — read replicated values only from `Promise_OnReplicationComplete`*) is
   owned by `ckecs-architecture-contract` §7 "Replication — the deferred-Apply contract". Cite
   it; do not re-derive it.
3. **Cues** — reliable/unreliable one-shot events with a multicast policy, used for
   client→server and server→clients routing (§4).

And one deliberate **non**-replication stance:

4. **Simulate-everywhere.** `DoesNotReplicate` + the same code running on every machine,
   converging because its inputs are replicated (or deterministic enough). Corpus example
   (BusterBlock): the entire NPC stack — `UBb_Npc_EntityScript` declares
   `default _Replication = ECk_Replication::DoesNotReplicate;`
   (`Script/Npc/BB_Npc_EntityScript.as:286`) and its combat receiver is composed
   `DoesNotReplicate` explicitly "to match the NPC's simulate-everywhere model (the SM that
   reads IsDowned also runs on every machine)" (`:633-634`), while the AI decision tier is
   additionally authority-gated (`if (utils_net::Get_HasAuthority(InHandle)) { AddNpcAI(...); }`,
   `:625-628`).

   **When simulate-everywhere is right:** high-population agents (BusterBlock budgets 120+
   NPCs) where per-agent replication cost is unjustifiable, no hard consistency requirement
   exists on the agent's exact state, and the inputs that matter (world clock, store state,
   damage events) are themselves replicated or routed through cues. **When it is wrong:**
   anything a player transacts with directly where divergence is visible and consequential —
   that state belongs in a replicated attribute written only by the authority (§4). Note the
   stance is under active designer review in the corpus (open question "should NPCs
   replicate"), so treat it as a considered trade-off, not free.

Decision heuristic before any of the machinery below: **does a remote client need to see or
interact with this exact entity's state?** Yes → Replicates (and everything in §2–§4 applies).
No, server bookkeeping only → DoesNotReplicate, server-only spawn. No, per-client
presentation → DoesNotReplicate, spawned on every machine (§6).

---

## 2. THE RULE — a replicated spawn needs a net-correlated lifetime owner

> **Any entity script with `_Replication = Replicates` must be spawned under a lifetime owner
> that is itself replicated. Acquire an ActorRelay channel and use its channel entity as the
> lifetime owner. Never `ck::TransientEntity()`, never a non-replicating entity.**

Why: entity replication ultimately rides a UE actor channel. An **ActorRelay channel** is the
framework's answer to "who owns the replicated thing" — an explicit channel entity owned by a
replicated actor (see `ck-failure-archaeology` §7 "Cue subsystem", whose LESSON names ActorRelay
as the house answer; new replicated-ownership designs start there). A replicated entity script
parented under a non-replicating owner has no path to clients, and the framework guards this
loudly:

- The spawn trips the `Get_Replication(ReplicatedOwner) == Replicates` ensure and floods the
  log with `[REP_DEBUG]` lines. Corpus incidents (verified via `git show`):
  - `de3099e1c` — an autotest spawned a `Replicates` checkout counter under the non-replicating
    test entity, "tripping the Get_Replication ensure. Spawn under an ActorRelay.Generic
    channel entity instead." The test *may even pass* while silently broken — this incident is
    the origin of the rule.
  - `98e206e97` — the same trap in a gym: replicating shelves spawned under a non-replicating
    station; fixed with the identical channel acquire.
- Production comments carry the rule in-source: "Spawned by the Gameplay GameState under an
  ActorRelay channel (the correct replicated-spawn anchor — `ck::TransientEntity()` is
  non-replicating and would flood `[REP_DEBUG]`)"
  (`Script/ECS/MissionDriver/BB_MissionDriver_EntityScript.as:4-6`).

### The canonical acquire → promise → spawn pattern

Generalized from `Script/WorldSettings/Gameplay/BB_Gameplay_GameState.as:69-135` (verified),
which spawns all of BusterBlock's world-singleton drivers this way. House style per
`Plugins/CkFoundation/Script/CLAUDE.md`:

```angelscript
class A<Prefix>_Gameplay_GameState : A<Prefix>_Master_GameState
{
    private FCk_Handle                     _GameStateEntity;
    private FCk_Handle_PendingActorRelay   _PendingChannel;
    private FCk_Handle                     _ChannelLifetimeOwner;
    private FCk_Handle_PendingEntityScript _PendingMyDriver;

    UFUNCTION(BlueprintOverride)
    void EcsConstructionScript(FCk_Handle InEntity)
    {
        auto _CkPerfScope = ck::ScopedStat();
        _GameStateEntity = InEntity;

        // Replicated spawns are SERVER-ONLY; clients receive the replicated
        // entities and discover them by tag or Acquire ticket (see §5).
        if (System::IsServer() == false)
        { return; }

        _PendingChannel = utils_actor_relay::Request_AcquireChannel(GameplayTags::ActorRelay_Generic);
        _PendingChannel.Promise_OnAcquired(
            FCk_Delegate_ActorRelay_Acquired(this, n"OnChannelAcquired"));
    }

    UFUNCTION()
    private void OnChannelAcquired(FCk_ActorRelay_ChannelResult InResult)
    {
        auto _CkPerfScope = ck::ScopedStat();
        auto LifetimeOwner = InResult.Get_ChannelEntity();
        _PendingChannel = FCk_Handle_PendingActorRelay();

        if (ck::Is_NOT_Valid(LifetimeOwner))
        {
            ck::Warning("[<Prefix>_Gameplay_GameState] ActorRelay channel acquisition returned an invalid lifetime owner");
            return;
        }

        // Cache it — later/injected spawns reuse the same replicated anchor.
        _ChannelLifetimeOwner = LifetimeOwner;

        _PendingMyDriver = utils_entity_script::Request_SpawnEntity(
            LifetimeOwner, U<Prefix>_MyDriver_EntityScript,
            U<Prefix>_MyDriver_EntityScript::Params());
        utils_pending_entity_script::Promise_OnConstructed(
            _PendingMyDriver,
            FCk_Delegate_EntityScript_Constructed(this, n"OnMyDriverConstructed"));
    }
}
```

Notes on the pattern (all confirmed at the cited file):

- **The promise can be the whole choreography.** The corpus GameState spawns six drivers from
  one `OnChannelAcquired`, and chains dependent spawns off `Promise_OnConstructed` (the kiosk
  driver is spawned only after the DayCycle constructs, so a valid clock handle can be
  *injected* — dependency injection over re-discovery, `BB_Gameplay_GameState.as:117-121,
  183-201`).
- **Non-replicating siblings may share the channel.** Server-only `DoesNotReplicate` drivers
  are spawned under the same `LifetimeOwner` for uniform cascade-destroy — harmless, since the
  guard only fires the other way around.
- **A driver that spawns replicated subordinates acquires its own channel.** Corpus example:
  the level-placed StoreDriver acquires a channel at construction
  (`Script/StoreDriver/BB_StoreDriver_EntityScript.as:404`) and anchors ~11 subordinate entity
  scripts plus runtime truck spawns to it. See `ck-game-driver-architecture`.
- **Inside tests** the same rule holds — the autotest entity does not replicate, so `Replicates`
  spawns in a test body also go under an acquired channel. Mechanics and the older
  `_NonReplicating`-subclass workaround you may still see: `ck-game-testing-discipline`.

Historical corroboration that the *need* is generic even where the mechanism differs: the
second corpus project (Venus, old-era) had no ActorRelay and solved the identical problem with
a dedicated always-relevant replicated anchor actor (`AVns_EcsReplicatedAnchor`) exposed as
`ck::TransientEntity_Replicated()` — same invariant ("replicated spawns need a replicated
anchor"), actor-based fix that predates the channel API. Do not copy the Venus mechanism; use
ActorRelay.

`ck::TransientEntity()` remains the correct world-scoped owner for **non-replicating** spawns —
corpus examples: client-local drivers ("the driver is NON-replicated and client-derived, so it
spawns on EVERY machine (no server gate) under `ck::TransientEntity()` — no ActorRelay channel
(that is only for replicated entity scripts)",
`Script/ECS/DayNightLampDriver/BB_DayNightLampDriver_Subsystem.as:4-7`), per-hit helper
entities, and the simulate-everywhere NPCs.

---

## 3. Bind before broadcast — the rep-notify one-shot

On a client, replicated carrier fragments' initial values are applied by the framework's
dispatch processor, which broadcasts their **one-shot** `OnRepNotify` at apply time. If your
client-side setup code binds that notify *after* the apply already ran, the values sit there
pre-filled, nothing changes again, and the notify never re-fires. Symptom: **blank panels /
default values on MP clients, server and listen-host fine.**

Corpus incident (verified): `d6f9785a4` "fix(storedriver): consume replicated carriers after
rep-notify bind on MP clients" — StoreDriver subordinate panels were blank on clients because
`FProcessor_ReplicatedFragments_Dispatch` applied and broadcast before the client's
`NeedsSetup` processor bound `BindTo_OnRepNotify`. Two valid fixes; the corpus uses (a):

- **(a) Bind, then reconcile once immediately** — after binding, run your reconcile function
  once to consume already-applied values; the notify handles subsequent updates. The shipped
  fix (`Script/StoreDriver/BB_StoreDriver_Processor_Setup.as:43-52`) does exactly this, with
  the why in a comment ending "replicated container values are applied AFTER OnConstructed".
- **(b) Bind with a replay-capable policy.** Ck signal binds take a binding policy; verbatim
  names from the root doctrine (`Plugins/CkFoundation/CLAUDE.md`, "Signals" — quoted verbatim
  because a past doc typo'd them): `ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame`
  (replay same-frame payload), `FireIfPayloadInFlight` (replay last payload from any frame),
  `IgnorePayloadInFlight` (future fires only). If the bind surface you're using exposes the
  policy, `FireIfPayloadInFlight` makes a late bind safe. Semantics, replay limits ("only the
  last payload replays"), and the unbind interaction: `ckecs-architecture-contract` §5.

The general law for any client-side one-shot (rep-notify, `Promise_OnConstructed`,
`OnReplicationComplete`): **either bind before the broadcast can happen, or use a
replay-capable bind, or reconcile manually after binding.** Never assume "it will fire
eventually" — on a client that joined with state already in flight, it already fired.

---

## 4. Authority — what runs where, and how a client asks for a change

### 4.1 Server-only logic is gated, not assumed

Anything that *decides* runs on the authority. Two corpus forms (verified):

- **Server-only spawn**: `if (System::IsServer() == false) { return; }` before spawning drivers
  (`BB_Gameplay_GameState.as:74-75`). Server-authoritative drivers with no client mirror
  (corpus: CollectiblesDriver, ItemOverflowDriver) are `DoesNotReplicate` + server-only spawn —
  clients never even have the entity.
- **Authority gate inside shared code**: `utils_net::Get_HasAuthority(Handle)` — the NPC's AI
  tier (`BB_Npc_EntityScript.as:625-628`) and every cue body that writes state (below).

### 4.2 Client → server requests route through cues

Framework `Request_*` mutations on replicated state are authority-gated and **silently dropped
when called without authority** — a client-side call no-ops with no error. Symptom: a button /
interaction works on the listen server, does nothing on a remote client.

The established corpus pattern (two verified incidents that converged on it):

- `a421b2ce2` "fix(OpenSign): client→server toggle cue for the replicated IsOpen attribute" —
  the interaction SM ran on the initiating machine and wrote the replicated attribute locally:
  client toggles no-op'd, and a locally-diverged client later *fought* server values. Fix
  established the pattern: the initiating machine fires a **Reliable cue owned by the
  replicated entity**; the cue body **authority-gates and only the server writes**; the
  attribute's own replication converges every client.
- `e140915f7` "fix: Route LootAll button through server cue for client authority" — a widget
  click handler called an authority-gated mass transfer directly; remote clients silently
  no-op'd. Same fix shape.

The pattern, generalized (fire side from `Script/ECS/OpenSign/BB_OpenSign_Hfsm.as:57-62`, body
from `BB_OpenSign_Cues.as:41-57` — both verified):

```angelscript
// Initiating machine (any): fire the cue AT the replicated entity.
utils_cue_generic::Request_ExecuteCue(
    ReplicatedOwnerEntity,
    utils_gameplay_tag::ResolveGameplayTag(n"Cue.<Game>.<Feature>.<Action>"),
    U<Prefix>_Cue_<Feature>_<Action>::Params(DesiredState),
    ECk_Cue_ReliabilityPolicy::Reliable,
    ECk_Cue_MulticastPolicy::ServerAndAllClients);

// Cue body: everyone may run it; ONLY the authority writes.
UFUNCTION(BlueprintOverride)
void DoBeginPlay(FCk_Handle InHandle)
{
    auto _CkPerfScope = ck::ScopedStat();
    const auto Owner = ck::OwnerEntity(InHandle);
    if (utils_net::Get_HasAuthority(Owner) == false)
    { return; }
    // ... the single authoritative write; replication converges the clients.
}
```

Design details that made the pattern robust (from the `a421b2ce2` commit body, verified):
carry the **desired state**, not "toggle", so double-fires converge instead of double-flipping;
make the durable state a replicated attribute and keep the cue a one-shot event
(`_LifetimeBehavior = AfterOneFrame`); handles ride the RPC and re-resolve server-side (safe
for already-replicated entities — no spawn race). A `ServerOnly` multicast policy is the
variant when only the server needs to run the body at all (`Script/Cues/BB_Cue_InventoryLootAll.as`).

### 4.3 Never finish construction lazily on a replicated entity

`FProcessor_EntityScript_Replicate` gates on the construction-finish tag. If your entity script
returns `Continue` from `DoConstruct` and finishes only after a long async cascade, it can
finish **after** the Replicate processor's window — and then it never replicates to clients at
all. Corpus incident (verified): `1b0665a3f` "fix(storedriver): finish driver construction
eagerly so it replicates to clients" — an added async subordinate pushed the finish past the
window; clients got blank panels because the driver entity itself never arrived. Fix: **return
`Finished` from `DoConstruct` on the server path; track subordinate/async readiness separately
via your own Ready tag/signal** (§5). Rule of thumb: deferred construction-finish is for
"downstream `Promise_OnConstructed` consumers must see a fully-set-up entity", never for "wait
for my whole async world" — on a `Replicates` script the latter is a replication outage.

---

## 5. Readiness gating — net entities arrive late

On a client, replicated entities arrive whenever the net driver delivers them: after level
load, after your GameState's construction, in arbitrary order relative to each other, and their
replicated *values* arrive after the entities themselves (§1/§3). Gameplay code that reads a
net-delivered entity at its own construct time is a race you will lose on real clients.

Consumer patterns (all corpus-verified):

1. **Per-feature Acquire tickets with `Promise_OnReady`.** A singleton feature exposes
   `utils_<feature>::Acquire<Feature>(...)` returning a pending handle whose
   `Promise_OnReady(Delegate)` resolves whether the singleton already exists or shows up later
   (`Script/ECS/DayCycle/BB_DayCycle_Utils.as:72-98`; same shape for
   `AcquireMissionDriver`, `AcquireEventFeed`). This is the polite form of discovery — no
   polling, no ordering assumption. Teach this shape for any world-singleton your game adds.
2. **An explicit Ready predicate for compound readiness.** When "the entity exists" is not
   enough — discovery finished, subordinates bound, listeners live — expose a single accurate
   gate. Corpus example (BusterBlock Gauntlet helpers,
   `Plugins/BusterBlockTests/Script/Gauntlet/BB_GauntletStoreHelpers.as:36-48`):
   `Is_StoreReady()` returns false until the driver entity exists **and** has stamped its Ready
   tag — the in-code comment is the lesson: "An order broadcast before this reaches no
   listener." Generalize: any request/broadcast aimed at a driver must be gated on the driver's
   Ready contract, not on the entity handle being valid. Driver-side Ready mechanics (readiness
   counters, Ready-tag stamping) belong to `ck-game-driver-architecture`.
3. **Read replicated values only after `Promise_OnReplicationComplete`** — the framework-level
   gate for "the values are actually applied on this client"
   (`ckecs-architecture-contract` §7).

One-liner on environments: single-player PIE and listen-server PIE hide almost every bug in
this skill — timing collapses and the local machine has authority. Anything replication-touching
must be exercised with a real client (multi-PIE net autotest or packaged client) before it is
called done; for the triage of "works in PIE, broken packaged/net", see
`ck-game-debugging-playbook` and `ck-debugging-playbook` §6.

---

## 6. Replication decision table by archetype

Archetype definitions and composition mechanics live in
`ck-game-entity-composition-patterns`; this table owns only the replication column. Corpus
stances (BusterBlock, verified via `default _Replication` census):

| Archetype | Stance | Lifetime owner | Why |
|---|---|---|---|
| World-singleton driver with client-visible state (missions, world clock, event feed) | `Replicates` | ActorRelay channel entity, GameState-acquired, server-only spawn (§2) | Clients need the entity + its replicated carriers; discover via Acquire ticket |
| Server-bookkeeping driver (eviction caps, collectible registry) | `DoesNotReplicate` | Same channel or server-side owner — server-only spawn | No client mirror needed; cheapest correct stance |
| Client-local/presentation driver (lighting, ambient visuals) | `DoesNotReplicate` | `ck::TransientEntity()`, spawned on EVERY machine (no server gate) | Derived from replicated inputs; each client renders its own |
| Placed world object / furniture (doors, shelves, signs, counters) | `Replicates` | Level placement; runtime spawns under an ActorRelay channel (§2) | Shared interactable state; players on all machines transact with it |
| NPC / high-count agent | `DoesNotReplicate` (simulate-everywhere), AI authority-gated | `ck::TransientEntity()` / population owner | Per-agent replication cost at 120+ agents; inputs replicated instead (§1.4 — stance under designer review in the corpus) |
| Objective / mission entities | `Replicates` | The (channel-anchored) driver's root | Clients render objective state; parent is already net-correlated |
| UI, widgets, cues, per-hit helpers | Not replicated (cues carry their own RPC) | Owning entity / `ck::TransientEntity()` | UMG is per-client; cues are the transport, not the state |

The one universal invariant across every row: **`Replicates` ⇒ net-correlated lifetime owner;
`DoesNotReplicate` ⇒ decide deliberately whether it spawns server-only or on every machine** —
that second choice is itself a stance, and mixing them up gives you either ghosts on the server
or missing visuals on clients.

---

## 7. Testing multiplayer behavior

Multi-PIE **net autotests** exist and are the cheapest real-client gate — the corpus carries
4 (`CkAutoTest_Net_*`: OpenSign client-toggle round-trip and rapid-server-toggle convergence
landed in the same commit as the authority fix `a421b2ce2`, plus ChangeablePoster and Vendor
replication tests). Inside any autotest, the §2 rule applies verbatim: a `Replicates` spawn
goes under an acquired ActorRelay channel, never the test entity. Authoring mechanics, the net
C++-stub rebuild trap, and the hard 30s net-test budget: `ck-game-testing-discipline` (which
owns test mechanics) and `ck-tests-authoring-and-running` §2b.

---

## Common mistakes

1. **Spawning a `Replicates` entity script under `ck::TransientEntity()` or any non-replicating
   owner.** `[REP_DEBUG]` flood + `Get_Replication` ensure; in tests it can pass while silently
   broken (`de3099e1c`, `98e206e97`). Fix: §2 acquire→promise→spawn.
2. **Binding a client-side rep-notify after the one-shot initial dispatch.** Blank client
   panels (`d6f9785a4`). Fix: bind then reconcile once, or a `FireIfPayloadInFlight`-family
   policy (§3).
3. **Writing replicated state from the initiating client.** Local write silently dropped or —
   worse — locally diverged and fighting later server values (`a421b2ce2`, `e140915f7`). Fix:
   Reliable cue at the replicated owner, authority-gated body (§4.2).
4. **Returning `Continue` from `DoConstruct` on a replicated script and finishing after an
   async cascade.** The entity misses the Replicate window and never reaches clients
   (`1b0665a3f`). Fix: `Finished` eagerly on the server path; separate Ready contract (§4.3).
5. **Reading a net-delivered singleton at your own construct time.** Works in PIE, races on
   clients. Fix: Acquire ticket + `Promise_OnReady`, or the feature's Ready predicate (§5).
6. **Reading replicated values in `OnConstructed`.** Composed ≠ values-applied; use
   `Promise_OnReplicationComplete` (`ckecs-architecture-contract` §7).
7. **Calling it done after listen-server PIE.** The listen host has authority for everything —
   it cannot exhibit mistakes 2, 3, or 5. Gate on a real client run (§7).

---

## Provenance and maintenance

Authored 2026-07-03 against BusterBlock dev tip `52a75e13d` (superproject, detached HEAD) and
CkFoundation framework docs dated 2026-07-02. Research base: the phase-0 corpus reports
(entity-composition, testing-and-incidents, feature-lifecycle, venus-recon, framework-doc-map);
every load-bearing incident and code citation above was independently re-verified on 2026-07-03.

Volatile claims and how to re-verify (Git Bash at the game repo root; note the repo-root
`.ignore` hides `Script/` from ripgrep — always `rg --no-ignore`):

```bash
# Incident commits (§2, §3, §4):
git show --stat de3099e1c d6f9785a4 1b0665a3f a421b2ce2 98e206e97 e140915f7

# The in-source rule + the canonical spawn pattern (§2):
rg --no-ignore -n "REP_DEBUG|ActorRelay channel" Script/ECS/MissionDriver/BB_MissionDriver_EntityScript.as
sed -n '69,135p' Script/WorldSettings/Gameplay/BB_Gameplay_GameState.as

# Rep-notify bind + reconcile (§3):
rg --no-ignore -n -A8 "BindTo_OnRepNotify" Script/StoreDriver/BB_StoreDriver_Processor_Setup.as

# Cue authority pattern (§4.2):
rg --no-ignore -n -B2 -A6 "Request_ExecuteCue" Script/ECS/OpenSign/BB_OpenSign_Hfsm.as
rg --no-ignore -n -A6 "Get_HasAuthority" Script/ECS/OpenSign/BB_OpenSign_Cues.as

# Simulate-everywhere stance (§1.4) and readiness gate (§5):
rg --no-ignore -n "simulate-everywhere|_Replication" Script/Npc/BB_Npc_EntityScript.as
rg --no-ignore -n -B3 -A10 "bool Is_StoreReady" Plugins/BusterBlockTests/Script/Gauntlet/BB_GauntletStoreHelpers.as

# Net autotest census (§7):
rg --no-ignore -l "CkAutoTest_Net" Plugins/BusterBlockTests/Script/Tests/

# Binding-policy names verbatim (§3):
rg -n "FireIfPayloadInFlight" Plugins/CkFoundation/CLAUDE.md
```

If a re-verify diverges (e.g. the OpenSign cue surface or the StoreDriver setup processor is
refactored), trust the code and update the citation — the *rules* (§2 rule, bind-before-broadcast,
authority-writes-only, eager-finish, readiness gating) are invariants of the framework's
replication design and outlive the exhibits.
