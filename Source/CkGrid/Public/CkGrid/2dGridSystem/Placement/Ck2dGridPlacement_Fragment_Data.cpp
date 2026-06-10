#include "Ck2dGridPlacement_Fragment_Data.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

#include "Serialization/Archive.h"

// --------------------------------------------------------------------------------------------------------------------
// Tier-B (used directly as ck::FFragment_2dGridPlacement_Params).

CK_REGISTER_SNAPSHOTABLE(FCk_Fragment_2dGridPlacement_ParamsData);

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Fragment_2dGridPlacement_ParamsData::
    SerializeSnapshot(
        FArchive& InAr,
        ck::FSnapshotContext& InCtx)
    -> void
{
    InCtx.Snapshot_Handle(InAr, _Occupant);
    InCtx.Snapshot_Handle(InAr, _Grid);

    auto RotationByte = uint8{0};
    if (InAr.IsSaving())
    { RotationByte = static_cast<uint8>(_Rotation); }

    InAr << _Anchor;
    InAr << RotationByte;
    InAr << _Cells;

    if (InAr.IsLoading())
    { _Rotation = static_cast<ECk_CardinalRotation>(RotationByte); }
}

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
