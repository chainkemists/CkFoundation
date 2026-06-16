#include "CkInventory_RequestHandlers.h"

#include "CkInventory/CkInventory_Log.h"
#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_Utils.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_RequestTraits.h"
#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_RequestTraits.h"
#include "CkInventory/Item/CkItem_Definition.h"
#include "CkInventory/Item/CkItem_Utils.h"
#include "CkInventory/ItemTrait/Stackable/CkItemTrait_Stackable.h"
#include "CkInventory/ItemTrait/Stackable/CkItemTrait_Stackable_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Payload/CkPayload.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

namespace ck::inventory_handlers
{
    namespace detail
    {
        auto BuildSortPredicate(const FCk_Request_Inventory_Sort& InRequest)
            -> TFunction<bool(const FCk_Handle_Item&, const FCk_Handle_Item&)>
        {
            const auto& NativePredicate  = InRequest.Get_SortPredicate();
            const auto& DynamicPredicate = InRequest.Get_SortPredicateDynamic();

            if (NOT NativePredicate.IsBound() && NOT DynamicPredicate.IsBound())
            { return {}; }

            return [NativePredicate, DynamicPredicate](const FCk_Handle_Item& A, const FCk_Handle_Item& B) -> bool
            {
                if (NativePredicate.IsBound()) { return NativePredicate.Execute(A, B); }
                auto ABeforeB = false;
                DynamicPredicate.ExecuteIfBound(A, B, ABeforeB);
                return ABeforeB;
            };
        }

        auto MapAddResultToTransfer(ECk_Inventory_OperationResult_Add InAddResult) -> ECk_Inventory_OperationResult_Transfer
        {
            switch (InAddResult)
            {
                case ECk_Inventory_OperationResult_Add::Failed_RejectedByCustomAcceptanceLogic:
                    return ECk_Inventory_OperationResult_Transfer::Failed_RejectedByCustomAcceptanceLogic;
                default:
                    return ECk_Inventory_OperationResult_Transfer::Failed_NoSpaceInTarget;
            }
        }
    }

    using FInventoryItemRecord = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;

    // ============================================================================================
    // DEFAULT IMPLEMENTATIONS
    // Each TXxx::Handle below is the default body. It works for the no-addon, no-shape-divergence
    // case — typically the DataOnly path. Per-shape behavior (Spatial in this codebase) is
    // expressed by full Handle specializations or static-helper specializations in the typed
    // inventory's RequestTraits.cpp.
    // ============================================================================================

    // ---- TAddItem default: validate + bind. Used by addon-less entries (DataOnly).
    template <typename TInventoryHandle, typename TAddon>
    auto TAddItem<TInventoryHandle, TAddon>::Handle(
        TInventoryHandle& InHandle,
        const FFragment_Inventory_Params&,
        const Entry& InEntry) -> Result
    {
        const auto& InRequest = InEntry.Common;
        auto ItemHandle = InRequest.Get_ItemToAdd();
        auto R = Result::Failed_InvalidItem;
        FCk_Handle_Inventory Base = InHandle;

        const auto Guard = ck::MakeRequestResultGuard<UUtils_Signal_Inventory_OnOperationResult_Add>(
            InRequest, [&]{ return MakePayload(Base, ItemHandle, R); });

        if (InRequest.Get_Acceptance() == ECk_AddAcceptance::Validate)
        {
            R = UCk_Utils_Inventory_UE::Get_CanAcceptItem(Base, ItemHandle);
            if (R != Result::Success)
            {
                // Caller-attributable rejection (e.g. Failed_ItemAlreadyInInventory,
                // Failed_RejectedByCustomAcceptanceLogic) — surface through the Result
                // enum, log at Display so the AutoTest framework doesn't escalate the
                // diagnostic to a test failure.
                ck::inventory::Display(TEXT("AddItem: Failed [{}] for item [{}] in inventory [{}]"),
                    R, ItemHandle, InHandle);
                return R;
            }
        }

        UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::Request_Connect(
            Base, ItemHandle, ECk_Record_LabelRequirementPolicy::Optional);
        TUtils_Item_ParentInventory::AddOrReplace(ItemHandle, Base);
        UCk_Utils_EntityLifetime_UE::Request_TransferLifetimeOwner(ItemHandle, Base);
        R = Result::Success;
        return R;
    }

