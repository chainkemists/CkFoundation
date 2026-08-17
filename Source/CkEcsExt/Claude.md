# CkEcsExt

**Purpose:** Higher-level ECS utilities built on top of `CkEcs`. Adds entity-tree structures (SceneNode), transform management, EntityHolder (actor ↔ entity bridging helpers), EntityScript extensions, and the `UCk_Utils_Ecs_Base_UE` meta-fragment infrastructure.

**Depends on:** `CkActor`, `CkCore`, `CkEcs`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`.
**Used by:** ~50 feature modules — nearly everything that touches entities goes through both `CkEcs` and `CkEcsExt`.

---

## Module layout

```
CkEcsExt/Public/CkEcsExt/
├── CkEcsExt_Utils.h   – UCk_Utils_Ecs_Base_UE (meta-fragment param storage)
├── EntityHolder/      – EntityHolder fragment (actor-holds-entity lifetime binding)
├── EntityScript/      – EntityScript extensions (additional script helpers)
├── SceneNode/         – parent/child scene tree for entities
├── Settings/          – EcsExt-specific settings
└── Transform/         – transform fragment utilities
```

---

## Core utilities

### `UCk_Utils_Ecs_Base_UE`

Provides two cross-cutting services:

1. **Meta-fragment parameter storage** — when a Meta Fragment (an entity-that-acts-like-a-component) is constructed on a client via replication, its construction params need to be pre-stored so the EntityScript can retrieve them. `UCk_Utils_Ecs_Base_UE` owns the param queue so clients don't have to manage it manually.
2. **Cross-record entity lookup** — `Get_EntityOrRecordEntry_WithFragmentAndLabel` queries both the main entity and its Record entries simultaneously, returning whichever has the specified fragment+label combo. Reduces O(n) searches at call sites.

A **Meta Fragment** is an entity whose handle is stored as a fragment on another entity — externally it looks like a regular fragment. The EntityScript for a Meta Fragment has its own construction lifecycle. `CkMeter`, `CkAttribute`, and similar systems use this pattern.

### EntityHolder

Binds an `AActor`'s lifetime to an entity. When the actor is destroyed, the entity is automatically destroyed. When an entity exists without an actor owner, use `OwningActor/` fragments in `CkEcs` instead.

### SceneNode

Parent/child scene tree composed of ECS entities. Lets you parent entities to other entities (not actors) with relative transform support. Used by `CkIsmRenderer`, multi-part entities, and procedural hierarchies.

### Transform

Transform fragment utilities — read/write world/local transform of an entity, propagate to actor components when present.

---

## Common patterns

```cpp
// Checking if a handle is a valid Record or Record entry with a given fragment:
auto FoundHandle = UCk_Utils_Ecs_Base_UE::Get_EntityOrRecordEntry_WithFragmentAndLabel<
    TFragment_MyFeature_Utils,
    TRecord_MyFeature_Utils>(InHandle, MyFeatureLabel);
