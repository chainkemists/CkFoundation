#include "CkGroundNav_CookedTile.h"

#include <Misc/Crc.h>

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

namespace ck::groundnav
{
    auto
        Get_CookedTileContentHash(
            TConstArrayView<uint8> InBlob) -> uint32
    {
        return InBlob.IsEmpty() ? 0 : FCrc::MemCrc32(InBlob.GetData(), InBlob.Num());
    }
}

// --------------------------------------------------------------------------------------------------------------------
