#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkGrid/2dGridSystem/Placement/Ck2dGridPlacement_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_2dGridOccupancy_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_2dGridOccupancy_StampCells;

    // --------------------------------------------------------------------------------------------------------------------

    // Grid-side record of the placements currently registered on this grid. CkRecord
    // auto-prunes a dead placement (reverse-link) when its entity is destroyed.
    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOf_GridPlacements, FCk_Handle_2dGridPlacement);

    using RecordOf_GridPlacements_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOf_GridPlacements>;

    // --------------------------------------------------------------------------------------------------------------------

    // Authoritative "what is currently stamped on the grid". Reconcile diffs this against
    // the desired set computed from the placement record.
    struct CKGRID_API FFragment_2dGridOccupancy_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_2dGridOccupancy_Current);

    public:
        friend class FProcessor_2dGridOccupancy_StampCells;
        friend class ::UCk_Utils_2dGridOccupancy_UE;

    private:
        TMap<FIntPoint, FCk_Handle_2dGridPlacement> _StampedCells;

    public:
        CK_PROPERTY_GET(_StampedCells);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Back-reference added to an OCCUPANT entity so its death-watch can find and destroy
    // the placement it owns (capture-free, member-handler friendly).
    struct CKGRID_API FFragment_2dGridOccupant_PlacementRef
    {
    public:
        CK_GENERATED_BODY(FFragment_2dGridOccupant_PlacementRef);

    public:
        friend class ::UCk_Utils_2dGridOccupancy_UE;

    private:
        FCk_Handle_2dGridPlacement _Placement;

    public:
        CK_PROPERTY_GET(_Placement);
    };

}

// --------------------------------------------------------------------------------------------------------------------
