#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkMacros/CkMacros.h"

#include "CkPhysics/Velocity/CkVelocity_Fragment_Params.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FCk_Tag_Velocity_Setup {};

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FCk_Fragment_Velocity_Params
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Velocity_Params);

    public:
        using ParamsType = FCk_Fragment_Velocity_ParamsData;

    public:
        FCk_Fragment_Velocity_Params() = default;
        explicit FCk_Fragment_Velocity_Params(ParamsType InParams);

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FCk_Fragment_Velocity_Current
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Velocity_Current);

    public:
        friend class FCk_Processor_Velocity_Setup;

    public:
        FCk_Fragment_Velocity_Current() = default;
        explicit FCk_Fragment_Velocity_Current(
            FVector InVelocity);

    private:
        FVector _CurrentVelocity = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_CurrentVelocity);
    };
}

// --------------------------------------------------------------------------------------------------------------------
