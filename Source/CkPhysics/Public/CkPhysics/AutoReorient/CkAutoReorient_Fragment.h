#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkPhysics/AutoReorient/CkAutoReorient_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_AutoReorient_OrientTowardsVelocity);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_AutoReorient_Params = FCk_AutoReorient_Spec;
}

// --------------------------------------------------------------------------------------------------------------------