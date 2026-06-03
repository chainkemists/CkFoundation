#pragma once

#include "Ck2dGridPlacement_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_2dGridPlacement_Params = FCk_Fragment_2dGridPlacement_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKGRID_API,
        2dGridPlacement_ObjectPlaced,
        FCk_Delegate_2dGridPlacement_ObjectPlaced,
        FCk_Handle_2dGridSystem,
        FCk_Handle,
        TArray<FIntPoint>);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKGRID_API,
        2dGridPlacement_ObjectRemoved,
        FCk_Delegate_2dGridPlacement_ObjectRemoved,
        FCk_Handle_2dGridSystem,
        FCk_Handle);
}

// --------------------------------------------------------------------------------------------------------------------
