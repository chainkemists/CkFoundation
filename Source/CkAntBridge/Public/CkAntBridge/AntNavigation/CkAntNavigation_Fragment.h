#pragma once

#include "CkAntNavigation_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkAntNavigation_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_AntNavigation_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_AntNavigation_Updated);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANTBRIDGE_API FFragment_AntNavigation_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_AntNavigation_Params);

    public:
        using ParamsType = FCk_Fragment_AntNavigation_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_AntNavigation_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANTBRIDGE_API FFragment_AntNavigation_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_AntNavigation_Current);

    public:
        friend class FProcessor_AntNavigation_HandleRequests;
        friend class UCk_Utils_AntNavigation_UE;

    private:
        // Add your properties here
        int32 _DummyProperty = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKANTBRIDGE_API FFragment_AntNavigation_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_AntNavigation_Requests);

    public:
        friend class FProcessor_AntNavigation_HandleRequests;
        friend class UCk_Utils_AntNavigation_UE;

    private:
        // Add your requests here
        int32 _DummyProperty = 0;
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_AntNavigation_Replicate; }

UCLASS(Blueprintable)
class CKANTBRIDGE_API UCk_Fragment_AntNavigation_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_AntNavigation_Rep);

public:
    friend class ck::FProcessor_AntNavigation_Replicate;
};

// --------------------------------------------------------------------------------------------------------------------
