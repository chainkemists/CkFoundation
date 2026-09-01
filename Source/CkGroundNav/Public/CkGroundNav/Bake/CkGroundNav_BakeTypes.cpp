#include "CkGroundNav_BakeTypes.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_GroundNav_BakeConfig::
    Get_IsValid() const
    -> bool
{
    return _CellSizeUu > 0.0f &&
           _CellHeightUu > 0.0f &&
           _TileSizeUu >= _CellSizeUu &&
           _MaxColumnsPerTile > 0 &&
           FMath::IsFinite(_CellSizeUu) &&
           FMath::IsFinite(_CellHeightUu) &&
           FMath::IsFinite(_TileSizeUu);
}

// --------------------------------------------------------------------------------------------------------------------
