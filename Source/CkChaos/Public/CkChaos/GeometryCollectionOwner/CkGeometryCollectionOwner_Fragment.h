#pragma once

#include "CkGeometryCollectionOwner_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

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
            FCk_Request_GeometryCollectionOwner_ApplyStrain_Replicated,
            FCk_Request_GeometryCollectionOwner_ApplyAoE_Replicated
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
            FCk_Request_GeometryCollectionOwner_ApplyStrain_Replicated,
            FCk_Request_GeometryCollectionOwner_ApplyAoE_Replicated
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

namespace ck { class FProcessor_GeometryCollectionOwner_Replicate; }

UCLASS(Blueprintable)
class CKCHAOS_API UCk_Fragment_GeometryCollectionOwner_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_GeometryCollectionOwner_Rep);

public:
    friend class ck::FProcessor_GeometryCollectionOwner_Replicate;

public:
    auto
    Broadcast_ApplyStrain(
        const FCk_Request_GeometryCollectionOwner_ApplyStrain_Replicated& InApplyStrain) -> void;

    auto
    Broadcast_ApplyAoE(
        const FCk_Request_GeometryCollectionOwner_ApplyAoE_Replicated& InApplyAoE) -> void;

    auto
    Broadcast_CrumbleNonActiveClusters() -> void;

    auto
    Broadcast_RemoveAllAnchors() -> void;

    auto
    Broadcast_RemoveAllAnchorsAndCrumbleNonActiveClusters() -> void;

public:
    auto
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>&) const -> void override;

protected:
    auto
    PostLink() -> void override;

private:
    UFUNCTION()
    void
    OnRep_Updated();

    UFUNCTION()
    void
    OnRep_CrumbleNonActiveClustersRequest();

    UFUNCTION()
    void
    OnRep_RemoveAllAnchors();

    UFUNCTION()
    void
    OnRep_CrumbleNonActiveClustersAndRemoveAllAnchors();

private:
    UPROPERTY(ReplicatedUsing = OnRep_CrumbleNonActiveClustersRequest);
    int32 _CrumbleNonActiveClustersRequest = 0;

    UPROPERTY(ReplicatedUsing = OnRep_RemoveAllAnchors);
    int32 _RemoveAllAnchors = 0;

    UPROPERTY(ReplicatedUsing = OnRep_CrumbleNonActiveClustersAndRemoveAllAnchors);
    int32 _RemoveAllAnchorsAndCrumbleNonActiveClusters = 0;

    UPROPERTY(ReplicatedUsing = OnRep_Updated);
    TArray<FCk_Request_GeometryCollectionOwner_ApplyStrain_Replicated> _Strain;
    int32 _Strain_LastValidIndex = 0;

    UPROPERTY(ReplicatedUsing = OnRep_Updated);
    TArray<FCk_Request_GeometryCollectionOwner_ApplyAoE_Replicated> _AoE;
    int32 _AoE_LastValidIndex = 0;
};

// --------------------------------------------------------------------------------------------------------------------
