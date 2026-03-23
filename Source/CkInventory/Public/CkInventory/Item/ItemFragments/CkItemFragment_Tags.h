#pragma once

#include "CkInventory/Item/CkInventoryItem_ItemFragment.h"

#include <GameplayTagContainer.h>

#include "CkItemFragment_Tags.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, DisplayName = "🏷️ Tags")
struct CKINVENTORY_API FCk_ItemFragment_Tags : public FCk_ItemFragment
{
    GENERATED_BODY()

public:
    auto
    OnApplied(
        FCk_Handle_Item& InItem) const -> void override;

    auto
    CanStackWith(
        const FCk_Handle_Item& InSource,
        const FCk_Handle_Item& InTarget) const -> bool override;

    auto
    OnSplit(
        const FCk_Handle_Item& InSourceItem,
        FCk_Handle_Item& InNewItem) const -> void override;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _Tags;

public:
    CK_PROPERTY_GET(_Tags);
};

// --------------------------------------------------------------------------------------------------------------------
