#pragma once

#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkWorld/Ticker/CkTicker.h"

#include "CkMacros/CkMacros.h"

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class FEcsWorld : public ck::FTicker
    {
        CK_GENERATED_BODY(FEcsWorld);

    private:
        FRegistryType _Registry;

    public:
        CK_PROPERTY_GET(_Registry);
        CK_PROPERTY_GET_NON_CONST(_Registry);
    };

    // --------------------------------------------------------------------------------------------------------------------
}
