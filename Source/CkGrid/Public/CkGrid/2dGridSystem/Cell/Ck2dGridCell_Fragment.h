#pragma once

#include "Ck2dGridCell_Fragment_Data.h"

#include "CkGrid/2dGridSystem/Placement/Ck2dGridPlacement_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_2dGridCell_UE;
class UCk_Utils_2dGridOccupancy_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_2dGridOccupancy_StampCells;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG_COUNTED(FTag_2dGridCell_Disabled);

    // Plain (NOT counted): occupancy is exclusive — validation guarantees <=1 occupant per cell.
    CK_DEFINE_ECS_TAG(FTag_2dGridCell_Occupied);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_2dGridCell_Params = FCk_Fragment_2dGridCell_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    // Set on a cell while it is stamped by a placement; cleared on reconcile when the
    // placement releases the cell. Mirrors the authoritative occupancy state.
    struct CKGRID_API FFragment_2dGridCell_Occupancy
    {
    public:
        CK_GENERATED_BODY(FFragment_2dGridCell_Occupancy);

    public:
        friend class FProcessor_2dGridOccupancy_StampCells;
        friend class ::UCk_Utils_2dGridOccupancy_UE;

    private:
        FCk_Handle_2dGridPlacement _Placement;

    public:
        CK_PROPERTY_GET(_Placement);
    };
}

// --------------------------------------------------------------------------------------------------------------------