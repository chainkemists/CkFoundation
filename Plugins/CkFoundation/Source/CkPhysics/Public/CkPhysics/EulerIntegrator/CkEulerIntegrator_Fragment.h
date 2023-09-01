#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkMacros/CkMacros.h"

#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Fragment_Params.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FCk_Tag_EulerIntegrator_Setup {};
    struct FCk_Tag_EulerIntegrator_Update {};

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FCk_Fragment_EulerIntegrator_Current
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_EulerIntegrator_Current);

    public:
        friend class FCk_Processor_EulerIntegrator_Setup;
        friend class FCk_Processor_EulerIntegrator_Update;

    private:
        FVector _DistanceOffset = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_DistanceOffset);
    };
}

// --------------------------------------------------------------------------------------------------------------------
