#pragma once

#include "CkInventory_Fragment_Data.h"
#include "CkInventory/Item/CkItem_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

#include <variant>

#include "CkInventory_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Inventory_UE;
class UCk_Utils_Inventory_DataOnly_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ---- Tags ----

    CK_DEFINE_ECS_TAG(FTag_Inventory_DataOnly);
    CK_DEFINE_ECS_TAG(FTag_Inventory_Spatial);
    CK_DEFINE_ECS_TAG(FTag_Inventory_MayRequireReplication);
    CK_DEFINE_ECS_TAG(FTag_Inventory_MayHaveChanged);

    // --------------------------------------------------------------------------------------------------------------------

    // ---- Fragments ----

    using FFragment_Inventory_Params = FCk_Fragment_Inventory_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINVENTORY_API FFragment_Inventory_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Inventory_Requests);

    public:
        friend class FProcessor_Inventory_HandleRequests;
        friend class UCk_Utils_Inventory_UE;
        friend class UCk_Utils_Inventory_DataOnly_UE;

        using AddItemRequestType              = FCk_Request_Inventory_AddItem;
        using RemoveItemRequestType           = FCk_Request_Inventory_RemoveItem;
        using StackItemsRequestType           = FCk_Request_Inventory_StackItems;
        using SplitStackRequestType           = FCk_Request_Inventory_SplitStack;
        using AddItemByDefinitionRequestType  = FCk_Request_Inventory_AddItemByDefinition;
        using TransferItemRequestType        = FCk_Request_Inventory_TransferItem;
        using SortRequestType                = FCk_Request_Inventory_Sort;

        using RequestType = std::variant<AddItemRequestType, RemoveItemRequestType,
                                          StackItemsRequestType, SplitStackRequestType,
                                          AddItemByDefinitionRequestType, TransferItemRequestType,
                                          SortRequestType>;
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

    // --------------------------------------------------------------------------------------------------------------------

    struct CKINVENTORY_API FFragment_Inventory_PreviousItems
    {
    public:
        CK_GENERATED_BODY(FFragment_Inventory_PreviousItems);
        friend class FProcessor_Inventory_FireSignals;

    private:
        TArray<FCk_Handle_Item> _Items;

    public:
        CK_PROPERTY_GET(_Items);
        CK_DEFINE_CONSTRUCTORS(FFragment_Inventory_PreviousItems, _Items);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // ---- Item → Inventory back-reference (entity holder) ----

    CK_DEFINE_ENTITY_HOLDER_AND_UTILS(TUtils_Item_ParentInventory, FFragment_Item_ParentInventory);

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
        TArray<FCk_Handle_Item>,
        TArray<FCk_Handle_Item>);

    // Per-request fulfillment signals (bound to the request handle entity, auto-unbind after firing)

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINVENTORY_API,
        Inventory_OnOperationResult_Add,
        FCk_Delegate_Inventory_OnOperationResult_Add,
        FCk_Handle_Inventory,
        FCk_Handle_Item,
        ECk_Inventory_OperationResult_Add);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINVENTORY_API,
        Inventory_OnOperationResult_Remove,
        FCk_Delegate_Inventory_OnOperationResult_Remove,
        FCk_Handle_Inventory,
        FCk_Handle_Item,
        ECk_Inventory_OperationResult_Remove);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINVENTORY_API,
        Inventory_OnOperationResult_Stack,
        FCk_Delegate_Inventory_OnOperationResult_Stack,
        FCk_Handle_Inventory,
        FCk_Handle_Item,
        FCk_Handle_Item,
        ECk_Inventory_OperationResult_Stack);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINVENTORY_API,
        Inventory_OnOperationResult_Split,
        FCk_Delegate_Inventory_OnOperationResult_Split,
        FCk_Handle_Inventory,
        FCk_Handle_Item,
        FCk_Handle_Item,
        ECk_Inventory_OperationResult_Split);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINVENTORY_API,
        Inventory_OnOperationResult_AddByDefinition,
        FCk_Delegate_Inventory_OnOperationResult_AddByDefinition,
        FCk_Handle_Inventory,
        ECk_Inventory_OperationResult_AddByDefinition,
        int32,
        TArray<FCk_Handle_Item>);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINVENTORY_API,
        Inventory_OnOperationResult_Transfer,
        FCk_Delegate_Inventory_OnOperationResult_Transfer,
        FCk_Handle_Inventory,
        FCk_Handle_Item,
        FCk_Handle_Inventory,
        int32,
        ECk_Inventory_OperationResult_Transfer);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINVENTORY_API,
        Inventory_OnOperationResult_Sort,
        FCk_Delegate_Inventory_OnOperationResult_Sort,
        FCk_Handle_Inventory,
        ECk_Inventory_OperationResult_Sort);
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
