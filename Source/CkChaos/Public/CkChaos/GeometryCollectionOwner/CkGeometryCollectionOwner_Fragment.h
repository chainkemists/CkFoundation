#pragma once

#include "CkGeometryCollectionOwner_Fragment_Data.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkGeometryCollectionOwner_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_GeometryCollectionOwner_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    struct CKCHAOS_API FFragment_GeometryCollectionOwner_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_GeometryCollectionOwner_Requests);

    public:
        friend class UCk_Utils_GeometryCollectionOwner_UE;
        friend class FProcessor_GeometryCollectionOwner_HandleRequests;

    public:
        using RequestType = std::variant
        <
            FCk_Request_GeometryCollectionOwner_ApplyRadialStrain_Replicated
        >;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_GeometryCollectionOwner_RequiresSetup);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCHAOS_API FFragment_GeometryCollection_ReplicationRequests
    {
    public:
        CK_GENERATED_BODY(FFragment_GeometryCollection_ReplicationRequests);

    public:
        friend class FProcessor_GeometryCollectionOwner_Replicate;
        friend class FProcessor_GeometryCollectionOwner_HandleRequests;

    public:
        using RequestType = std::variant
        <
            FCk_Request_GeometryCollectionOwner_ApplyRadialStrain_Replicated
        >;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKCHAOS_API FCk_RepData_GeometryCollectionOwner
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_GeometryCollectionOwner);

    UPROPERTY()
    int32 CrumbleNonActiveClustersRequest = 0;

    UPROPERTY()
    int32 RemoveAllAnchors = 0;

    UPROPERTY()
    int32 RemoveAllAnchorsAndCrumbleNonActiveClusters = 0;

    UPROPERTY()
    TArray<FCk_Request_GeometryCollectionOwner_ApplyRadialStrain_Replicated> RadialStrains;
};

namespace ck
{
    using FFragment_ContainerRef_GeometryCollectionOwner = TFragment_ContainerEntryRef<FCk_RepData_GeometryCollectionOwner>;
}

// --------------------------------------------------------------------------------------------------------------------
