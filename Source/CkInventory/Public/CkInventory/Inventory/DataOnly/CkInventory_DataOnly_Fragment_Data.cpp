#include "CkInventory_DataOnly_Fragment_Data.h"

#include "CkSnapshot/Context/CkSnapshot_FragmentRegistry.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Writer.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_SNAPSHOTABLE(FCk_Fragment_Inventory_DataOnly_ParamsData);

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_Inventory_DataOnly_ParamsData::
FCk_Fragment_Inventory_DataOnly_ParamsData(
    FGameplayTag InName, TOptional<int32> InBoundLimit)
    : _Name(InName)
{
    if (InBoundLimit.IsSet())
    {
        _BoundMode = ECk_Inventory_DataOnly_BoundMode::Bounded;
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
