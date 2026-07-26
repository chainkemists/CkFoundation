#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Per-agent identity colour shared by every visualisation. Opt-in via Set_DebugColor — Get_DebugColor
// falls back to a stable hash-derived colour for agents that never opted in.
// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_CrowdAgent_UE;

namespace ck
{
    struct CKCROWD_API FFragment_CrowdAgent_DebugColor
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_DebugColor);
        friend class ::UCk_Utils_CrowdAgent_UE;

    private:
        FLinearColor _Color = FLinearColor::White;

    public:
        CK_PROPERTY_GET(_Color);
    };
}

// --------------------------------------------------------------------------------------------------------------------
