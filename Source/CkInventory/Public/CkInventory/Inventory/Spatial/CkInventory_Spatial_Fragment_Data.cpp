#include "CkInventory_Spatial_Fragment_Data.h"

#include "CkSnapshot/Context/CkSnapshot_Context.h"
#include "CkSnapshot/Context/CkSnapshot_FragmentRegistry.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Writer.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Reader.h"

#include "Serialization/Archive.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_SNAPSHOTABLE(FCk_Fragment_Inventory_Spatial_ParamsData);
CK_REGISTER_SNAPSHOTABLE(FCk_InventoryItem_Spatial_ReplicatedEntry);

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_InventoryItem_Spatial_ReplicatedEntry::
    SerializeSnapshot(
        FArchive& InAr,
        ck::FSnapshotContext& InCtx)
    -> void
{
    InCtx.Snapshot_Handle(InAr, _ItemHandle);

    InAr << _Coordinate;

    auto RotationRaw = static_cast<uint8>(_Rotation);
    InAr << RotationRaw;
    _Rotation = static_cast<ECk_CardinalRotation>(RotationRaw);
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_Inventory_Spatial_ParamsData::
FCk_Fragment_Inventory_Spatial_ParamsData(
    FGameplayTag InName, FIntPoint InDimensions)
    : _Name(InName)
    , _Dimensions(InDimensions)
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_InventoryItem_Spatial_ReplicatedEntry::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return _ItemHandle == InOther._ItemHandle
        && _Coordinate == InOther._Coordinate
        && _Rotation == InOther._Rotation;
}
