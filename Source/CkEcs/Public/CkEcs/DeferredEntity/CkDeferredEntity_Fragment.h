#pragma once

#include "CkDeferredEntity_Fragment_Data.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_DeferredEntity_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG_COUNTED(FTag_DeferredEntity);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKECS_API,
        OnDeferredEntitySetupComplete,
        FCk_Delegate_DeferredEntity_OnComplete_MC,
        FCk_Handle_DeferredEntity);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKECS_API,
        OnDeferredEntityFullyComplete,
        FCk_Delegate_DeferredEntity_OnFullyComplete_MC,
        FCk_Handle_DeferredEntity);
}

// --------------------------------------------------------------------------------------------------------------------