#pragma once

#include "CkInventory/ItemTrait/CkItemTrait.h"

#include "CkItemTrait_Dimensions.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DisplayName = "📐 Dimensions")
class CKINVENTORY_API UCk_ItemTrait_Dimensions : public UCk_ItemTrait
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ItemTrait_Dimensions);

public:
    auto
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const -> void override;

#if WITH_EDITOR
    auto
    DoValidate_Implementation(
        const UCk_InventoryItem_Definition* InDefinition,
        TArray<FText>& OutErrors) const -> EDataValidationResult override;
#endif

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _Dimensions = FIntPoint(1, 1);

public:
    CK_PROPERTY_GET(_Dimensions);
};

// --------------------------------------------------------------------------------------------------------------------