    // ---- TRemoveItem default: validate + unbind. Used by DataOnly.
    template <typename TInventoryHandle, typename TAddon>
    auto TRemoveItem<TInventoryHandle, TAddon>::Handle(
        TInventoryHandle& InHandle,
        const FFragment_Inventory_Params&,
        const Entry& InEntry) -> Result
    {
        const auto& InRequest = InEntry.Common;
        auto ItemHandle = InRequest.Get_ItemToRemove();
        auto R = Result::Failed_InvalidItem;
        FCk_Handle_Inventory Base = InHandle;

        const auto Guard = ck::MakeRequestResultGuard<UUtils_Signal_Inventory_OnOperationResult_Remove>(
            InRequest, [&]{ return MakePayload(Base, ItemHandle, R); });

        if (ck::Is_NOT_Valid(ItemHandle))
        {
            ck::inventory::Warning(TEXT("RemoveItem: Invalid item handle"));
            return R;
        }

        if (NOT FInventoryItemRecord::Get_ContainsEntry(Base, ItemHandle))
        {
            R = Result::Failed_ItemNotInInventory;
            ck::inventory::Warning(TEXT("RemoveItem: Item [{}] not in inventory [{}]"), ItemHandle, InHandle);
            return R;
        }

        UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::Request_Disconnect(Base, ItemHandle);
        TUtils_Item_ParentInventory::AddOrReplace(ItemHandle, FCk_Handle_Inventory());
        R = Result::Success;
        return R;
    }

    // ---- TStackItems: shared body + OnSourceFullyConsumed hook (default no-op).
    template <typename TInventoryHandle, typename TAddon>
    auto TStackItems<TInventoryHandle, TAddon>::OnSourceFullyConsumed(TInventoryHandle&, FCk_Handle_Item&) -> void
    {
    }

    template <typename TInventoryHandle, typename TAddon>
    auto TStackItems<TInventoryHandle, TAddon>::Handle(
        TInventoryHandle& InHandle,
        const FFragment_Inventory_Params&,
        const Entry& InEntry) -> Result
    {
        const auto& InRequest = InEntry.Common;
        auto SourceItem = InRequest.Get_SourceItem();
        auto TargetItem = InRequest.Get_TargetItem();
        auto R = Result::Failed_InvalidSourceItem;
        FCk_Handle_Inventory Base = InHandle;

        const auto Guard = ck::MakeRequestResultGuard<UUtils_Signal_Inventory_OnOperationResult_Stack>(
            InRequest, [&]{ return MakePayload(Base, SourceItem, TargetItem, R); });

        const auto CanStackResult = UCk_Utils_ItemTrait_Stackable_UE::Get_CanStackItems(Base, SourceItem, TargetItem);
        if (CanStackResult != Result::Success)
        {
            // Caller-attributable rejection (full stack, custom rejection, mismatch) — surface
            // through the Result enum, log at Display so the AutoTest framework doesn't escalate
            // the diagnostic to a test failure.
            ck::inventory::Display(TEXT("StackItems: Failed [{}] for source [{}] and target [{}] in inventory [{}]"),
                CanStackResult, SourceItem, TargetItem, InHandle);
            R = CanStackResult;
            return R;
        }

        const auto SourceCount = UCk_Utils_ItemTrait_Stackable_UE::Get_StackCount(SourceItem);
        const auto TargetCount = UCk_Utils_ItemTrait_Stackable_UE::Get_StackCount(TargetItem);
        // Effective max = min(definition max, the inventory's StackingPolicy clamp); MAX_int32 when uncapped.
        const auto MaxTarget   = UCk_Utils_ItemTrait_Stackable_UE::Get_EffectiveMaxStackSize(Base, TargetItem);
        const auto Available   = MaxTarget - TargetCount;

        const auto Requested     = (InRequest.Get_Count() == ck::Inventory::AllAvailableCount)
            ? SourceCount
            : FMath::Min(InRequest.Get_Count(), SourceCount);
        const auto TransferCount = FMath::Min(Requested, Available);

        if (TransferCount <= 0)
        {
            R = Result::Failed_TargetStackFull;
            return R;
        }

        UCk_Utils_ItemTrait_Stackable_UE::Request_AdjustStackCount(TargetItem, TransferCount);

        if (const auto SourceRemaining = SourceCount - TransferCount;
            SourceRemaining <= 0)
        {
            OnSourceFullyConsumed(InHandle, SourceItem);
            UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::Request_Disconnect(Base, SourceItem);
            TUtils_Item_ParentInventory::AddOrReplace(SourceItem, FCk_Handle_Inventory());
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(SourceItem);
        }
        else
        {
            UCk_Utils_ItemTrait_Stackable_UE::Request_AdjustStackCount(SourceItem, -TransferCount);
        }

        R = Result::Success;
        return R;
    }

