#include "CkGroundNav_CookedTile.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GroundNav_CookedTile_UE::
    Get_IsCompatibleWith(
        int32 InFormatVersion) const
    -> bool
{
    return _FormatVersion == InFormatVersion;
}

// --------------------------------------------------------------------------------------------------------------------
