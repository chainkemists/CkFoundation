#pragma once

#include "CkBallistics_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Ballistics_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Ballistics_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_Ballistics_Updated);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKBALLISTICS_API FFragment_Ballistics_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Ballistics_Params);

    public:
        using ParamsType = FCk_Fragment_Ballistics_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Ballistics_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKBALLISTICS_API FFragment_Ballistics_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Ballistics_Current);

    public:
        friend class FProcessor_Ballistics_Setup;
        friend class FProcessor_Ballistics_HandleRequests;
        friend class FProcessor_Ballistics_EndPlay;
        friend class UCk_Utils_Ballistics_UE;

    private:
        // Add your properties here
        int32 _DummyProperty = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKBALLISTICS_API FFragment_Ballistics_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Ballistics_Requests);

    public:
        friend class FProcessor_Ballistics_HandleRequests;
        friend class UCk_Utils_Ballistics_UE;

    public:
        using RequestType = std::variant
        <
            FCk_Request_Ballistics_ExampleRequest
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
// --------------------------------------------------------------------------------------------------------------------