    // ---- TSplitStack default: validate + create + OnSplit + bind. No placement (DataOnly).
    template <typename TInventoryHandle, typename TAddon>
    auto TSplitStack<TInventoryHandle, TAddon>::Handle(
        TInventoryHandle& InHandle,
        const FFragment_Inventory_Params&,
        const Entry& InEntry) -> Result
    {
        const auto& InRequest = InEntry.Common;
        auto SourceItem = InRequest.Get_SourceItem();
        auto NewItem    = FCk_Handle_Item{};
        auto R          = Result::Failed_InvalidSourceItem;
        FCk_Handle_Inventory Base = InHandle;

        const auto Guard = ck::MakeRequestResultGuard<UUtils_Signal_Inventory_OnOperationResult_Split>(
            InRequest, [&]{ return MakePayload(Base, SourceItem, NewItem, R); });

        if (ck::Is_NOT_Valid(SourceItem))
        {
            ck::inventory::Warning(TEXT("SplitStack: Invalid source item handle"));
            return R;
        }

        if (NOT FInventoryItemRecord::Get_ContainsEntry(Base, SourceItem))
        {
            R = Result::Failed_SourceNotInInventory;
            ck::inventory::Warning(TEXT("SplitStack: Source [{}] not in inventory [{}]"), SourceItem, InHandle);
            return R;
        }

        if (NOT UCk_Utils_ItemTrait_Stackable_UE::Get_IsStackable(SourceItem))
        {
            R = Result::Failed_ItemNotStackable;
            ck::inventory::Warning(TEXT("SplitStack: Source [{}] is not stackable"), SourceItem);
            return R;
        }

        const auto CurrentCount = UCk_Utils_ItemTrait_Stackable_UE::Get_StackCount(SourceItem);
        const auto SplitCount   = InRequest.Get_SplitCount();

        if (SplitCount < 1 || SplitCount >= CurrentCount)
        {
            R = Result::Failed_InsufficientCount;
            ck::inventory::Warning(TEXT("SplitStack: Invalid split count [{}] for source [{}] with count [{}]"),
                SplitCount, SourceItem, CurrentCount);
            return R;
        }

        // A split mints a NEW entry, so the entry bound must gate it (Spatial's specialization
        // gates via placement instead). A split is unit-neutral — Get_RemainingSlots only
        // constrains under BoundedByUniqueEntries; Unbounded / BoundedByTotalUnits pass.
        if (const auto DataOnlyHandle = UCk_Utils_Inventory_DataOnly_UE::Cast(Base);
            ck::IsValid(DataOnlyHandle)
            && UCk_Utils_Inventory_DataOnly_UE::Get_RemainingSlots(DataOnlyHandle) <= 0)
        {
            R = Result::Failed_NoSpaceForNewItem;
            ck::inventory::Display(TEXT("SplitStack: No entry room for the split-off item in bounded inventory [{}]"),
                InHandle);
            return R;
        }

        // The split-off entry may not exceed the inventory's effective max stack size.
        if (SplitCount > UCk_Utils_ItemTrait_Stackable_UE::Get_EffectiveMaxStackSize(Base, SourceItem))
        {
            R = Result::Failed_NoSpaceForNewItem;
            ck::inventory::Display(TEXT("SplitStack: Split count [{}] exceeds the effective max stack size of inventory [{}]"),
                SplitCount, InHandle);
            return R;
        }

        auto* Definition  = UCk_Utils_Item_UE::Get_Definition(SourceItem);
        auto ContextOwner = UCk_Utils_ContextOwner_UE::Get_ContextOwner(Base);
        NewItem = UCk_Utils_Item_UE::Create(ContextOwner, Definition);

        if (ck::Is_NOT_Valid(NewItem))
        {
            ck::inventory::Warning(TEXT("SplitStack: Failed to create new item from definition"));
            return R;
        }

        Definition->OnSplit(SourceItem, NewItem);

        UCk_Utils_ItemTrait_Stackable_UE::Request_OverrideStackCount(NewItem, SplitCount);
        UCk_Utils_ItemTrait_Stackable_UE::Request_AdjustStackCount(SourceItem, -SplitCount);

        UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::Request_Connect(
            Base, NewItem, ECk_Record_LabelRequirementPolicy::Optional);
        TUtils_Item_ParentInventory::AddOrReplace(NewItem, Base);
        UCk_Utils_EntityLifetime_UE::Request_TransferLifetimeOwner(NewItem, Base);

        R = Result::Success;
        return R;
    }

    // ---- TAddByDefinition: shared body + TryPlace hook (default returns true).
    template <typename TInventoryHandle, typename TAddon>
    auto TAddByDefinition<TInventoryHandle, TAddon>::TryPlace(TInventoryHandle&, FCk_Handle_Item&) -> bool
    {
        return true;
    }

