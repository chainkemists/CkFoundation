#include "CkItem_Validator.h"

#include <Misc/DataValidation.h>

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR

auto
    UCk_ItemValidator::
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
    UCk_ItemValidator::
    DoValidate_Implementation(
        const UCk_InventoryItem_Definition* InDefinition,
        TArray<FText>& OutErrors) const
    -> EDataValidationResult
{
    return EDataValidationResult::Valid;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
