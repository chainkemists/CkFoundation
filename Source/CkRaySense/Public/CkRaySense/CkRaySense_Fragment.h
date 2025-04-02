#pragma once

#include "CkRaySense_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkSignal/CkSignal_Macros.h"

#include "CkRaySense_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_RaySense_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_RaySense_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_RaySense_Updated);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_RaySense_Params = FCk_Fragment_RaySense_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRAYSENSE_API FFragment_RaySense_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_RaySense_Current);

    public:
        friend class FProcessor_RaySense_HandleRequests;
        friend class FProcessor_RaySense_Teardown;
        friend class UCk_Utils_RaySense_UE;

    private:
        // Add your properties here
        int32 _DummyProperty = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRAYSENSE_API FFragment_RaySense_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_RaySense_Requests);

    public:
        friend class FProcessor_RaySense_HandleRequests;
        friend class UCk_Utils_RaySense_UE;

    public:
        using RequestType = std::variant
        <
            FCk_Request_RaySense_ExampleRequest
        >;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKRAYSENSE_API, OnRaySenseTraceHit, FCk_Delegate_RaySense_LineTrace_MC, FCk_Handle_RaySense, FCk_RaySense_HitResult);

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_RaySense_Replicate; }

UCLASS(Blueprintable)
class CKRAYSENSE_API UCk_Fragment_RaySense_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_RaySense_Rep);

public:
    friend class ck::FProcessor_RaySense_Replicate;
};

// --------------------------------------------------------------------------------------------------------------------