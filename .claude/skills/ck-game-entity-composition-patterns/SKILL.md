---
name: ck-game-entity-composition-patterns
description: 'Use when choosing a CkFoundation game entity archetype, lifetime owner, context root, child-entity topology, or the narrow cases that justify an Actor bridge.'
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


## Reference files — load only what the task needs

Section numbers cited elsewhere in this skill point into these files.

| Topic | Read |
|---|---|
| The archetype catalogue | `references/archetype-catalogue.md` |
| Anti-patterns — with the incidents that proved them | `references/anti-patterns.md` |

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
