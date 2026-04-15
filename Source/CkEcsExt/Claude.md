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

## See also
- `CkEcs/Claude.md` — the handle, processor, signal, and EntityScript primitives.
- `CkRecord/Claude.md` — the Record system that Meta Fragments are stored in.
- `CkLabel/Claude.md` — label matching used in cross-record lookup.
