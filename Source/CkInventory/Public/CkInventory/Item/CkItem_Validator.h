#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>

#include "CkItem_Validator.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_InventoryItem_Definition;
class FDataValidationContext;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKINVENTORY_API UCk_ItemValidator : public UObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ItemValidator);

    // ---- Validation ----

#if WITH_EDITOR
public:
    auto
    Validate(
        const UCk_InventoryItem_Definition* InDefinition,
        FDataValidationContext& Context) const -> EDataValidationResult;

protected:
    UFUNCTION(BlueprintNativeEvent,
              DisplayName = "Validate")
    EDataValidationResult
    DoValidate(
        const UCk_InventoryItem_Definition* InDefinition,
        UPARAM(ref) TArray<FText>& OutErrors) const;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
