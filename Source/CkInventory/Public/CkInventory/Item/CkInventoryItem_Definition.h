#pragma once

#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include <CoreMinimal.h>

#include "CkInventoryItem_Definition.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKINVENTORY_API UCk_InventoryItem_Definition : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_InventoryItem_Definition);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FText _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, MultiLine = true))
    FText _Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UTexture2D> _Icon;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_Description);
    CK_PROPERTY_GET(_Icon);
};

// --------------------------------------------------------------------------------------------------------------------
