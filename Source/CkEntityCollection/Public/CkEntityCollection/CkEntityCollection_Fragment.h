#pragma once

#include "CkEntityCollection_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

#include "CkSignal/CkSignal_Macros.h"

#include "CkEntityCollection_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_EntityCollection_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_EntityCollection_CollectionUpdated);
    CK_DEFINE_ECS_TAG(FTag_EntityCollection_MayRequireReplication);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKENTITYCOLLECTION_API FFragment_EntityCollection_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityCollection_Params);

    public:
        using ParamsType = FCk_Fragment_EntityCollection_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_EntityCollection_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKENTITYCOLLECTION_API FFragment_EntityCollection_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityCollection_Requests);

    public:
        friend class FProcessor_EntityCollection_HandleRequests;
        friend class UCk_Utils_EntityCollection_UE;

        using AddEntitiesRequestType = FCk_Request_EntityCollection_AddEntities;
        using RemoveEntitiesRequestType = FCk_Request_EntityCollection_RemoveEntities;

        using RequestType = std::variant<AddEntitiesRequestType, RemoveEntitiesRequestType>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfEntityCollections, FCk_Handle_EntityCollection);
    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_EntityCollections_RecordOfEntities, FCk_Handle);
    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_EntityCollections_RecordOfEntities_Previous, FCk_Handle);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKENTITYCOLLECTION_API,
        EntityCollection_OnCollectionUpdated,
        FCk_Delegate_EntityCollection_OnCollectionUpdated_MC,
        FCk_Handle_EntityCollection,
        TArray<FCk_Handle>,
        TArray<FCk_Handle>);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_EntityCollection_Replicate; }

UCLASS(Blueprintable)
class CKENTITYCOLLECTION_API UCk_Fragment_EntityCollection_Rep : public UCk_Ecs_ReplicatedObject_UE
{
private:
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_EntityCollection_Rep);

public:
    auto
    Broadcast_AddOrUpdate(
        const FCk_EntityCollection_Content& InEntityCollectionContent) -> void;

public:
    auto
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>&) const -> void override;

private:
    auto
    PostLink() -> void override;

public:
    auto
    Request_TryUpdateReplicatedEntityCollections() -> void;

private:
    UFUNCTION()
    void
    OnRep_Updated();

private:
    UPROPERTY(ReplicatedUsing = OnRep_Updated);
    TArray<FCk_EntityCollection_Content> _EntityCollectionsToReplicate;
    TArray<FCk_EntityCollection_Content> _EntityCollectionsToReplicate_Previous;
};

// --------------------------------------------------------------------------------------------------------------------
