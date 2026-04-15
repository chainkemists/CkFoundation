# CkActor

**Purpose:** Actor ↔ Entity bridge. Provides the fragment and utilities that associate an `AActor` with an ECS entity, plus actor-level gameplay tag constants for CkFoundation actors.

**Depends on:** `CkCore`, `CkEcs`, `CkLog`, `CkVariables`.
**Used by:** `CkEcsExt` (directly — EcsExt's EntityHolder builds on this), plus ~20 feature modules that need actor-owned entities.

---

## What's here

```
CkActor/Public/CkActor/
├── ActorModifier/     – fragment + utils for actor-level modifiers
└── CkActor_GameplayTags.cpp – gameplay tag definitions for CkActor
```

`CkActor` is deliberately thin. Its job is to declare the bridge data (fragments, tag constants) that higher-level modules consume. `CkEcsExt` owns the full actor-binding lifecycle logic.

---

## Owning actor pattern

Entities that are driven by an actor store an `FFragment_OwningActor_Current` fragment (defined in `CkEcs/OwningActor/`). `CkActor` extends this with actor-specific modifiers and tag associations.

Typical creation pattern (through a feature module's Utils):

```cpp
// 1. Spawn the entity
auto EntityHandle = UCk_Utils_EntityLifetime_UE::Request_SpawnEntity(World, SpawnParams);

// 2. Associate the actor (via CkEcsExt's EntityHolder or the OwningActor fragment)
// The OwningActor fragment is set by the feature module's setup processor.
```

Actor utilities (for getting the actor from an entity or vice versa) live in `CkCore/Actor/CkActor_Utils.h` and `CkEcs/OwningActor/`.

---

## Actor-modifier fragment

`ActorModifier/` provides a fragment for temporary actor-state overrides (visibility, collision, movement locks) applied to an actor through its associated entity. Useful for ability/VFX systems that need to temporarily alter actor properties without tangling ECS state with actor state.

---

## Anti-patterns

1. Don't hold an `AActor*` raw pointer directly in a processor fragment — use `TStrongObjectPtr<AActor>` or route through the OwningActor fragment. Raw actor pointers can become stale during streaming.
2. Don't assume every entity has an actor. ECS entities are independent of actors; only entities explicitly given an OwningActor fragment have one.

---

## See also
- `CkEcsExt/Claude.md` — EntityHolder lifecycle binding (actor destroyed → entity destroyed).
- `CkEcs/Claude.md` — `FFragment_OwningActor_Current` in `CkEcs/OwningActor/`.
- `CkCore/Actor/README.md` — `UCk_Utils_Actor_UE` spawn/find helpers.
