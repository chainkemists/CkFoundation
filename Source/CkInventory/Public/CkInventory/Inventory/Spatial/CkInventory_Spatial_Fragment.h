#pragma once

#include "CkInventory_Spatial_Fragment_Data.h"
#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_RequestTraits.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

class UCk_Utils_Inventory_Spatial_UE;

namespace ck
{
    using FFragment_Inventory_Spatial_Params = FCk_Fragment_Inventory_Spatial_ParamsData;

    template <>
    struct CKINVENTORY_API TFragment_Inventory_Requests<FCk_Handle_Inventory_Spatial>
    {
    public:
        CK_GENERATED_BODY(TFragment_Inventory_Requests<FCk_Handle_Inventory_Spatial>);

    public:
        friend class UCk_Utils_Inventory_UE;
        friend class UCk_Utils_Inventory_Spatial_UE;

        using Traits = TInventoryRequestTraits<FCk_Handle_Inventory_Spatial>;

        // Per-operation entry typedefs (kept as aliases so existing call sites that reference
        // FFragment_Inventory_Spatial_Requests::AddItemEntry etc. don't have to change).
        using AddItemEntry                = typename Traits::AddItem::Entry;
        using RemoveItemEntry             = typename Traits::RemoveItem::Entry;
        using StackItemsEntry             = typename Traits::StackItems::Entry;
        using SplitStackEntry             = typename Traits::SplitStack::Entry;
        using AddItemByDefinitionEntry    = typename Traits::AddByDefinition::Entry;
        using SortEntry                   = typename Traits::Sort::Entry;
        using TransferItemToSpatialEntry  = typename Traits::Transfer_ToSpatial::Entry;
        using TransferItemToDataOnlyEntry = typename Traits::Transfer_ToDataOnly::Entry;
        using RelocateItemEntry           = typename Traits::Relocate::Entry;

        using RequestType = typename Traits::Variant;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
        auto Get_RequestsMutable() -> RequestList& { return _Requests; }
    };

    using FFragment_Inventory_Spatial_Requests = TFragment_Inventory_Requests<FCk_Handle_Inventory_Spatial>;

    // Per-feature restore-replication done marker (mirrors FTag_Inventory_DataOnly_RestoreReplicated).
    // ck::FTag_Snapshot_JustRestored is shared by every owner-resident feature, so no single feature
    // may remove it — each feature pairs it with its own done tag instead.
    CK_DEFINE_ECS_TAG(FTag_Inventory_Spatial_RestoreReplicated);

    struct CKINVENTORY_API FFragment_Inventory_Spatial_SyncReplication
    {
    public:
        CK_GENERATED_BODY(FFragment_Inventory_Spatial_SyncReplication);

    private:
        TArray<FCk_InventoryItem_Spatial_ReplicatedEntry> _ItemsToReplicate;
        TArray<FCk_InventoryItem_Spatial_ReplicatedEntry> _ItemsToReplicate_Previous;

    public:
        CK_PROPERTY_GET(_ItemsToReplicate);
        CK_PROPERTY_GET(_ItemsToReplicate_Previous);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Inventory_Spatial_SyncReplication, _ItemsToReplicate, _ItemsToReplicate_Previous);
    };

    using FFragment_ContainerRef_Inventory_Spatial_Items = TFragment_ContainerEntryRef<FCk_RepData_Inventory_Spatial_Items>;
}
