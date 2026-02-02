#pragma once

#include "CkEntityCollection_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

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

    struct CKENTITYCOLLECTION_API FFragment_EntityCollection_SyncReplication
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityCollection_SyncReplication);

    private:
        TArray<FCk_EntityCollection_Content> _EntityCollectionsToReplicate;
        TArray<FCk_EntityCollection_Content> _EntityCollectionsToReplicate_Previous;

    public:
        CK_PROPERTY_GET(_EntityCollectionsToReplicate);
        CK_PROPERTY_GET(_EntityCollectionsToReplicate_Previous);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_EntityCollection_SyncReplication, _EntityCollectionsToReplicate, _EntityCollectionsToReplicate_Previous);
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
        FCk_Delegate_EntityCollection_OnCollectionUpdated,
        FCk_Handle_EntityCollection,
        FCk_EntityCollection_Content,
        FCk_EntityCollection_Content,
        TArray<FCk_Handle>,
        TArray<FCk_Handle>);

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_EntityCollection_Requests);
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKENTITYCOLLECTION_API FCk_RepData_EntityCollections
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_EntityCollections);

    UPROPERTY()
    TArray<FCk_EntityCollection_Content> EntityCollections;
};

namespace ck
{
    using FFragment_ContainerRef_EntityCollections = TFragment_ContainerEntryRef<FCk_RepData_EntityCollections>;
}

// --------------------------------------------------------------------------------------------------------------------
