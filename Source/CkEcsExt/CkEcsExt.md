# CkEcsExt

Spatial and hierarchical systems for ECS entities. Provides world transforms, scene node parent-child hierarchies, transform interpolation, and granular replication of positional data.

## Key Concepts

- **Transform** — Core 3D position/rotation/scale on an ECS entity. Can sync from an Unreal actor's root component or skeletal mesh socket. Supports granular replication (location, rotation, scale separately).
- **SceneNode** — A child entity with a local (relative) transform attached to a parent Transform. Supports up to 10 layer tags for grouping. Parent moves, children follow.
- **EntityHolder** — Generic pattern for storing a reference to another entity. Template utilities (`TUtils_EntityHolder<T>`) and macros (`CK_DEFINE_ENTITY_HOLDER`) for quick composition.
- **Transform Interpolation** — Configurable smoothing for replicated transforms (strategy, smooth distance, timing).

## Example: Weapon Attached to Character Hand

```mermaid
flowchart LR
    A["Character entity<br/>with Transform"] -->|"SceneNode: Add"| B["Weapon entity<br/>local offset to hand"]
    B -->|"character moves"| C["Weapon auto-updates<br/>relative position"]
```

## Usage Examples

### Add a transform to an entity

```cpp
UCk_Utils_Transform_UE::Add(Entity, InitialTransform);
```

### Sync transform from an Unreal component

```cpp
UCk_Utils_Transform_UE::AddAndAttachToUnrealComponent(Entity, SceneComponent);
```

### Move an entity

```cpp
UCk_Utils_Transform_UE::Request_SetLocation(TransformHandle, NewLocation);
UCk_Utils_Transform_UE::Request_SetTransform(TransformHandle, NewTransform);
```

### Query transform

```cpp
auto Location = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle);
auto Transform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(TransformHandle);
```

### Attach a scene node child

```cpp
UCk_Utils_SceneNode_UE::Add(ParentTransformHandle, LocalOffset);
```

### React to transform changes

```cpp
UCk_Utils_Transform_UE::BindTo_OnUpdate(TransformHandle, OnTransformChangedDelegate);
```

## Tests

No tests found for this module in CkTest.
