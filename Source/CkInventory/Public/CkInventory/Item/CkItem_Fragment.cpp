#include "CkItem_Fragment.h"

#include "CkInventory/Item/CkItem_Definition.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

#include "Serialization/Archive.h"

// --------------------------------------------------------------------------------------------------------------------
// ck:: hoisted to an unqualified alias because CK_REGISTER_SNAPSHOTABLE token-pastes the type name.

using FSnap_InventoryItem = ck::FFragment_InventoryItem;
using FSnap_Item_SpatialPlacement = ck::FFragment_Item_SpatialPlacement;
CK_REGISTER_SNAPSHOTABLE(FSnap_InventoryItem);
CK_REGISTER_SNAPSHOTABLE(FSnap_Item_SpatialPlacement);

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FFragment_InventoryItem::
    SerializeSnapshot(
        FArchive& InAr,
        ck::FSnapshotContext& /*InCtx*/)
    -> void
{
    if (InAr.IsSaving())
    {
        auto* Raw = const_cast<UObject*>(static_cast<const UObject*>(_Definition.Get()));
        InAr << Raw;
    }
    else if (InAr.IsLoading())
    {
        UObject* Raw = nullptr;
        InAr << Raw;
        _Definition = Cast<UCk_InventoryItem_Definition>(Raw);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FFragment_Item_SpatialPlacement::
    SerializeSnapshot(
        FArchive& InAr,
        ck::FSnapshotContext& /*InCtx*/)
    -> void
{
    auto RotationByte = uint8{0};

    if (InAr.IsSaving())
    { RotationByte = static_cast<uint8>(_Rotation); }

    InAr << _Anchor;
    InAr << RotationByte;

    if (InAr.IsLoading())
    { _Rotation = static_cast<ECk_CardinalRotation>(RotationByte); }
}

// --------------------------------------------------------------------------------------------------------------------
