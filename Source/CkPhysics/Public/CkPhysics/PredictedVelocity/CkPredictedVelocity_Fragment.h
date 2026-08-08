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

    using FFragment_PredictedVelocity_Params = FCk_PredictedVelocity_Spec;

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