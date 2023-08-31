#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkMacros/CkMacros.h"

#include "CkPhysics/Integrator/CkIntegrator_Fragment_Params.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FCk_Tag_Integrator_Setup {};
    struct FCk_Tag_Integrator_Update {};

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FCk_Fragment_Integrator_Current
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Integrator_Current);

    public:
        friend class FCk_Processor_Integrator_Setup;
        friend class FCk_Processor_Integrator_Update;

    private:
        FVector _DistanceOffset = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_DistanceOffset);
    };
}

// --------------------------------------------------------------------------------------------------------------------
