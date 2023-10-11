#pragma once

#include "CkCore/Component/CkActorComponent.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/OwningActor/CkOwningActor_Fragment_Params.h"

#include "CkEcsConstructionScript_ActorComponent.generated.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCk_Delegate_OnReplicationComplete_MC);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract,
       Blueprintable,
       BlueprintType,
       HideCategories("Replication", "ComponentTick", "Rendering", "Activation", "Tags", "ComponentReplication", "Mobile", "RayTracing",
                      "Collision", "AssetUserData", "Cooking", "Sockets", "Variable", "Navigation", "HLOD", "Physics"))
class CKUNREAL_API UCk_EcsConstructionScript_ActorComponent_UE : public UCk_EcsConstructionScript_ActorComponent_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EcsConstructionScript_ActorComponent_UE);

public:
    UCk_EcsConstructionScript_ActorComponent_UE();

    friend class UCk_Fragment_EntityReplicationDriver_Rep;

protected:
    auto OnUnregister() -> void override;

protected:
    auto Do_Construct_Implementation(
        const FCk_ActorComponent_DoConstruct_Params& InParams) -> void override;

private:
    auto TryInvoke_OnReplicationComplete(EInvoke_Caller InCaller) -> void override;

public:
    // Temporary function for Obsidian Toggle
    UFUNCTION(BlueprintImplementableEvent)
    bool ShouldConstruct() const;

public:
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<class UCk_EcsWorld_Subsystem_UE> _EcsWorldSubsystem;

    UPROPERTY(EditDefaultsOnly)
    ECk_Replication _Replication = ECk_Replication::Replicates;

    UPROPERTY(EditDefaultsOnly, Instanced)
    TObjectPtr<class UCk_UnrealEntity_WithActor_PDA> _UnrealEntity;

private:
    UPROPERTY(BlueprintAssignable, Category = "Public",
        meta = (AllowPrivateAccess))
    FCk_Delegate_OnReplicationComplete_MC _OnReplicationComplete_MC;

private:
    enum class EOnReplicationCompleteBroadcastStep
    {
        Wait,
        WaitOnReplicationDriver,
        WaitOnConstructionScript,
        Broadcast
    };

    EOnReplicationCompleteBroadcastStep _ReplicationComplete_BroadcastStep = EOnReplicationCompleteBroadcastStep::Wait;

public:
    CK_PROPERTY(_UnrealEntity);
};

// --------------------------------------------------------------------------------------------------------------------