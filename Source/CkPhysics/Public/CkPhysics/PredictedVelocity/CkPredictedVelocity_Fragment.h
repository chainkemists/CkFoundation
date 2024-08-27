#pragma once

#include "CkPredictedVelocity_Fragment_Data.h"

#include "CkCore/Time/CkTime.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_PredictedVelocity_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_PredictedVelocity_Updated);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_PredictedVelocity_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_PredictedVelocity_Params);

    public:
        using ParamsType = FCk_Fragment_PredictedVelocity_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_PredictedVelocity_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_PredictedVelocity_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_PredictedVelocity_Current);

    public:
        friend class FProcessor_PredictedVelocity_Update;
        friend class UCk_Utils_PredictedVelocity_UE;

    private:
        FVector _PreviousLocation = FVector::ZeroVector;
        FVector _CurrentVelocity = FVector::ZeroVector;
        FCk_Time _PreviousDeltaTime = {};

    public:
        CK_PROPERTY_GET(_PreviousLocation);
        CK_PROPERTY_GET(_CurrentVelocity);
        CK_PROPERTY_GET(_PreviousDeltaTime);
    };
}

// --------------------------------------------------------------------------------------------------------------------