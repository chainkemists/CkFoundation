#include "Ck2dGridSystem_Spec.h"

#include "CkCore/Enums/CkEnums.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_2dGridSystem_Spec::
    Resolve_GridParams() const
    -> FCk_Fragment_2dGridSystem_ParamsData
{
    auto Params = FCk_Fragment_2dGridSystem_ParamsData{Dimensions, CellSize};
    Params.Set_DefaultCellState(ECk_EnableDisable::Enable);
    Params.Set_ExceptionCoordinates(DisabledCells);
    Params.Set_DefaultCellTags(DefaultCellTags);

    return Params;
}

// --------------------------------------------------------------------------------------------------------------------
