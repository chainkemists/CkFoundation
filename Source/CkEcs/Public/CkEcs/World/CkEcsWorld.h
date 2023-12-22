#pragma once

#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Ticker/CkTicker.h"

#include "CkCore/Macros/CkMacros.h"

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FEcsWorld : public ck::FTicker
    {
        CK_GENERATED_BODY(FEcsWorld);

    private:
        FRegistryType _Registry;

    public:
        CK_PROPERTY_GET(_Registry);
        CK_PROPERTY_GET_NON_CONST(_Registry);

        CK_DEFINE_CONSTRUCTORS(FEcsWorld, _Registry);
    };

    // --------------------------------------------------------------------------------------------------------------------
}
