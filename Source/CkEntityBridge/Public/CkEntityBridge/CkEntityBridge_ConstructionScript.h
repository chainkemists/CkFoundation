#pragma once

#include "CkCore/Component/CkActorComponent.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkCore/SharedValues/CkSharedValues.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkEntityBridge/CkEntityBridge_Fragment_Data.h"

#include "CkEcs/OwningActor/CkOwningActor_Fragment_Data.h"

#include "CkEntityBridge_ConstructionScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_OnReplicationComplete_MC,
    FCk_Handle, InEntity);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_OnInjectEntityConstructionParams_MC,
    FCk_SharedInstancedStruct, InParamsToInject);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_PreConstruct_MC,
    FCk_Handle, InEntity);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract,
       Blueprintable,
       BlueprintType,
       HideCategories("Replication", "ComponentTick", "Rendering", "Activation", "Tags", "ComponentReplication", "Mobile", "RayTracing",
                      "Collision", "AssetUserData", "Cooking", "Sockets", "Variable", "Navigation", "HLOD", "Physics"))
class CKENTITYBRIDGE_API UCk_EntityBridge_ActorComponent_UE : public UCk_EntityBridge_ActorComponent_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityBridge_ActorComponent_UE);

public:
    UCk_EntityBridge_ActorComponent_UE();

    friend class UCk_Fragment_EntityReplicationDriver_Rep;

protected:
    auto
    OnRegister() -> void override;

    auto
    OnUnregister() -> void override;

protected:
    auto
    Do_Construct_Implementation(
        const FCk_ActorComponent_DoConstruct_Params& InParams) -> void override;

private:
    auto
    TryInvoke_OnPreConstruct(
        const FCk_Handle& Entity,
        EInvoke_Caller InCaller) const -> void override;

    auto
    TryInvoke_OnReplicationComplete(
        EInvoke_Caller InCaller) -> void override;

    auto
    Get_EntityConstructionParamsToInject() const -> FInstancedStruct override;

public:
    UFUNCTION(BlueprintCallable)
    void
    Request_ReplicateNonReplicatedActor();

    UFUNCTION(BlueprintCallable)
    bool
    Get_IsReplicationComplete() const;

    UPROPERTY(EditDefaultsOnly)
    ECk_Replication _Replication = ECk_Replication::Replicates;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TSubclassOf<UCk_Entity_ConstructionScript_PDA> _ConstructionScript;

    UPROPERTY(Instanced)
    TObjectPtr<class UCk_EntityBridge_Config_WithActor_PDA> EntityConfig;

private:
    UPROPERTY(BlueprintAssignable, Category = "Public", DisplayName = "On Replication Complete",
        meta = (AllowPrivateAccess))
    FCk_Delegate_OnReplicationComplete_MC _OnReplicationComplete_MC;

    UPROPERTY(BlueprintAssignable, Category = "Public", DisplayName = "On Inject Entity Construction Params (DEPRECATED - Use PreConstruct)",
        meta = (AllowPrivateAccess, DeprecatedProperty))
    FCk_Delegate_OnInjectEntityConstructionParams_MC _OnInjectEntityConstructionParams_MC;

    UPROPERTY(BlueprintAssignable, Category = "Public", DisplayName = "On PreConstruct",
        meta = (AllowPrivateAccess, DeprecatedProperty))
    FCk_Delegate_PreConstruct_MC _OnPreConstruct;

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
    CK_PROPERTY(EntityConfig);
};

// --------------------------------------------------------------------------------------------------------------------
