#pragma once

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkInventory/Item/CkInventoryItem_ItemFragment.h"

#include <CoreMinimal.h>

#include "CkInventoryItem_Definition.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class CKINVENTORY_API UCk_InventoryItem_Definition : public UCk_Entity_ConstructionScript_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_InventoryItem_Definition);

public:
    auto
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const -> void override;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Core Settings",
              meta = (AllowPrivateAccess = true))
    FText _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Core Settings",
              meta = (AllowPrivateAccess = true, MultiLine = true))
    FText _Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Core Settings",
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UTexture2D> _Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Core Settings",
              meta = (AllowPrivateAccess = true, BaseStruct = "/Script/CkInventory.Ck_ItemFragment", ExcludeBaseStruct))
    TArray<FInstancedStruct> _ItemFragments;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_Description);
    CK_PROPERTY_GET(_Icon);
    CK_PROPERTY_GET(_ItemFragments);
};

// --------------------------------------------------------------------------------------------------------------------
