#pragma once

#include "CkAudio_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkAudio_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Audio_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Audio_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_Audio_Updated);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKAUDIO_API FFragment_Audio_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Audio_Params);

    public:
        using ParamsType = FCk_Fragment_Audio_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Audio_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKAUDIO_API FFragment_Audio_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Audio_Current);

    public:
        friend class FProcessor_Audio_Setup;
        friend class FProcessor_Audio_HandleRequests;
        friend class FProcessor_Audio_Teardown;
        friend class UCk_Utils_Audio_UE;

    private:
        // Add your properties here
        int32 _DummyProperty = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKAUDIO_API FFragment_Audio_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Audio_Requests);

    public:
        friend class FProcessor_Audio_HandleRequests;
        friend class UCk_Utils_Audio_UE;

    public:
        using RequestType = std::variant
        <
            FCk_Request_Audio_ExampleRequest
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

namespace ck { class FProcessor_Audio_Replicate; }

UCLASS(Blueprintable)
class CKAUDIO_API UCk_Fragment_Audio_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_Audio_Rep);

public:
    friend class ck::FProcessor_Audio_Replicate;
};

// --------------------------------------------------------------------------------------------------------------------