    template <typename TInventoryHandle, typename TAddon>
    auto TAddByDefinition<TInventoryHandle, TAddon>::Handle(
        TInventoryHandle& InHandle,
        const FFragment_Inventory_Params&,
        const Entry& InEntry) -> Result
    {
        const auto& InRequest = InEntry.Common;
        auto R            = Result::Failed_InvalidDefinition;
        auto AmountAdded  = int32{0};
        auto ItemsCreated = TArray<FCk_Handle_Item>{};
        FCk_Handle_Inventory Base = InHandle;

        const auto Guard = ck::MakeRequestResultGuard<UUtils_Signal_Inventory_OnOperationResult_AddByDefinition>(
            InRequest, [&]{ return MakePayload(Base, R, AmountAdded, ItemsCreated); });

        const auto& Definition = InRequest.Get_Definition();
        const auto  Amount     = InRequest.Get_Amount();
        const auto  Policy     = InRequest.Get_Policy();

        if (ck::Is_NOT_Valid(Definition))
        {
            ck::inventory::Warning(TEXT("AddItemByDefinition: Invalid definition"));
            return R;
        }

        if (Amount < 1)
        {
            R = Result::Failed_ZeroAmount;
            ck::inventory::Warning(TEXT("AddItemByDefinition: Amount must be >= 1, got [{}]"), Amount);
            return R;
        }

        R = Result::Failed_NoSpaceAvailable;
        auto Remaining = Amount;
        const auto IsStackable = Definition->template Has_ItemTrait<UCk_ItemTrait_Stackable>();

        // Units budget from committed state (bound metric + per-entry room + custom quota).
        // Our own stack writes inside this handler are deferred (attribute modifiers fold next
        // pump pass), so re-reading capacity mid-loop would double-count room — decrement the
        // budget instead.
        auto UnitsBudget = UCk_Utils_Inventory_UE::Get_AbsorbableUnits(Base, Definition);

        if (IsStackable && Policy == ECk_Inventory_AddPolicy::PreferStacking)
        {
            const auto FillResult = UCk_Utils_ItemTrait_Stackable_UE::Request_FillExistingStacks(
                Base, Definition, FMath::Min(Remaining, UnitsBudget));
            const auto Filled     = FillResult.Get_FilledCount();
            Remaining   -= Filled;
            AmountAdded += Filled;
            UnitsBudget -= Filled;
        }

        auto ContextOwner = UCk_Utils_ContextOwner_UE::Get_ContextOwner(Base);
        while (Remaining > 0)
        {
            if (UnitsBudget <= 0)
            { break; }

            auto NewItem = UCk_Utils_Item_UE::Create(ContextOwner, Definition);
            if (ck::Is_NOT_Valid(NewItem))
            {
                ck::inventory::Warning(TEXT("AddItemByDefinition: Failed to create item from definition"));
                break;
            }

            auto CountForThisItem = int32{1};
            if (IsStackable)
            {
                const auto EffectiveMax = UCk_Utils_ItemTrait_Stackable_UE::Get_EffectiveMaxStackSize(Base, NewItem);
                CountForThisItem        = FMath::Min3(Remaining, EffectiveMax, UnitsBudget);
                UCk_Utils_ItemTrait_Stackable_UE::Request_OverrideStackCount(NewItem, CountForThisItem);
            }

            // Explicit-count acceptance check: the stack write above is still settling (modifier
            // folds next pump pass), so deriving the count from the item would read the trait's
            // initial count instead of CountForThisItem.
            if (const auto AcceptResult = UCk_Utils_Inventory_UE::Get_CanAcceptItem_WithCount(
                    Base, NewItem, ECk_Inventory_AddPolicy::ForceNewItem, CountForThisItem);
                AcceptResult != ECk_Inventory_OperationResult_Add::Success)
            {
                // Map the specific Get_CanAcceptItem failure to the closest AddByDefinition
                // result. Without this mapping, every can-accept failure (including the
                // bounds-full / no-fit cases) collapses to Failed_RejectedByCustomAcceptanceLogic,
                // hiding the real reason from the caller.
                R = (AcceptResult == ECk_Inventory_OperationResult_Add::Failed_RejectedByCustomAcceptanceLogic)
                    ? Result::Failed_RejectedByCustomAcceptanceLogic
                    : Result::Failed_NoSpaceAvailable;
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(NewItem);
                break;
            }

            if (NOT TryPlace(InHandle, NewItem))
            {
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(NewItem);
                break;
            }

            UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::Request_Connect(
                Base, NewItem, ECk_Record_LabelRequirementPolicy::Optional);
            TUtils_Item_ParentInventory::AddOrReplace(NewItem, Base);
            UCk_Utils_EntityLifetime_UE::Request_TransferLifetimeOwner(NewItem, Base);
            ItemsCreated.Add(NewItem);

            Remaining   -= CountForThisItem;
            AmountAdded += CountForThisItem;
            UnitsBudget -= CountForThisItem;
        }

        if (Remaining <= 0)
        { R = Result::Success_AllAdded; }
        else if (AmountAdded > 0)
        { R = Result::Success_PartiallyAdded; }
        else if (R != Result::Failed_RejectedByCustomAcceptanceLogic)
        { R = Result::Failed_NoSpaceAvailable; }

        return R;
    }

