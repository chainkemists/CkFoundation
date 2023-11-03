#pragma once

#include "CkCore/Component/CkActorComponent.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkUnreal/EntityBridge/CkEntityBridge_Fragment_Data.h"

#include "CkEcs/OwningActor/CkOwningActor_Fragment_Params.h"

#include "CkEntityBridge_ConstructionScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCk_Delegate_OnReplicationComplete_MC);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract,
       Blueprintable,
       BlueprintType,
       HideCategories("Replication", "ComponentTick", "Rendering", "Activation", "Tags", "ComponentReplication", "Mobile", "RayTracing",
                      "Collision", "AssetUserData", "Cooking", "Sockets", "Variable", "Navigation", "HLOD", "Physics"))
class CKUNREAL_API UCk_EntityBridge_ActorComponent_UE : public UCk_EntityBridge_ActorComponent_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityBridge_ActorComponent_UE);

public:
    UCk_EntityBridge_ActorComponent_UE();

    friend class UCk_Fragment_EntityReplicationDriver_Rep;

protected:
    auto OnUnregister() -> void override;

protected:
    auto Do_Construct_Implementation(
        const FCk_ActorComponent_DoConstruct_Params& InParams) -> void override;

private:
    auto TryInvoke_OnReplicationComplete(EInvoke_Caller InCaller) -> void override;

public:
    UFUNCTION(BlueprintCallable)
    void Request_ReplicateNonReplicatedActor();

    UFUNCTION(BlueprintCallable)
    bool Get_IsReplicationComplete() const;

    UPROPERTY(EditDefaultsOnly)
    ECk_Replication _Replication = ECk_Replication::Replicates;

    UPROPERTY(EditDefaultsOnly, Instanced)
    TObjectPtr<class UCk_EntityBridge_Config_WithActor_PDA> EntityConfig;

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
    CK_PROPERTY(EntityConfig);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew)
class CKUNREAL_API UCk_EntityBridge_ConstructionScript_WithTransform_PDA : public UCk_EntityBridge_ConstructionScript_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityBridge_ConstructionScript_WithTransform_PDA);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn, AllowPrivateAccess = true))
    FTransform _EntityInitialTransform;

public:
    CK_PROPERTY(_EntityInitialTransform);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKUNREAL_API UCKk_Utils_EntityBridge_ConstructionScript_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCKk_Utils_EntityBridge_ConstructionScript_UE);

private:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|EntityBridge|ConstructionScript")
    static FCk_Handle
    BuildEntity(
        FCk_Handle InHandle,
        const UCk_EntityBridge_Config_Base_PDA* InEntityConfig);
};

// --------------------------------------------------------------------------------------------------------------------
