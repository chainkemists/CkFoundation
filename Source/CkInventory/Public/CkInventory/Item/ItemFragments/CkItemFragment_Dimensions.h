#pragma once

#include "CkInventory/Item/CkInventoryItem_ItemFragment.h"

#include "CkItemFragment_Dimensions.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, DisplayName = "📐 Dimensions")
struct CKINVENTORY_API FCk_ItemFragment_Dimensions : public FCk_ItemFragment
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_ItemFragment_Dimensions);

public:
    auto
    OnApplied(
        FCk_Handle_Item& InItem) const -> void override;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FIntPoint _Dimensions = FIntPoint(1, 1);

public:
    CK_PROPERTY_GET(_Dimensions);
};

// --------------------------------------------------------------------------------------------------------------------