    // ---- TSort default: simple record sort. Used by DataOnly.
    template <typename TInventoryHandle, typename TAddon>
    auto TSort<TInventoryHandle, TAddon>::Handle(
        TInventoryHandle& InHandle,
        const FFragment_Inventory_Params&,
        const Entry& InEntry) -> Result
    {
        const auto& InRequest = InEntry.Common;
        auto R = Result::Failed_InvalidInventory;
        FCk_Handle_Inventory Base = InHandle;

        const auto Guard = ck::MakeRequestResultGuard<UUtils_Signal_Inventory_OnOperationResult_Sort>(
            InRequest, [&]{ return MakePayload(Base, R); });

        if (ck::Is_NOT_Valid(InHandle))
        { return R; }

        const auto SortPredicate = detail::BuildSortPredicate(InRequest);
        if (NOT SortPredicate)
        {
            R = Result::Failed_NoPredicate;
            return R;
        }

        FInventoryItemRecord::Sort(Base, [&](const FCk_Handle_Item& A, const FCk_Handle_Item& B)
        { return SortPredicate(A, B); });

        R = Result::Success;
        return R;
    }

    // ---- TRelocate default: not supported (Spatial-only operation; specialize in shape folder).
    template <typename TInventoryHandle, typename TAddon>
    auto TRelocate<TInventoryHandle, TAddon>::Handle(
        TInventoryHandle&,
        const FFragment_Inventory_Params&,
        const Entry&) -> Result
    {
        ck::inventory::Warning(TEXT("RelocateItem: not supported for this inventory shape"));
        return Result::Failed_NotSpatialInventory;
    }

