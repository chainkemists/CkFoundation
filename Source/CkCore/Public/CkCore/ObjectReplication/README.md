# CkCore / ObjectReplication

`UCk_ReplicatedObject_UE` — a `UObject` base class that supports network replication, RPCs, and UE5's Iris replication system. Use when you need a replicated object that isn't an `AActor` or `UActorComponent`.

**Key file:** `CkReplicatedObject.h`

## Public class

```cpp
UCLASS() class CKCORE_API UCk_ReplicatedObject_UE : public UObject
{
public:
    UCk_ReplicatedObject_UE();
    UCk_ReplicatedObject_UE(const FObjectInitializer&);

    auto GetOwningActor() const -> AActor*;

    // UObject overrides:
    auto CallRemoteFunction     (UFunction*, void*, FOutParmRec*, FFrame*) -> bool     override;
    auto GetFunctionCallspace   (UFunction*, FFrame*)                      -> int32   override;
    auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const      -> void    override;
    auto IsSupportedForNetworking() const                                   -> bool    override;

#if UE_WITH_IRIS
    auto RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext&,
                                      UE::Net::EFragmentRegistrationFlags) -> void override;
#endif
};
```

## What it does

- Routes RPCs through the owning actor's net driver (`CallRemoteFunction`, `GetFunctionCallspace`).
- Participates in both legacy replication and Iris (`RegisterReplicationFragments` under `UE_WITH_IRIS`).
- Returns `IsSupportedForNetworking() = true` so it can be referenced in replicated properties.

## Usage

Inherit, mark properties `Replicated`, and ensure the object is owned by a replicated actor:

```cpp
UCLASS() class UMy_ReplicatedThing : public UCk_ReplicatedObject_UE
{
    GENERATED_BODY()

    UPROPERTY(Replicated) int32 _Value = 0;

    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutProps) const override
    {
        Super::GetLifetimeReplicatedProps(OutProps);
        DOREPLIFETIME(UMy_ReplicatedThing, _Value);
    }
};
```

Outer of the object **must** resolve to an `AActor` via the standard UObject outer chain. `GetOwningActor()` walks outers until it finds one; if it returns `nullptr`, RPCs silently fail.

## When to use this vs. what

- **This** — you want a replicated UObject that's conceptually attached to an actor but isn't an `AActor` or `UActorComponent` (e.g., runtime inventory line items, menu models, data-view models).
- **`UActorComponent`** — you want lifetime managed by the actor's component system.
- **ECS + `CkIrisRelay`-style glue** — you want replicated **entity state**, not a replicated UObject. Replicated state on entities goes through CkEcs's Iris integration, not this base.

## Depends on
`Macros/`, UE's `CoreUObject`, Iris modules (when enabled).

## Used by
UI view-models, runtime inventory views, anywhere a replicated-but-not-actor UObject is needed.

## See also
- `CkEcs` — entity-level replication. Different mechanism, don't conflate.
- UE docs on Iris (`UE_WITH_IRIS`, `FFragmentRegistrationContext`).
