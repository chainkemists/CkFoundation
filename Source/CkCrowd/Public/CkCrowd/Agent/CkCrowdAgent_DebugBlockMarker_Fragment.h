#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment_Data.h"

#include "CkPmg/CkPmg_Fragment_Data_DebugShapes.h"

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// FCk_Handle_Pmg_DebugShape is a stored member, so the CkPmg header above cannot be a forward decl.

namespace ck
{
    // Handle to the PMG floor disc backing the block-status visualization, plus the change-gates
    // that keep the per-tick update from re-pushing requests the shape already has.
    //
    // Created LAZILY — only for an agent that has actually been blocked at least once — and then
    // KEPT and toggled, never recreated. `BlockedRecheck` resumes a held agent on a ~1s cadence, so
    // a create/destroy-per-episode marker would churn a mesh entity every second per agent in a
    // jammed crowd; visibility toggling is what the sibling DrawBody visualization does for the
    // same reason. The disc is SceneNode-parented to the agent with PMG `InDuration = -1.0f`, so it
    // follows the agent and cascades on its destruction; `_EndPlay` additionally destroys it
    // explicitly, because a PMG shape is an entity and is NOT reclaimed by merely dropping the
    // handle.
    struct CKCROWD_API FFragment_CrowdAgent_DebugBlockMarker
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_DebugBlockMarker);
        friend class FProcessor_CrowdAgent_DrawBlockStatus_Setup;
        friend class FProcessor_CrowdAgent_DrawBlockStatus_Update;
        friend class FProcessor_CrowdAgent_DrawBlockStatus_EndPlay;

    private:
        FCk_Handle_Pmg_DebugShape _DiscHandle{};

        FLinearColor _LastAppliedColor   = FLinearColor::White;
        bool         _LastAppliedVisible = false;

        // Re-colouring is driven off the CAUSE rather than the resolved colour so a cause change
        // that happens to map to the same colour still costs nothing, and so the compare is against
        // the thing that actually varies.
        ECk_CrowdAgent_BlockedReason _LastAppliedCause = ECk_CrowdAgent_BlockedReason::GoalOccupied;

    public:
        CK_PROPERTY_GET(_DiscHandle);
        CK_PROPERTY_GET(_LastAppliedColor);
        CK_PROPERTY_GET(_LastAppliedVisible);
        CK_PROPERTY_GET(_LastAppliedCause);
    };
}

// --------------------------------------------------------------------------------------------------------------------
