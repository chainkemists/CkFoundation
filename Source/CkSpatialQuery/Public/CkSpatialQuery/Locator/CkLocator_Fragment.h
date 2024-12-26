#pragma once

#include "CkLocator_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkLocator_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Locator_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Locator_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_Locator_Updated);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSPATIALQUERY_API FFragment_Locator_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Locator_Params);

    public:
        using ParamsType = FCk_Fragment_Locator_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Locator_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSPATIALQUERY_API FFragment_Locator_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Locator_Current);

    public:
        friend class FProcessor_Locator_Setup;
        friend class FProcessor_Locator_HandleRequests;
        friend class FProcessor_Locator_Teardown;
        friend class UCk_Utils_Locator_UE;

    private:
        // Add your properties here
        int32 _DummyProperty = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSPATIALQUERY_API FFragment_Locator_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Locator_Requests);

    public:
        friend class FProcessor_Locator_HandleRequests;
        friend class UCk_Utils_Locator_UE;

    public:
    	using RequestType = std::variant
        <
            FCk_Request_Locator_ExampleRequest
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

namespace ck { class FProcessor_Locator_Replicate; }

UCLASS(Blueprintable)
class CKSPATIALQUERY_API UCk_Fragment_Locator_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_Locator_Rep);

public:
    friend class ck::FProcessor_Locator_Replicate;
};

// --------------------------------------------------------------------------------------------------------------------