#pragma once

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Fragment_Data.h"

#include "CkSignal/Public/CkSignal/CkSignal_Macros.h"

#include "CkEntityReplicationDriver_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKNET_API,
        OnReplicationComplete,
        FCk_Delegate_EntityReplicationDriver_OnReplicationComplete_MC,
        FCk_Handle);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKNET_API,
        OnDependentsReplicationComplete,
        FCk_Delegate_EntityReplicationDriver_OnReplicationComplete_MC,
        FCk_Handle);
}

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_EntityReplicationDriver_UE;

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_ReplicationDriver_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ReplicationDriver_Requests);

    public:
        friend class FProcessor_ReplicationDriver_HandleRequests;
        friend class UCk_Utils_EntityReplicationDriver_UE;

    public:
        using RequestType = FCk_Request_ReplicationDriver_ReplicateEntity;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable)
class CKNET_API UCk_Fragment_EntityReplicationDriver_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    friend class UCk_Utils_EntityReplicationDriver_UE;
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_EntityReplicationDriver_Rep);

public:
    auto
    Get_IsReplicationCompleteOnAllDependents() -> bool;

private:
    auto
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const -> void override;

private:
    UFUNCTION(Server, Reliable)
    void
    Request_Replicate_NonReplicatedActor(
        FCk_EntityReplicationDriver_ConstructionInfo_NonReplicatedActor InConstructionInfo);

private:
    UFUNCTION()
    void
    OnRep_ReplicationData();

    UFUNCTION()
    void
    OnRep_ReplicationData_Ability();

    UFUNCTION()
    void
    OnRep_ReplicationData_ReplicatedActor();

    UFUNCTION()
    void
    OnRep_ReplicationData_NonReplicatedActor();

    UFUNCTION()
    void
    OnRep_ExpectedNumberOfDependentReplicationDrivers();

private:
    UPROPERTY(ReplicatedUsing = OnRep_ReplicationData)
    FCk_EntityReplicationDriver_ReplicationData _ReplicationData;

    UPROPERTY(ReplicatedUsing = OnRep_ReplicationData_Ability)
    FCk_EntityReplicationDriver_AbilityData _ReplicationData_Ability;

    UPROPERTY(ReplicatedUsing = OnRep_ReplicationData_ReplicatedActor)
    FCk_EntityReplicationDriver_ConstructionInfo_ReplicatedActor _ReplicationData_ReplicatedActor;

    UPROPERTY(ReplicatedUsing = OnRep_ReplicationData_NonReplicatedActor)
    FCk_EntityReplicationDriver_ConstructionInfo_NonReplicatedActor _ReplicationData_NonReplicatedActor;

    UPROPERTY(ReplicatedUsing = OnRep_ExpectedNumberOfDependentReplicationDrivers)
    int32 _ExpectedNumberOfDependentReplicationDrivers = -1;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>> _PendingChildEntityConstructions;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>> _PendingChildAbilityEntityConstructions;

private:
    auto
    DoAdd_SyncedDependentReplicationDriver() -> void;

private:
    UPROPERTY(Transient)
    int32 _NumSyncedDependentReplicationDrivers = 0;

public:
    // TODO: reduce the exposure of this variable
    CK_PROPERTY(_ReplicationData);
    CK_PROPERTY(_ReplicationData_Ability);
    CK_PROPERTY(_ReplicationData_ReplicatedActor);
    CK_PROPERTY(_ReplicationData_NonReplicatedActor);
    CK_PROPERTY(_ExpectedNumberOfDependentReplicationDrivers);
};

// --------------------------------------------------------------------------------------------------------------------
