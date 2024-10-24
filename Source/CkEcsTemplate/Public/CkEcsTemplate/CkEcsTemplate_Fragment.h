#pragma once

#include "CkEcsTemplate_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkEcsTemplate_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_EcsTemplate_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_EcsTemplate_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_EcsTemplate_Updated);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECSTEMPLATE_API FFragment_EcsTemplate_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_EcsTemplate_Params);

    public:
        using ParamsType = FCk_Fragment_EcsTemplate_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_EcsTemplate_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECSTEMPLATE_API FFragment_EcsTemplate_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_EcsTemplate_Current);

    public:
        friend class FProcessor_EcsTemplate_Setup;
        friend class FProcessor_EcsTemplate_HandleRequests;
        friend class FProcessor_EcsTemplate_Teardown;
        friend class UCk_Utils_EcsTemplate_UE;

    private:
        // Add your properties here
        int32 _DummyProperty = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECSTEMPLATE_API FFragment_EcsTemplate_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_EcsTemplate_Requests);

    public:
        friend class FProcessor_EcsTemplate_HandleRequests;
        friend class UCk_Utils_EcsTemplate_UE;

    public:
    	using RequestType = std::variant
        <
            FCk_Request_EcsTemplate_ExampleRequest
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

namespace ck { class FProcessor_EcsTemplate_Replicate; }

UCLASS(Blueprintable)
class CKECSTEMPLATE_API UCk_Fragment_EcsTemplate_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_EcsTemplate_Rep);

public:
    friend class ck::FProcessor_EcsTemplate_Replicate;
};

// --------------------------------------------------------------------------------------------------------------------