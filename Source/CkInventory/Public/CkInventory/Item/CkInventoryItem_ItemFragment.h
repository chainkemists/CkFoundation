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
};

// --------------------------------------------------------------------------------------------------------------------
