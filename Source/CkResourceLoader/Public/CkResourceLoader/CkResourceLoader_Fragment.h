#pragma once

#include "CkResourceLoader_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkSignal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ResourceLoader_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKRESOURCELOADER_API FFragment_ResourceLoader_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_ResourceLoader_Params);

    public:
        using ParamsType = FCk_Fragment_ResourceLoader_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ResourceLoader_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRESOURCELOADER_API FFragment_ResourceLoader_PendingObjects
    {
    public:
        CK_GENERATED_BODY(FFragment_ResourceLoader_PendingObjects);

    public:
        friend class FProcessor_ResourceLoader_HandleRequests;
        friend class UCk_Utils_ResourceLoader_UE;

    private:
        TArray<FCk_ResourceLoader_PendingObject> _PendingObjects;

    public:
        CK_PROPERTY(_PendingObjects);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRESOURCELOADER_API FFragment_ResourceLoader_PendingObjectBatches
    {
    public:
        CK_GENERATED_BODY(FFragment_ResourceLoader_PendingObjectBatches);

    public:
        friend class FProcessor_ResourceLoader_HandleRequests;
        friend class UCk_Utils_ResourceLoader_UE;

    private:
        TArray<FCk_ResourceLoader_PendingObjectBatch> _PendingObjectBatches;

    public:
        CK_PROPERTY(_PendingObjectBatches);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRESOURCELOADER_API FFragment_ResourceLoader_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ResourceLoader_Requests);

    public:
        friend class FProcessor_ResourceLoader_HandleRequests;
        friend class UCk_Utils_ResourceLoader_UE;

    private:
        using LoadObjectRequestType = FCk_Request_ResourceLoader_LoadObject;
        using LoadObjectBatchRequestType = FCk_Request_ResourceLoader_LoadObjectBatch;
        using UnloadObjectRequestType = FCk_Request_ResourceLoader_UnloadObject;

        using RequestType = std::variant<LoadObjectRequestType, LoadObjectBatchRequestType, UnloadObjectRequestType>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKRESOURCELOADER_API, ResourceLoader_OnObjectLoaded,
        FCk_Delegate_ResourceLoader_OnObjectLoaded_MC, FCk_Handle, FCk_Payload_ResourceLoader_OnObjectLoaded);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKRESOURCELOADER_API, ResourceLoader_OnObjectBatchLoaded,
        FCk_Delegate_ResourceLoader_OnObjectBatchLoaded_MC, FCk_Handle, FCk_Payload_ResourceLoader_OnObjectBatchLoaded);
}

// --------------------------------------------------------------------------------------------------------------------
