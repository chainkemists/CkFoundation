#pragma once

#include "CkInventory/ItemTrait/CkItemTrait.h"

#include <NativeGameplayTags.h>

#include "CkItemTrait_Stackable.generated.h"

// --------------------------------------------------------------------------------------------------------------------

CKINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IntegerAttribute_InventoryItem_StackCount);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DisplayName = "🔢 Stackable")
class CKINVENTORY_API UCk_ItemTrait_Stackable : public UCk_ItemTrait
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_ItemFragment_Stackable);

public:
    CK_GENERATED_BODY(UCk_ItemTrait_Stackable);

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
              meta = (AllowPrivateAccess = true, ClampMin = 1))
    int32 _InitialCount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _HasMaxStackSize = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 1, EditCondition = "_HasMaxStackSize"))
    int32 _MaxStackSize = 10;

public:
    CK_PROPERTY_GET(_InitialCount);
    CK_PROPERTY_GET(_HasMaxStackSize);
    CK_PROPERTY_GET(_MaxStackSize);
};

// --------------------------------------------------------------------------------------------------------------------