    // ---- TTransfer: cross-shape, single body. Calls SourceTraits/TargetTraits-supplied
    //      primitives via the typed handles' specialized TXxx::Handle (Add, Remove).
    template <typename TSourceHandle, typename TTargetHandle, typename TAddon>
    static auto DoTransfer(
        TSourceHandle& InSource,
        TTargetHandle& InTarget,
        FCk_Handle_Item InSourceItem,
        int32 InCount,
        ECk_Inventory_AddPolicy InPolicy,
        const FCk_SpatialPlacement& InPlacement,
        FCk_Handle_Item& OutNewTargetItem,
        int32& OutCountTransferred) -> ECk_Inventory_OperationResult_Transfer
    {
        FCk_Handle_Inventory BaseSource = InSource;
        FCk_Handle_Inventory BaseTarget = InTarget;

        if (ck::Is_NOT_Valid(InSourceItem))
        { return ECk_Inventory_OperationResult_Transfer::Failed_InvalidSourceItem; }

        if (NOT FInventoryItemRecord::Get_ContainsEntry(BaseSource, InSourceItem))
        { return ECk_Inventory_OperationResult_Transfer::Failed_SourceNotInInventory; }

        if (ck::Is_NOT_Valid(InTarget))
        { return ECk_Inventory_OperationResult_Transfer::Failed_InvalidTargetInventory; }

        if (BaseTarget == BaseSource)
        { return ECk_Inventory_OperationResult_Transfer::Failed_SameInventory; }

        const auto IsStackable = UCk_Utils_ItemTrait_Stackable_UE::Get_IsStackable(InSourceItem);
        auto SourceCount = IsStackable ? UCk_Utils_ItemTrait_Stackable_UE::Get_StackCount(InSourceItem) : 1;
        auto TransferCount = IsStackable
            ? ((InCount == ck::Inventory::AllAvailableCount) ? SourceCount : FMath::Min(InCount, SourceCount))
            : 1;

        if (TransferCount <= 0)
        { return ECk_Inventory_OperationResult_Transfer::Failed_ZeroCount; }

        // Captured once from the pre-fill source count; recomputing it at the return sites from the
        // already-decremented SourceCount + OutCountTransferred would double-count pre-filled units.
        const auto RequestedTotal = (InCount == ck::Inventory::AllAvailableCount) ? SourceCount : InCount;

        const auto* Definition = UCk_Utils_Item_UE::Get_Definition(InSourceItem);

        if (IsStackable)
        {
            // Clamp to what the target can absorb right now (bound metric + per-entry room +
            // custom quota, from committed state). A clamped transfer settles as Success_Partial
            // instead of failing after the pre-fill already moved units.
            TransferCount = FMath::Min(TransferCount,
                UCk_Utils_Inventory_UE::Get_AbsorbableUnits(BaseTarget, Definition));

            if (TransferCount <= 0)
            { return ECk_Inventory_OperationResult_Transfer::Failed_NoSpaceInTarget; }
        }

        // Helper lambdas: synthesize a Remove/Add call by constructing a synthetic request entry
        // and dispatching through the typed Handle. This keeps Transfer's algorithm shape-agnostic
        // while routing through each shape's actual Add/Remove logic (including grid placement).
        const auto DoRemoveFromSource = [&](FCk_Handle_Item ItemToRemove) -> ECk_Inventory_OperationResult_Remove
        {
            using SourceRemoveEntry = typename TInventoryRequestTraits<TSourceHandle>::RemoveItem::Entry;
            FCk_Request_Inventory_RemoveItem Req{ItemToRemove};
            return TInventoryRequestTraits<TSourceHandle>::RemoveItem::Handle(
                InSource, FFragment_Inventory_Params{}, SourceRemoveEntry{Req});
        };

        const auto DoAddToTarget = [&](FCk_Handle_Item ItemToAdd,
            ECk_AddAcceptance InAcceptance = ECk_AddAcceptance::Validate) -> ECk_Inventory_OperationResult_Add
        {
            using TargetAddEntry = typename TInventoryRequestTraits<TTargetHandle>::AddItem::Entry;
            FCk_Request_Inventory_AddItem Req{ItemToAdd};
            Req.Set_Acceptance(InAcceptance);
            // Construct entry: with placement addon if Spatial, else default.
            if constexpr (std::is_same_v<typename TargetAddEntry::AddonType, FCk_SpatialPlacement>)
            {
                return TInventoryRequestTraits<TTargetHandle>::AddItem::Handle(
                    InTarget, FFragment_Inventory_Params{}, TargetAddEntry{Req, InPlacement});
            }
            else
            {
                return TInventoryRequestTraits<TTargetHandle>::AddItem::Handle(
                    InTarget, FFragment_Inventory_Params{}, TargetAddEntry{Req});
            }
        };

        if (IsStackable && InPolicy == ECk_Inventory_AddPolicy::PreferStacking)
        {
            const auto FillResult = UCk_Utils_ItemTrait_Stackable_UE::Request_FillExistingStacks(
                BaseTarget, Definition, TransferCount, InSourceItem);
            const auto Filled = FillResult.Get_FilledCount();

            if (Filled > 0 && ck::IsValid(FillResult.Get_LastFilledItem()))
            { OutNewTargetItem = FillResult.Get_LastFilledItem(); }

            TransferCount       -= Filled;
            OutCountTransferred += Filled;
            SourceCount         -= Filled;

            if (Filled > 0)
            {
                UCk_Utils_ItemTrait_Stackable_UE::Request_AdjustStackCount(InSourceItem, -Filled);

                if (SourceCount <= 0)
                {
                    DoRemoveFromSource(InSourceItem);
                    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InSourceItem);
                }
            }
        }

        if (TransferCount <= 0)
        {
            UCk_Utils_Inventory_UE::Request_MarkInventory_AsMayHaveChanged(BaseTarget);
            return (OutCountTransferred >= RequestedTotal)
                ? ECk_Inventory_OperationResult_Transfer::Success
                : ECk_Inventory_OperationResult_Transfer::Success_Partial;
        }

        // The remainder lands as ONE entry in the target (whole item or split-off item), which may
        // not exceed the target's effective max stack size (StackingPolicy clamp).
        const auto MaxPerNewEntry = IsStackable
            ? UCk_Utils_ItemTrait_Stackable_UE::Get_EffectiveMaxStackSize_ByDefinition(BaseTarget, Definition)
            : MAX_int32;
        const auto MoveCount = FMath::Min(TransferCount, MaxPerNewEntry);

