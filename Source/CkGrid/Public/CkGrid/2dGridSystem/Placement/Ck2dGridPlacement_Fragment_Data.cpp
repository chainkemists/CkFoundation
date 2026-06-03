#include "Ck2dGridPlacement_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_2dGridPlacement_ReplicatedEntry::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return _Occupant == InOther._Occupant
        && _Anchor   == InOther._Anchor
        && _Rotation == InOther._Rotation
        && _Cells    == InOther._Cells;
}

// --------------------------------------------------------------------------------------------------------------------
