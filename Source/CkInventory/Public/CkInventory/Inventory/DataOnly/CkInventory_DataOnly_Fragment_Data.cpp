#include "CkInventory_DataOnly_Fragment_Data.h"

#include "CkEcs/Snapshot/CkSnapshot_Context.h"
#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

#include "Serialization/Archive.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_SNAPSHOTABLE(FCk_Fragment_Inventory_DataOnly_ParamsData);
CK_REGISTER_SNAPSHOTABLE(FCk_InventoryItem_DataOnly_ReplicatedEntry);

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_InventoryItem_DataOnly_ReplicatedEntry::
    SerializeSnapshot(
        FArchive& InAr,
        ck::FSnapshotContext& InCtx)
    -> void
{
    InCtx.Snapshot_Handle(InAr, _ItemHandle);
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_Inventory_DataOnly_ParamsData::
FCk_Fragment_Inventory_DataOnly_ParamsData(
    FGameplayTag InName, TOptional<int32> InBoundLimit, ECk_Inventory_DataOnly_BoundMode InBoundMode)
    : _Name(InName)
{
    if (InBoundLimit.IsSet())
    {
        _BoundMode = (InBoundMode == ECk_Inventory_DataOnly_BoundMode::Unbounded)
            ? ECk_Inventory_DataOnly_BoundMode::BoundedByUniqueEntries
            : InBoundMode;
        _BoundLimit = InBoundLimit.GetValue();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto 
    FCk_InventoryItem_DataOnly_ReplicatedEntry::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return _ItemHandle == InOther._ItemHandle;
}