        if (MoveCount >= SourceCount)
        {
            DoRemoveFromSource(InSourceItem);

            const auto AddResult = DoAddToTarget(InSourceItem);

            if (AddResult != ECk_Inventory_OperationResult_Add::Success)
            {
                // Rollback into source — re-add. Always uses the source's AddItem.
                using SourceAddEntry = typename TInventoryRequestTraits<TSourceHandle>::AddItem::Entry;
                FCk_Request_Inventory_AddItem RollbackReq{InSourceItem};
                ECk_Inventory_OperationResult_Add Rollback;
                if constexpr (std::is_same_v<typename SourceAddEntry::AddonType, FCk_SpatialPlacement>)
                {
                    Rollback = TInventoryRequestTraits<TSourceHandle>::AddItem::Handle(
                        InSource, FFragment_Inventory_Params{}, SourceAddEntry{RollbackReq, FCk_SpatialPlacement{}});
                }
                else
                {
                    Rollback = TInventoryRequestTraits<TSourceHandle>::AddItem::Handle(
                        InSource, FFragment_Inventory_Params{}, SourceAddEntry{RollbackReq});
                }
                CK_ENSURE_IF_NOT(Rollback == ECk_Inventory_OperationResult_Add::Success,
                    TEXT("TransferItem: Rollback re-add to source [{}] failed with [{}]"), InSource, Rollback)
                {}
                ck::inventory::Warning(TEXT("TransferItem: Target [{}] refused add of [{}] ({}); rolled back source remove"),
                    InTarget, InSourceItem, AddResult);
                return detail::MapAddResultToTransfer(AddResult);
            }

            OutNewTargetItem = InSourceItem;
            OutCountTransferred += MoveCount;
            UCk_Utils_Inventory_UE::Request_MarkInventory_AsMayHaveChanged(BaseTarget);
        }
        else
        {
            // The split copy below inherits the source's runtime tags via a DEFERRED OnSplit
            // request, so the synchronous CATEGORICAL acceptance check (custom CanAcceptItem —
            // tags / type / traits) would see the not-yet-copied tags and wrongly reject it. Decide
            // that categorical question against the SOURCE (whose tags are committed) — the same
            // Get_CanAcceptItem the Stock prompt runs on the held item — and skip only it on the
            // copy. Abort ONLY on a categorical rejection: quantitative capacity is already clamped
            // into TransferCount above (Get_AbsorbableUnits) and re-derived on the copy, so a NoSpace
            // result must fall through to the normal partial-transfer path rather than abort it.
            // Checked before touching the source stack so a rejection needs no rollback.
            if (const auto SourceAccept = UCk_Utils_Inventory_UE::Get_CanAcceptItem(BaseTarget, InSourceItem);
                SourceAccept == ECk_Inventory_OperationResult_Add::Failed_RejectedByCustomAcceptanceLogic)
            {
                ck::inventory::Display(TEXT("TransferItem: source [{}] categorically rejected by target [{}]; split skipped"),
                    InSourceItem, InTarget);
                return detail::MapAddResultToTransfer(SourceAccept);
            }

            UCk_Utils_ItemTrait_Stackable_UE::Request_AdjustStackCount(InSourceItem, -MoveCount);

            auto TargetContextOwner = UCk_Utils_ContextOwner_UE::Get_ContextOwner(BaseTarget);
            auto NewItem = UCk_Utils_Item_UE::Create(TargetContextOwner, Definition);

            if (ck::Is_NOT_Valid(NewItem))
            {
                UCk_Utils_ItemTrait_Stackable_UE::Request_AdjustStackCount(InSourceItem, +MoveCount);
                ck::inventory::Warning(TEXT("TransferItem: Failed to create new item for partial transfer"));
                return ECk_Inventory_OperationResult_Transfer::Failed_NoSpaceInTarget;
            }

            Definition->OnSplit(InSourceItem, NewItem);
            UCk_Utils_ItemTrait_Stackable_UE::Request_OverrideStackCount(NewItem, MoveCount);

            // Acceptance already proven against the source above; skip the redundant categorical
            // recheck on the not-yet-tag-copied split unit. Placement / grid-space checks still run.
            const auto AddResult = DoAddToTarget(NewItem, ECk_AddAcceptance::AlreadyValidated);

            if (AddResult != ECk_Inventory_OperationResult_Add::Success)
            {
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(NewItem);
                UCk_Utils_ItemTrait_Stackable_UE::Request_AdjustStackCount(InSourceItem, +MoveCount);
                ck::inventory::Warning(TEXT("TransferItem: Target [{}] refused split add of [{}] ({}); restored source stack"),
                    InTarget, NewItem, AddResult);
                return detail::MapAddResultToTransfer(AddResult);
            }

            OutNewTargetItem = NewItem;
            OutCountTransferred += MoveCount;
            UCk_Utils_Inventory_UE::Request_MarkInventory_AsMayHaveChanged(BaseTarget);
        }

