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

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Inventory_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Inventory_MassTransfer_Churn;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_Inventory_DataOnly);
    CK_DEFINE_ECS_TAG(FTag_Inventory_Spatial);
    CK_DEFINE_ECS_TAG(FTag_Inventory_MayRequireReplication);
    CK_DEFINE_ECS_TAG(FTag_Inventory_MayHaveChanged);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Inventory_Params = FCk_Fragment_Inventory_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct FCk_EmptyAddon {};

    template <typename TBaseRequest, typename TAddon = FCk_EmptyAddon>
    struct TInventory_RequestEntry
    {
        using BaseRequestType = TBaseRequest;
        using AddonType       = TAddon;

        TBaseRequest Common;
        TAddon Addon{};

        TInventory_RequestEntry() = default;

        explicit TInventory_RequestEntry(TBaseRequest InCommon)
            : Common(MoveTemp(InCommon)) {}

        TInventory_RequestEntry(TBaseRequest InCommon, TAddon InAddon)
            : Common(MoveTemp(InCommon)), Addon(MoveTemp(InAddon)) {}
    };

    // Primary template — explicit specializations live in each shape's fragment header
    // (CkInventory_Spatial_Fragment.h / CkInventory_DataOnly_Fragment.h). The base Utils
    // dispatch helper resolves TFragment_Inventory_Requests<TShape> when both shape headers
    // are visible at the point of use (which they are via CkInventory_Utils.h's includes).
    template <typename TShape>
    struct TFragment_Inventory_Requests;

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

    // Item → Inventory back-reference.
    CK_DEFINE_ENTITY_HOLDER_AND_UTILS_ROUNDTRIP(TUtils_Item_ParentInventory, FFragment_Item_ParentInventory, FCk_Handle_Inventory);

    // Inventory slot → item reference (spatial grid cells point to items).
    CK_DEFINE_ENTITY_HOLDER_AND_UTILS_ROUNDTRIP(TUtils_InventorySlot_ItemRef, FFragment_InventorySlot_ItemRef, FCk_Handle_Item);

    CK_DEFINE_RECORD_OF_ENTITIES_ROUNDTRIP(FFragment_RecordOfInventories, FCk_Handle_Inventory);
    CK_DEFINE_RECORD_OF_ENTITIES_ROUNDTRIP(FFragment_RecordOfInventoryItems, FCk_Handle_Item);

    // --------------------------------------------------------------------------------------------------------------------

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
        FCk_Handle_Item,
        ECk_Inventory_OperationResult_Transfer);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINVENTORY_API,
        Inventory_OnOperationResult_Sort,
        FCk_Delegate_Inventory_OnOperationResult_Sort,
        FCk_Handle_Inventory,
        ECk_Inventory_OperationResult_Sort);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINVENTORY_API,
        Inventory_OnOperationResult_Relocate,
        FCk_Delegate_Inventory_OnOperationResult_Relocate,
        FCk_Handle_Inventory,
        FCk_Handle_Item,
        FIntPoint,
        ECk_Inventory_OperationResult_Relocate);

    // --------------------------------------------------------------------------------------------------------------------

    // In-flight state for a standalone mass-transfer op. Lives on a transient-owned op entity (a plain
    // FCk_Handle, discriminated by this fragment) — NOT on any inventory. The churn is its friend; the
    // Utils boundary populates it at kickoff. ck::FFragment_PacedWork is added separately by the churn
    // (pacer/budget only).
    struct CKINVENTORY_API FFragment_Inventory_MassTransfer_InFlight
    {
        CK_GENERATED_BODY(FFragment_Inventory_MassTransfer_InFlight);

        friend class FProcessor_Inventory_MassTransfer_Churn;
        friend class ::UCk_Utils_Inventory_UE;

    private:
        // Gathered once at kickoff (committed read); never resized after — _Cursor advances instead,
        // so _Pending.IsEmpty() at finish == "empty at kickoff".
        TArray<FCk_Handle_Item>      _Pending;
        int32                        _Cursor = 0;
        FCk_BestTransferTargetParams _TargetResolution;

        int32 _StepsPerPass     = 1;
        int32 _MaxStepsPerFrame = 1;

        int32 _UnitsMoved      = 0;
        int32 _ItemsFullyMoved = 0;
        int32 _ItemsFailed     = 0;
        // Distinguishes Failed_NoCandidateAccepts (nothing placed) from Success_Partial.
        bool  _AnyUnitMoved    = false;

    public:
        CK_PROPERTY_GET(_StepsPerPass);
        CK_PROPERTY_GET(_MaxStepsPerFrame);
        CK_PROPERTY_GET(_UnitsMoved);
        CK_PROPERTY_GET(_ItemsFullyMoved);
        CK_PROPERTY_GET(_ItemsFailed);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINVENTORY_API,
        Inventory_MassTransfer_OnComplete,
        FCk_Delegate_Inventory_MassTransfer_OnComplete,
        FCk_Handle,
        ECk_Inventory_MassTransfer_Result,
        int32,
        int32,
        int32);
}

// --------------------------------------------------------------------------------------------------------------------
