#include "CkGroundNav_CookedSourceManifest.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GroundNav_CookedSourceManifest_UE::
    Get_IsCompatibleWith(
        int32 InFormatVersion) const
    -> bool
{
    return _FormatVersion == InFormatVersion;
}

// --------------------------------------------------------------------------------------------------------------------
