#pragma once

#include "CkStateTree_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_StateTree_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_StateTree_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_StateTree_Updated);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATETREE_API FFragment_StateTree_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_StateTree_Params);

    public:
        using ParamsType = FCk_Fragment_StateTree_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_StateTree_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATETREE_API FFragment_StateTree_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_StateTree_Current);

    public:
        friend class FProcessor_StateTree_Setup;
        friend class FProcessor_StateTree_HandleRequests;
        friend class FProcessor_StateTree_EndPlay;
        friend class UCk_Utils_StateTree_UE;

    private:
        // Add your properties here
        int32 _DummyProperty = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSTATETREE_API FFragment_StateTree_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_StateTree_Requests);

    public:
        friend class FProcessor_StateTree_HandleRequests;
        friend class UCk_Utils_StateTree_UE;

    public:
        using RequestType = std::variant
        <
            FCk_Request_StateTree_ExampleRequest
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