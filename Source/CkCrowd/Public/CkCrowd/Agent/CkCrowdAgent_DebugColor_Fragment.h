#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Per-agent identity colour, used by every visualisation (breadcrumb path, planned-path overlay,
// debugger Agent List swatch). Opt-in fragment: only present on agents the gym/debugger
// explicitly stamps via UCk_Utils_CrowdAgent_UE::Set_DebugColor. Production agents that never
// opt in carry zero overhead — Get_DebugColor falls back to a hash-derived colour so untagged
// agents still get a stable distinct colour when a tool happens to render them.
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
