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

### Actor rebind after a snapshot restore

A snapshot restore severs the actor↔entity bridge: `Run_Restore`'s `clear()` drops the non-snapshotable
OwningActor + Transform-root links, so a restored bridged entity comes back with no live actor. After the
snapshot respawn pass spawns a fresh actor of `FFragment_ActorSpawnIntent`'s class,
`UCk_Utils_ActorRebind_UE::Request_RebindActor` re-links the two WITHOUT re-running
`WithActor::Construct` — the gameplay fragments already round-tripped. It re-adds the actor→entity
reverse-lookup component, the entity→actor OwningActor fragment, and the Transform bound to the actor's
root component (OwningActor first, so `Transform::Add` routes to `AddAndAttachToUnrealComponent` and
PRESERVES the restored world transform rather than seeding from the actor's spawn transform).

Because Construct does not re-run, any ACTOR-SIDE wiring the original construction did (cached entity
handles on the actor, `NewObject` components, camera directors, …) is dead on the respawned actor. Game
code keys a processor on `ck::FTag_ActorJustRebound` to run its own idempotent reattach, then REMOVES the
tag as its done-guard. The tag is TRANSIENT — one-shot post-restore bookkeeping, never captured into a save.

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
