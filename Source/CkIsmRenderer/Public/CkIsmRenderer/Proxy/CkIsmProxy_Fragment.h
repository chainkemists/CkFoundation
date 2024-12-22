#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_IsmProxy_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_IsmProxy_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_IsmProxy_NeedsInstanceAdded);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKISMRENDERER_API FFragment_IsmProxy_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_IsmProxy_Params);

    public:
        using ParamsType = FCk_Fragment_IsmProxy_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_IsmProxy_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKISMRENDERER_API FFragment_IsmProxy_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_IsmProxy_Current);

    public:
        friend class FProcessor_IsmProxy_Setup;
        friend class FProcessor_IsmProxy_AddInstance;
        friend class FProcessor_IsmProxy_HandleRequests;

    private:
        int32 _IsmInstanceIndex = INDEX_NONE;
        TArray<float> _CustomDataValues;

    public:
        CK_PROPERTY_GET(_IsmInstanceIndex);
        CK_PROPERTY_GET(_CustomDataValues);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKISMRENDERER_API FFragment_IsmProxy_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_IsmProxy_Requests);

    public:
        using SetCustomDataValueRequestType = FCk_Request_IsmProxy_SetCustomDataValue;
        using SetCustomDataRequestType = FCk_Request_IsmProxy_SetCustomData;

        using RequestType = std::variant<SetCustomDataValueRequestType, SetCustomDataRequestType>;
        using RequestList = TArray<RequestType>;

    public:
        friend class FProcessor_IsmProxy_HandleRequests;
        friend class UCk_Utils_IsmProxy_UE;

    private:
        RequestList _Requests;
    };
}

// --------------------------------------------------------------------------------------------------------------------
