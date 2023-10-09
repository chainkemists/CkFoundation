#pragma once

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Fragment_Data.h"

#include "CkEntityReplicationDriver_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_EntityReplicationDriver_UE;

namespace ck
{
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
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable)
class CKNET_API UCk_Fragment_EntityReplicationDriver_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_EntityReplicationDriver_Rep);

private:
    auto
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const -> void override;

private:
    UFUNCTION()
    void
    OnRep_ReplicationData();

    UFUNCTION()
    void
    OnRep_ReplicationData_ReplicatedActor();

private:
    UPROPERTY(ReplicatedUsing = OnRep_ReplicationData)
    FCk_EntityReplicationDriver_ReplicationData _ReplicationData;

    UPROPERTY(ReplicatedUsing = OnRep_ReplicationData_ReplicatedActor)
    FCk_EntityReplicationDriver_ConstructionInfo_ReplicatedActor _ReplicationData_ReplicatedActor;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>> _PendingChildEntityConstructions;

public:
    // TODO: reduce the exposure of this variable
    CK_PROPERTY(_ReplicationData);
    CK_PROPERTY(_ReplicationData_ReplicatedActor);
};

// --------------------------------------------------------------------------------------------------------------------
