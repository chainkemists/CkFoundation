#pragma once

#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkWorld/Ticker/CkTicker.h"

#include "CkMacros/CkMacros.h"

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class World : public ck::FTicker
    {
        CK_GENERATED_BODY(World);

    private:
        FRegistryType _Registry;

    public:
        CK_PROPERTY_GET(_Registry);
    };

    // --------------------------------------------------------------------------------------------------------------------
}
