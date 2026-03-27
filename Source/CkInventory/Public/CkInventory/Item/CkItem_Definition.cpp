#include "CkItem_Definition.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include <Misc/DataValidation.h>

#include "CkInventory/Item/CkItem_Fragment.h"
#include "CkInventory/Item/CkItem_Utils.h"
#include "CkInventory/ItemTrait/CkItemTrait.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_InventoryItem_Definition::UCk_InventoryItem_Definition()
{
    _ShowReplicationInEditor = false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryItem_Definition::
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const
    -> void
{
    InHandle.Add<ck::FFragment_InventoryItem>(TWeakObjectPtr(this));

    for (const auto& ItemTrait : _ItemTraits)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(ItemTrait),
            TEXT("DoConstruct: Invalid ItemTrait on Definition [{}]."), this->GetFName())
        { continue; }

        Request_Construct_Instanced(InHandle, ItemTrait.Get());
    }

    UCk_Utils_Handle_UE::Set_DebugName(InHandle, *ck::Format_UE(TEXT("Item ({})"), this->GetFName()));
}

// --------------------------------------------------------------------------------------------------------------------

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
    return ck::IsValid(Get_ItemTrait(InTraitClass), ck::IsValid_Policy_NullptrOnly{});
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

    // Check if the changed element's type already exists at another index
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
        // Restore the previous class that was at this index before the edit
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
