#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_EulerIntegrator_DoOnePredictiveUpdate);
    CK_DEFINE_ECS_TAG(FTag_EulerIntegrator_NeedsUpdate);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_EulerIntegrator_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_EulerIntegrator_Current);

    public:
        friend class FProcessor_EulerIntegrator_DoOnePredictiveUpdate;
        friend class FProcessor_EulerIntegrator_Update;

    private:
        FVector _DistanceOffset = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_DistanceOffset);
    };
}

// --------------------------------------------------------------------------------------------------------------------
