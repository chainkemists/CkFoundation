#pragma once

#include "CkInventory/Item/CkInventoryItem_ItemFragment.h"

#include <NativeGameplayTags.h>

#include "CkItemFragment_Stackable.generated.h"

// --------------------------------------------------------------------------------------------------------------------

CKINVENTORY_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_IntegerAttribute_InventoryItem_StackCount);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, DisplayName = "🔢 Stackable")
struct CKINVENTORY_API FCk_ItemFragment_Stackable : public FCk_ItemFragment
{
    GENERATED_BODY()

public:
    auto
    OnApplied(
        FCk_Handle_Item& InItem) const -> void override;

private:
    // The initial stack count for this item. Must be >= 1.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 1))
    int32 _InitialCount = 1;

    // Whether this item has a maximum stack size.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _HasMaxStackSize = true;

    // The maximum number of items that can be stacked.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 1, EditCondition = "_HasMaxStackSize"))
    int32 _MaxStackSize = 10;

public:
    CK_PROPERTY_GET(_InitialCount);
    CK_PROPERTY_GET(_HasMaxStackSize);
    CK_PROPERTY_GET(_MaxStackSize);
};

// --------------------------------------------------------------------------------------------------------------------
