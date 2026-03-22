#pragma once

#include "CkInventory/Item/CkInventoryItem_Fragment_Data.h"

#include <CoreMinimal.h>

#include "CkInventoryItem_ItemFragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (Hidden))
struct CKINVENTORY_API FCk_ItemFragment
{
    GENERATED_BODY()

public:
    virtual ~FCk_ItemFragment() = default;

    virtual auto
    OnApplied(
        FCk_Handle_Item& InItem) const -> void {}

    virtual auto
    CanStackWith(
        const FCk_Handle_Item& InSource,
        const FCk_Handle_Item& InTarget) const -> bool { return true; }

public:
    // Retrieves a specific item fragment from the item's definition.
    template<typename T> requires std::is_base_of_v<FCk_ItemFragment, T>
    static auto Get(const FCk_Handle_Item& InItem) -> const T*;

    template<typename T> requires std::is_base_of_v<FCk_ItemFragment, T>
    static auto Has(const FCk_Handle_Item& InItem) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
