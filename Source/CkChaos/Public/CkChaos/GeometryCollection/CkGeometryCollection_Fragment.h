#pragma once

#include "CkGeometryCollection_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkGeometryCollection_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_GeometryCollection_UE;
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
        friend class FProcessor_GeometryCollectionOwner_HandleRequests;
        friend class UCk_Utils_GeometryCollectionOwner_UE;

    public:
        using RequestType = std::variant
        <
            FCk_Request_GeometryCollection_ApplyStrain_Replicated,
            FCk_Request_GeometryCollection_ApplyAoE_Replicated
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

    struct CKCHAOS_API FFragment_GeometryCollection_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_GeometryCollection_Params);

    public:
        using ParamsType = FCk_Fragment_GeometryCollection_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_GeometryCollection_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCHAOS_API FFragment_GeometryCollection_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_GeometryCollection_Current);

    public:
        friend class FProcessor_GeometryCollection_HandleRequests;
        friend class UCk_Utils_GeometryCollection_UE;

    private:
        // Add your properties here
        int32 _DummyProperty = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCHAOS_API FFragment_GeometryCollection_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_GeometryCollection_Requests);

    public:
        friend class FProcessor_GeometryCollection_HandleRequests;
        friend class UCk_Utils_GeometryCollection_UE;

    public:
        using RequestType = std::variant
        <
            FCk_Request_GeometryCollection_ApplyStrain,
            FCk_Request_GeometryCollection_ApplyAoE
        >;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCHAOS_API FFragment_GeometryCollection_ReplicationRequests
    {
    public:
        CK_GENERATED_BODY(FFragment_GeometryCollection_Requests);

    public:
        friend class FProcessor_GeometryCollectionOwner_Replicate;
        friend class FProcessor_GeometryCollectionOwner_HandleRequests;
        friend class UCk_Utils_GeometryCollection_UE;

    public:
        using RequestType = std::variant
        <
            FCk_Request_GeometryCollection_ApplyStrain_Replicated,
            FCk_Request_GeometryCollection_ApplyAoE_Replicated
        >;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_AND_UTILS(FUtils_RecordOfGeometryCollections, FFragment_RecordOfGeometryCollections, FCk_Handle_GeometryCollection);
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
        const FCk_Request_GeometryCollection_ApplyStrain_Replicated& InApplyStrain) -> void;

    auto
    Broadcast_ApplyAoE(
        const FCk_Request_GeometryCollection_ApplyAoE_Replicated& InApplyAoE) -> void;

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

private:
    UPROPERTY(ReplicatedUsing = OnRep_Updated);
    TArray<FCk_Request_GeometryCollection_ApplyStrain_Replicated> _Strain;
    int32 _Strain_LastValidIndex = 0;

    UPROPERTY(ReplicatedUsing = OnRep_Updated);
    TArray<FCk_Request_GeometryCollection_ApplyAoE_Replicated> _AoE;
    int32 _AoE_LastValidIndex = 0;
};

// --------------------------------------------------------------------------------------------------------------------
