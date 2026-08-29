#include "CkItem_Definition.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Snapshot/CkSnapshot_RestoreMarker.h"

#include "CkInventory/CkInventory_Log.h"
#include "CkInventory/Item/CkItem_Fragment.h"
#include "CkInventory/ItemTrait/CkItemTrait.h"

#include <Misc/DataValidation.h>


// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryItem_Definition::
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const
    -> void
{
    // An item whose definition is the CLASS DEFAULT is not an item. It has no traits, no name and no icon, and
    // is still structurally valid enough to hold an inventory slot and a grid cell for the rest of the save's
    // life - invisible, unusable, and impossible for the player to clear.
    //
    // Nothing produces one deliberately: Create always sets an archetype and Add is handed a real definition.
    // The KNOWN producer is a snapshot load whose recorded archetype path did not resolve, which falls back to
    // constructing from this CDO. Catching it HERE catches it by what the entity IS rather than by how it was
    // made, which is the whole point: it stays route-agnostic, so it also covers the case CkSnapshot cannot see
    // - a recipe whose path is EMPTY because an older build already rebuilt it once and captured the null
    // archetype back as nothing - and any producer nobody has thought of yet. There is deliberately no load or
    // net-mode branch here; deciding which route minted it is the reaper's job, not the detector's.
    //
    // Composition deliberately CONTINUES. The entity stays structurally an item so its container hydrates
    // intact - the inventory handlers are all-or-nothing, and refusing here would fail the whole container
    // rather than the one broken entry. The tag is what gets this reaped: at load finish when the load owns it,
    // and by ck::FProcessor_UnresolvedHusk_Reap on every other route.
    const auto DefinitionIsUsable = NOT HasAnyFlags(RF_ClassDefaultObject);

    if (NOT DefinitionIsUsable)
    {
        // A Warning and not an ensure, deliberately, and this is the ONE place in the husk contract where that is
        // the right level: reaching here is a statement about the DATA - a save naming content this build cannot
        // resolve - and it fires once per broken item on every load of that save, which is a condition the player
        // is already told about through the load report. The loud channel belongs to the reaper, which knows
        // something this guard cannot: whether a load owns this husk. A husk that arrives by any other route means
        // an unaccounted-for producer, and THAT is a code defect, so it ensures there.
        ck::inventory::Warning(
            TEXT("Item entity [{}] is being constructed from the CLASS DEFAULT of [{}], so it has no traits and no ")
            TEXT("identity. This is what a save whose item definition could not be resolved rebuilds into; the entity ")
            TEXT("is marked so it is reaped instead of left holding a slot nothing can ever use"),
            InHandle, GetClass());

        InHandle.AddOrGet<ck::FTag_Snapshot_UnresolvedArchetype>();
    }

    InHandle.Add<ck::FFragment_InventoryItem>(TWeakObjectPtr(this));

    auto SeenTraitClasses = TSet<const UClass*>{};
    for (const auto& ItemTrait : _ItemTraits)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(ItemTrait),
            TEXT("DoConstruct: Invalid ItemTrait on Definition [{}]."), this->GetFName())
        { continue; }

        // One-trait-per-class is invariant (Get_ItemTrait<T> returns the first match). Caught here so
        // a runtime breach doesn't cascade into Fragment-already-exists ensures downstream.
        bool AlreadySeen = false;
        SeenTraitClasses.Add(ItemTrait->GetClass(), &AlreadySeen);
        CK_ENSURE_IF_NOT(NOT AlreadySeen,
            TEXT("DoConstruct: Duplicate trait class [{}] on Definition [{}]."),
            ItemTrait->GetClass(), this->GetFName())
        { continue; }

        Request_Construct_Instanced(InHandle, ItemTrait.Get(), {});
    }

    UCk_Utils_Handle_UE::Set_DebugName(InHandle, *ck::Format_UE(TEXT("Item ({})"), this->GetFName()));
}

auto
    UCk_InventoryItem_Definition::
    ShowReplicationInEditor() const
    -> bool
{
    return false;
}