```

---

## Anti-patterns

1. Don't roll your own param-storage for Meta Fragments. Use `UCk_Utils_Ecs_Base_UE`'s queue — skipping it causes construction-order bugs on clients.
2. Don't build entity hierarchies with raw `FCk_Entity` parent pointers. Use `SceneNode/` — it handles destruction ordering and transform propagation.
3. Don't hold an actor ↔ entity relationship by keeping both raw pointers. Use `EntityHolder/` to bind lifetimes.

---

## Implementation notes

### SceneNode layer ordering

Every `TProcessor_SceneNode_Update<FTag_SceneNode_LayerN>` declares a `RunAfter` list holding
`FProcessor_SceneNode_FollowUnrealAnchor`, plus `FProcessor_Transform_HandleRequests` for layer 0 and
`TProcessor_SceneNode_Update<Layer N-1>` for the rest. Both dependencies are load-bearing:

- Without `Transform_HandleRequests` in layer 0's list, layer 0 can run in parallel with request handling
  and miss the `FTag_Transform_Updated` that handling just added to the root (e.g. a tween writing world
  onto the root), so the gate check fails and nothing propagates.
- Without the layer-to-layer chain, descendants run before the tag their parent DEFERRED becomes visible.

Either gap stops motion from propagating past the first scene-node link, silently.

### SceneNode Unreal anchors

`FFragment_SceneNode_UnrealAnchor` is a scene-node's foreign Unreal anchor: a `USceneComponent`
(`Socket == None`) or a mesh socket. `FProcessor_SceneNode_FollowUnrealAnchor` composes
`entity.world = offset * anchor.world` every tick, read-only w.r.t. the anchor.

It is deliberately NOT the Transform module's `FFragment_Transform_RootComponent` / `_MeshSocket`: those
pair with `SyncFromActor`/`SyncToActor`, a bidirectional actor bridge that would drag a Movable anchor
around. Keeping a separate fragment makes anchor-follow a read-only SceneNode concern and leaves the
Transform feature untouched. The anchor fragment is not snapshotable (a live component ref can't be
remapped — parity with the Transform anchor fragments); the composed world pose is restored via
`FFragment_Transform`.

`FTag_Transform_ExternallyDriven` is the inverse switch on the Transform side: an anchor-bound child that
should instead follow its scene-node parent (Unreal `AttachToComponent` semantics) carries the tag, and
`SyncFromActor`/`SyncFromMeshSocket` back off while `SyncToActor` keeps pushing the parent-driven pose onto
the anchor.

### Physics ownership

An entity is EITHER Chaos-simulated (CkOverlapBody's `UShapeComponent` path, CkRaySense's engine-trace
path) OR Jolt-simulated (CkSpatialQuery's Probe, CkJolt's bodies/characters) — never both; the two engines
maintain independent, non-interacting world representations. Composing both onto one entity is a design
error surfaced at COMPOSITION time by `ck::physics_ownership::TryClaim_Chaos` / `TryClaim_Jolt`
(`CK_ENSURE` at the composing call site + `false` return, so the caller returns an invalid handle), never a
runtime log.

The two tags are COUNTED because multiple SAME-world features may legitimately stack on one entity (a Probe
and a JoltBody both claim Jolt; two Sensors both claim Chaos) — only the cross-world claim is a conflict.
`Release_*` drops one claim; EndPlay teardown does not need to release (the entity is dying).

### Bridged actors after a snapshot restore — respawn, not rebind

A restored bridged entity comes back with no live actor (the OwningActor + Transform-root links are not
persistable). The v3 loader's answer is **actor-first respawn**, not a rebind of the restored entity: it
spawns a FRESH actor of `FFragment_ActorSpawnIntent`'s class at the saved transform (deferred, so the
saved `UPROPERTY(SaveGame)` fields land before `BeginPlay`), and that actor's own `BeginPlay` re-runs
`WithActor::Construct`, which composes the entity and links the bridge exactly as in a fresh world. The
saved payloads then hydrate onto it. See `CkSnapshot/CLAUDE.md` § "The v3 load machine".

Because Construct DOES re-run, actor-side wiring is rebuilt by the normal construction path and no
post-restore reattach hook is needed. An earlier design re-bridged the restored entity in place instead
(`UCk_Utils_ActorRebind_UE::Request_RebindActor` + a `ck::FTag_ActorJustRebound` marker game code keyed a
reattach processor on); it lost its last caller when `FProcessor_ActorRespawn` was deleted (`a8ad20458`,
2026-07-12) and the whole path was removed in 2026-08. Do not reintroduce it — under v3 there is nothing
for it to do.

`WithActor::Construct` only stamps the intent for a RUNTIME-spawned actor. A level-placed one gets
`FFragment_SaveKey` (from `ck::save_key::Get_LevelPlacedIdentity`) instead: the level re-creates it on every boot,
so respawning it from the save would duplicate it, and capture's classification lets the intent outrank the key —
they are alternatives, never both. `_SnapshotRespawnable` therefore only governs runtime-spawned bridges.

`FFragment_ActorSpawnIntent` stores the class as a plain `FString` (`UClass::GetPathName`), NOT a
`TSoftClassPtr`: a soft-object path does not survive the snapshot's SaveGame memory-archive
`SerializeItem` round-trip (it comes back empty), whereas a plain FString round-trips reliably — the same
value path `FFragment_SaveKey`'s FGuid uses. Resolved at respawn via `FSoftClassPath::TryLoadClass`. The
raw `AActor` pointer is never serialized; the spawn transform comes from the restored Transform fragment,
not from this fragment.

---

## See also
- `CkEcs/Claude.md` — the handle, processor, signal, and EntityScript primitives.
- `CkRecord/Claude.md` — the Record system that Meta Fragments are stored in.
- `CkLabel/Claude.md` — label matching used in cross-record lookup.

## Transform-local settle after derived producers

`FProcessor_Transform_HandleRequests` and the `TProcessor_SceneNode_Update` layer chain are registered once
in `FGroup_Transform`. Those canonical processors declare `LocalSettleAfter = FGroup_Transform_Derived`, so
the scheduler can replay the same ordered transform-resolution slice before `FGroup_Transform_Finalize`.
The request drain additionally declares `LocalSettleTrigger = true`, so the barrier activates only when its
consumed transform request fragment is dirty; the marker-less SceneNode layers
then replay in their normal depth order. A camera view-anchor request therefore drains and all descendants
compose in the same frame without a camera exception, duplicate processor identity, or a second transform
group.

Do not activate this barrier from `FTag_Transform_Updated`: that tag remains present until cleanup and cannot
converge a settle loop. The consumed transform request queue is the activation source. The SceneNode chain
is replay-only and stays unaware of which derived system produced the request.
