#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkPhysics/AutoReorient/CkAutoReorient_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FTag_AutoReorient_OrientTowardsVelocity {};

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPHYSICS_API FFragment_AutoReorient_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_AutoReorient_Params);

    public:
        using ParamsType = FCk_Fragment_AutoReorient_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_AutoReorient_Params, _Params);
    };
}

// --------------------------------------------------------------------------------------------------------------------