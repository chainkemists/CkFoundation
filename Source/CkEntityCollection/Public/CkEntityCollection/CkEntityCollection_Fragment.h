#pragma once

#include "CkEntityCollection_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

#include "CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_EntityCollection_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_EntityCollection_CollectionUpdated);

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