auto
    UCk_InventoryItem_Definition::
    GetPrimaryAssetId() const
    -> FPrimaryAssetId
{
    return FPrimaryAssetId{FPrimaryAssetType{_AssetRegistryCategory}, GetFName()};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryItem_Definition::
    Get_ItemTraitByClass(
        TSubclassOf<UCk_ItemTrait> InTraitClass) const
    -> const UCk_ItemTrait*
{
    return Get_ItemTrait(InTraitClass.Get());
}

auto
    UCk_InventoryItem_Definition::
    Get_ItemTrait(
        const UClass* InTraitClass) const
    -> const UCk_ItemTrait*
{
    const auto Result = ck::algo::FindIf(_ItemTraits,
    [InTraitClass](const TObjectPtr<const UCk_ItemTrait>& InTrait)
    {
        return ck::IsValid(InTrait) && InTrait->IsA(InTraitClass);
    });

    if (ck::Is_NOT_Valid(Result))
    { return nullptr; }

    return Result.GetValue().Get();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryItem_Definition::
    Has_ItemTrait(
        const UClass* InTraitClass) const
    -> bool
{
    return ck::IsValid(Get_ItemTrait(InTraitClass));
}

auto
    UCk_InventoryItem_Definition::
    Has_ItemTraitByClass(
        TSubclassOf<UCk_ItemTrait> InTraitClass) const
    -> bool
{
    return Has_ItemTrait(InTraitClass.Get());
}

auto
    UCk_InventoryItem_Definition::
    Has_AnyItemTraitByClass(
        const TArray<TSubclassOf<UCk_ItemTrait>>& InTraitClasses) const
    -> bool
{
    return ck::algo::AnyOf(InTraitClasses,
    [&](const TSubclassOf<UCk_ItemTrait>& InTraitClass)
    {
        return Has_ItemTrait(InTraitClass.Get());
    });
}

auto
    UCk_InventoryItem_Definition::
    Has_AllItemTraitsByClass(
        const TArray<TSubclassOf<UCk_ItemTrait>>& InTraitClasses) const
    -> bool
{
    return ck::algo::AllOf(InTraitClasses,
    [&](const TSubclassOf<UCk_ItemTrait>& InTraitClass)
    {
        return Has_ItemTrait(InTraitClass.Get());
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryItem_Definition::
    CanStackWith(
        const FCk_Handle_Item& InSource,
        const FCk_Handle_Item& InTarget) const
    -> bool
{
    return ck::algo::AllOf(_ItemTraits,
    [&](const TObjectPtr<const UCk_ItemTrait>& InTrait)
    {
        return ck::IsValid(InTrait) && InTrait->CanStackWith(InSource, InTarget);
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryItem_Definition::
    OnSplit(
        const FCk_Handle_Item& InSourceItem,
        FCk_Handle_Item& InNewItem) const
    -> void
{
    ck::algo::ForEach(_ItemTraits,
    [&](const TObjectPtr<const UCk_ItemTrait>& InTrait)
    {
        if (ck::IsValid(InTrait))
        { InTrait->OnSplit(InSourceItem, InNewItem); }
    });
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR

auto
    UCk_InventoryItem_Definition::
    IsDataValid(
        FDataValidationContext& Context) const
    -> EDataValidationResult
{
    auto Result = Super::IsDataValid(Context);

    if (IsTemplate())
    { return Result; }

    // ---- Validate Core Info ----

    if (_CoreInfo.Get_Name().IsEmpty())
    {
        Context.AddError(FText::FromString(ck::Format_UE(
            TEXT("Item Definition [{}] has an empty Name."), this->GetFName())));

        Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
    }

    // ---- Validate Traits ----

    for (auto Idx = 0; Idx < _ItemTraits.Num(); ++Idx)
    {
        const auto& Trait = _ItemTraits[Idx];

        if (ck::Is_NOT_Valid(Trait))
        {
            Context.AddError(FText::FromString(ck::Format_UE(
                TEXT("Item Definition [{}] has a null trait at index {}."), this->GetFName(), Idx)));

            Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
            continue;
        }

        Result = CombineDataValidationResults(Result, Trait->Validate(this, Context));
    }

    // ---- Validate via ItemValidator class (if set) ----

    if (ck::IsValid(_ItemValidatorClass))
    {
        const auto* Validator = GetDefault<UCk_ItemValidator>(_ItemValidatorClass);

        if (ck::IsValid(Validator))
        {
            Result = CombineDataValidationResults(Result, Validator->Validate(this, Context));
        }
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryItem_Definition::
    PreEditChange(
        FProperty* PropertyAboutToChange)
    -> void
{
    Super::PreEditChange(PropertyAboutToChange);

    // Snapshot the current trait classes so PostEditChangeProperty can restore on duplicate
    _CachedTraitClasses.Reset();
    for (const auto& Trait : _ItemTraits)
    {
        _CachedTraitClasses.Add(ck::IsValid(Trait) ? Trait->GetClass() : nullptr);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryItem_Definition::
    PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.GetMemberPropertyName() != GET_MEMBER_NAME_CHECKED(UCk_InventoryItem_Definition, _ItemTraits))
    { return; }

    const auto ChangedIndex = PropertyChangedEvent.GetArrayIndex(
        GET_MEMBER_NAME_STRING_CHECKED(UCk_InventoryItem_Definition, _ItemTraits));

    if (ChangedIndex == INDEX_NONE)
    { return; }

    if (NOT _ItemTraits.IsValidIndex(ChangedIndex))
    { return; }

    const auto& ChangedTrait = _ItemTraits[ChangedIndex];

    if (ck::Is_NOT_Valid(ChangedTrait))
    { return; }

    const auto* ChangedClass = ChangedTrait->GetClass();

    const auto IsDuplicate = ck::algo::AnyOf(_ItemTraits,
        [&, Idx = 0](const TObjectPtr<const UCk_ItemTrait>& InTrait) mutable
    {
        const auto CurrentIdx = Idx++;
        return CurrentIdx != ChangedIndex
            && ck::IsValid(InTrait)
            && InTrait->GetClass() == ChangedClass;
    });

    if (IsDuplicate)
    {
        const auto PreviousClass = _CachedTraitClasses.IsValidIndex(ChangedIndex)
            ? _CachedTraitClasses[ChangedIndex]
            : TSubclassOf<UCk_ItemTrait>{};

        if (ck::IsValid(PreviousClass))
        {
            _ItemTraits[ChangedIndex] = NewObject<UCk_ItemTrait>(this, PreviousClass.Get());
        }
        else
        {
            _ItemTraits[ChangedIndex] = nullptr;
        }
    }
}

#endif

// --------------------------------------------------------------------------------------------------------------------
