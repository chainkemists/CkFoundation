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

public:
    //auto
    //Request_ReplicateEntity(
    //    const FCk_EntityReplicationDriver_ConstructionInfo& InConstructionInfo) -> void;

    UFUNCTION(Server, Reliable)
    void
    Request_ReplicateEntity_OnServer(
        const FCk_EntityReplicationDriver_ConstructionInfo& InConstructionInfo);

private:
    UFUNCTION()
    void
    OnRep_ReplicationData();

private:
    UPROPERTY(ReplicatedUsing = OnRep_ReplicationData)
    TArray<FCk_EntityReplicationDriver_ReplicationData> _ReplicationData;

    int32 _ReplicateFrom = 0;

public:
    // TODO: reduce the exposure of this variable
    CK_PROPERTY(_ReplicationData);
};

// --------------------------------------------------------------------------------------------------------------------
