#pragma once

#include "CkInventory_Fragment_Data.h"
#include "CkInventory/Item/CkInventoryItem_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

#include <variant>

#include "CkInventory_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Inventory_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ---- Tags ----

    CK_DEFINE_ECS_TAG(FTag_Inventory_DataOnly);
    CK_DEFINE_ECS_TAG(FTag_Inventory_Spatial);
    CK_DEFINE_ECS_TAG(FTag_Inventory_MayRequireReplication);

    // --------------------------------------------------------------------------------------------------------------------

    // ---- Fragments ----

    struct CKINVENTORY_API FFragment_Inventory_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Inventory_Params);

    public:
        using ParamsType = FCk_Fragment_Inventory_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Inventory_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINVENTORY_API FFragment_Inventory_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Inventory_Requests);

    public:
        friend class FProcessor_Inventory_ProcessSlots;
        friend class UCk_Utils_Inventory_UE;

        using AddItemRequestType    = FCk_Request_Inventory_AddItem;
        using RemoveItemRequestType = FCk_Request_Inventory_RemoveItem;

        using RequestType = std::variant<AddItemRequestType, RemoveItemRequestType>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINVENTORY_API FFragment_Inventory_SyncReplication
    {
    public:
        CK_GENERATED_BODY(FFragment_Inventory_SyncReplication);

    private:
        TArray<FCk_InventoryItem_ReplicatedEntry> _ItemsToReplicate;
        TArray<FCk_InventoryItem_ReplicatedEntry> _ItemsToReplicate_Previous;

    public:
        CK_PROPERTY_GET(_ItemsToReplicate);
        CK_PROPERTY_GET(_ItemsToReplicate_Previous);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Inventory_SyncReplication, _ItemsToReplicate, _ItemsToReplicate_Previous);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // ---- Records ----

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfInventories, FCk_Handle_Inventory);
    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfInventoryItems, FCk_Handle_Item);

    // --------------------------------------------------------------------------------------------------------------------

    // ---- Signal ----

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINVENTORY_API,
        Inventory_OnItemsChanged,
        FCk_Delegate_Inventory_OnItemsChanged,
        FCk_Handle_Inventory,
        TArray<FCk_Handle>,
        TArray<FCk_Handle>);
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKINVENTORY_API FCk_RepData_InventoryItems
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_InventoryItems);

    UPROPERTY()
    TArray<FCk_InventoryItem_ReplicatedEntry> Items;
};

namespace ck
{
    using FFragment_ContainerRef_InventoryItems = TFragment_ContainerEntryRef<FCk_RepData_InventoryItems>;
}

// --------------------------------------------------------------------------------------------------------------------