        return (OutCountTransferred >= RequestedTotal)
            ? ECk_Inventory_OperationResult_Transfer::Success
            : ECk_Inventory_OperationResult_Transfer::Success_Partial;
    }

    template <typename TSourceHandle, typename TAddon>
    auto TTransfer<TSourceHandle, FCk_Handle_Inventory_Spatial, TAddon>::Handle(
        TSourceHandle& InHandle,
        const FFragment_Inventory_Params&,
        const Entry& InEntry) -> Result
    {
        const auto& InRequest = InEntry.Common;
        auto SourceItem      = InRequest.Get_SourceItem();
        auto TargetInventory = InRequest.Get_TargetInventory();
        auto NewTargetItem   = FCk_Handle_Item{};
        auto CountTransferred = int32{0};
        auto R               = Result::Failed_InvalidSourceItem;
        FCk_Handle_Inventory Base       = InHandle;
        FCk_Handle_Inventory TargetBase = TargetInventory;

        const auto Guard = ck::MakeRequestResultGuard<UUtils_Signal_Inventory_OnOperationResult_Transfer>(
            InRequest, [&]{ return MakePayload(Base, SourceItem, TargetBase, CountTransferred, NewTargetItem, R); });

        R = DoTransfer<TSourceHandle, FCk_Handle_Inventory_Spatial, TAddon>(
            InHandle, TargetInventory, SourceItem,
            InRequest.Get_Count(), InRequest.Get_Policy(),
            InRequest.Get_Placement(), NewTargetItem, CountTransferred);
        return R;
    }

    template <typename TSourceHandle, typename TAddon>
    auto TTransfer<TSourceHandle, FCk_Handle_Inventory_DataOnly, TAddon>::Handle(
        TSourceHandle& InHandle,
        const FFragment_Inventory_Params&,
        const Entry& InEntry) -> Result
    {
        const auto& InRequest = InEntry.Common;
        auto SourceItem      = InRequest.Get_SourceItem();
        auto TargetInventory = InRequest.Get_TargetInventory();
        auto NewTargetItem   = FCk_Handle_Item{};
        auto CountTransferred = int32{0};
        auto R               = Result::Failed_InvalidSourceItem;
        FCk_Handle_Inventory Base       = InHandle;
        FCk_Handle_Inventory TargetBase = TargetInventory;

        const auto Guard = ck::MakeRequestResultGuard<UUtils_Signal_Inventory_OnOperationResult_Transfer>(
            InRequest, [&]{ return MakePayload(Base, SourceItem, TargetBase, CountTransferred, NewTargetItem, R); });

        R = DoTransfer<TSourceHandle, FCk_Handle_Inventory_DataOnly, TAddon>(
            InHandle, TargetInventory, SourceItem,
            InRequest.Get_Count(), InRequest.Get_Policy(),
            FCk_SpatialPlacement{}, NewTargetItem, CountTransferred);
        return R;
    }

    // Explicit instantiations for every (Handle, Addon) pair the framework uses. Located here
    // (not in the per-shape Traits.cpp files) because the default Handle template bodies live
    // in this TU. Specializations declared in the per-shape Traits.h are visible (both headers
    // included above), so the linker routes Spatial's overrides correctly.
    template struct TAddItem         <FCk_Handle_Inventory_DataOnly>;
    template struct TAddItem         <FCk_Handle_Inventory_Spatial,  FCk_SpatialPlacement>;
    template struct TRemoveItem      <FCk_Handle_Inventory_DataOnly>;
    template struct TRemoveItem      <FCk_Handle_Inventory_Spatial>;
    template struct TStackItems      <FCk_Handle_Inventory_DataOnly>;
    template struct TStackItems      <FCk_Handle_Inventory_Spatial>;
    template struct TSplitStack      <FCk_Handle_Inventory_DataOnly>;
    template struct TSplitStack      <FCk_Handle_Inventory_Spatial,  FCk_SpatialPlacement>;
    template struct TAddByDefinition <FCk_Handle_Inventory_DataOnly>;
    template struct TAddByDefinition <FCk_Handle_Inventory_Spatial>;
    template struct TSort            <FCk_Handle_Inventory_DataOnly>;
    template struct TSort            <FCk_Handle_Inventory_Spatial>;
    template struct TTransfer        <FCk_Handle_Inventory_Spatial,  FCk_Handle_Inventory_Spatial>;
    template struct TTransfer        <FCk_Handle_Inventory_Spatial,  FCk_Handle_Inventory_DataOnly>;
    template struct TTransfer        <FCk_Handle_Inventory_DataOnly, FCk_Handle_Inventory_Spatial>;
    template struct TTransfer        <FCk_Handle_Inventory_DataOnly, FCk_Handle_Inventory_DataOnly>;
    template struct TRelocate        <FCk_Handle_Inventory_Spatial>;
}
