#pragma once
#include "CkCore/ObjectReplication/CkObjectReplicatorComponent.h"
#include "CkCore/ObjectReplication/CkReplicatedObject.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkMacros/CkMacros.h"

#include "CkReplicatedObjects_Fragment_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// TODO: Move this to its own file
UCLASS(NotBlueprintType, NotBlueprintable)
class CKECS_API UCk_Ecs_ReplicatedObject
    : public UCk_ReplicatedObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ecs_ReplicatedObject);

public:
    // TODO: Remove as ActorInfo is no longer required to know about ReplicatedObject
    friend class UCk_Utils_OwningActor_UE;
    friend class ACk_World_Actor_Replicated_UE;
    friend struct FCk_ReplicatedObjects;

public:
    static auto Create(
        TSubclassOf<UCk_Ecs_ReplicatedObject> InReplicatedObject,
        AActor* InTopmostOwningActor,
        FName InName,
        FCk_Handle InAssociatedEntity) -> UCk_Ecs_ReplicatedObject*;

public:
    UFUNCTION()
    virtual void OnRep_ReplicatedActor(AActor* InActor);

    virtual auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const -> void override;

protected:
    UPROPERTY(Transient)
    FCk_Handle _AssociatedEntity;

    UPROPERTY(ReplicatedUsing = OnRep_ReplicatedActor)
    AActor* _ReplicatedActor = nullptr;

public:
    CK_PROPERTY(_AssociatedEntity);
    CK_PROPERTY_GET(_ReplicatedActor);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKECS_API FCk_ReplicatedObjects
{
    GENERATED_BODY()

    friend class UCk_Utils_ReplicatedObjects_UE;

public:
    CK_GENERATED_BODY(FCk_ReplicatedObjects);

    // TODO: clean up the struct

private:
    UPROPERTY()
    TArray<UCk_ReplicatedObject*> _ReplicatedObjects;

private:
    auto DoRequest_LinkAssociatedEntity(FCk_Handle InEntity) -> void;

public:
    CK_PROPERTY(_ReplicatedObjects);
};

// --------------------------------------------------------------------------------------------------------------------
