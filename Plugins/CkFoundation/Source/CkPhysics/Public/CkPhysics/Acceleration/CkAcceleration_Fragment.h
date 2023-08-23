#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkMacros/CkMacros.h"

#include "CkPhysics/Acceleration/CkAcceleration_Fragment_Params.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FCk_Tag_Acceleration_Setup {};

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FCk_Fragment_Acceleration_Params
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Acceleration_Params);

    public:
        using ParamsType = FCk_Fragment_Acceleration_ParamsData;

    public:
        FCk_Fragment_Acceleration_Params() = default;
        explicit FCk_Fragment_Acceleration_Params(ParamsType InParams);

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FCk_Fragment_Acceleration_Current
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_Acceleration_Current);

    public:
        friend class FCk_Processor_Acceleration_Setup;

    public:
        FCk_Fragment_Acceleration_Current() = default;
        explicit FCk_Fragment_Acceleration_Current(
            FVector InAcceleration);

    private:
        FVector _CurrentAcceleration = FVector::ZeroVector;

    public:
        CK_PROPERTY_GET(_CurrentAcceleration);
    };
}

// --------------------------------------------------------------------------------------------------------------------
