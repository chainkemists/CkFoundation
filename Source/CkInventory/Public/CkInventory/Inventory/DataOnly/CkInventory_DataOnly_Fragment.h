#pragma once

#include "CkInventory_DataOnly_Fragment_Data.h"
#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_RequestTraits.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

class UCk_Utils_Inventory_DataOnly_UE;

namespace ck
{
    using FFragment_Inventory_DataOnly_Params = FCk_Fragment_Inventory_DataOnly_ParamsData;

    template <>
    struct CKINVENTORY_API TFragment_Inventory_Requests<FCk_Handle_Inventory_DataOnly>
    {
    public:
        CK_GENERATED_BODY(TFragment_Inventory_Requests<FCk_Handle_Inventory_DataOnly>);

    public:
        friend class UCk_Utils_Inventory_UE;
        friend class UCk_Utils_Inventory_DataOnly_UE;

        using Traits = TInventoryRequestTraits<FCk_Handle_Inventory_DataOnly>;

        using AddItemEntry                = typename Traits::AddItem::Entry;
        using RemoveItemEntry             = typename Traits::RemoveItem::Entry;
        using StackItemsEntry             = typename Traits::StackItems::Entry;
        using SplitStackEntry             = typename Traits::SplitStack::Entry;
        using AddItemByDefinitionEntry    = typename Traits::AddByDefinition::Entry;
        using SortEntry                   = typename Traits::Sort::Entry;
        using TransferItemToSpatialEntry  = typename Traits::Transfer_ToSpatial::Entry;
        using TransferItemToDataOnlyEntry = typename Traits::Transfer_ToDataOnly::Entry;

        using RequestType = typename Traits::Variant;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    using FFragment_Inventory_DataOnly_Requests = TFragment_Inventory_Requests<FCk_Handle_Inventory_DataOnly>;

    // Per-feature restore-replication done marker. ck::FTag_Snapshot_JustRestored lives on the OWNER
    // entity and is shared by every owner-resident feature, so no single feature may remove it —
    // each feature pairs it with its own done tag instead. Never leaks across restores: a restore
    // rebuilds entities from snapshot bytes and this tag is not snapshotted.
    CK_DEFINE_ECS_TAG(FTag_Inventory_DataOnly_RestoreReplicated);

    struct CKINVENTORY_API FFragment_Inventory_DataOnly_SyncReplication
    {
    public:
        CK_GENERATED_BODY(FFragment_Inventory_DataOnly_SyncReplication);

    private:
        TArray<FCk_InventoryItem_DataOnly_ReplicatedEntry> _ItemsToReplicate;
        TArray<FCk_InventoryItem_DataOnly_ReplicatedEntry> _ItemsToReplicate_Previous;

    public:
        CK_PROPERTY_GET(_ItemsToReplicate);
        CK_PROPERTY_GET(_ItemsToReplicate_Previous);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Inventory_DataOnly_SyncReplication, _ItemsToReplicate, _ItemsToReplicate_Previous);
    };

    using FFragment_ContainerRef_Inventory_DataOnly_Items = TFragment_ContainerEntryRef<FCk_RepData_Inventory_DataOnly_Items>;
}
