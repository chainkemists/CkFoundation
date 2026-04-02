#include "CkItemTrait.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Misc/DataValidation.h>

#include "CkInventory/Item/CkItem_Definition.h"
#include "CkInventory/Item/CkItem_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ItemTrait::
    CanStackWith(
        const FCk_Handle_Item& InSource,
        const FCk_Handle_Item& InTarget) const
    -> bool
{
    return DoCanStackWith(InSource, InTarget);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ItemTrait::
    OnSplit(
        const FCk_Handle_Item& InSourceItem,
        FCk_Handle_Item& InNewItem) const
    -> void
{
    DoOnSplit(InSourceItem, InNewItem);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ItemTrait::
    DoCanStackWith_Implementation(
        const FCk_Handle_Item& InSource,
        const FCk_Handle_Item& InTarget) const
    -> bool
{
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ItemTrait::
    DoOnSplit_Implementation(
        const FCk_Handle_Item& InSourceItem,
        FCk_Handle_Item& InNewItem) const
    -> void
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ItemTrait::
    ShowReplicationInEditor() const
    -> bool
{
    return false;
}


#if WITH_EDITOR

auto
    UCk_ItemTrait::
    Validate(
        const UCk_InventoryItem_Definition* InDefinition,
        FDataValidationContext& Context) const
    -> EDataValidationResult
{
    auto Errors = TArray<FText>{};

    const auto Result = DoValidate(InDefinition, Errors);

    for (const auto& Error : Errors)
    {
        Context.AddError(Error);
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ItemTrait::
    DoValidate_Implementation(
        const UCk_InventoryItem_Definition* InDefinition,
        TArray<FText>& OutErrors) const
    -> EDataValidationResult
{
    return EDataValidationResult::Valid;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
