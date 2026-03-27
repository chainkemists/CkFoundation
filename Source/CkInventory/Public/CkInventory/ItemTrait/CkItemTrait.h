#pragma once

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkInventory/Item/CkItem_Fragment_Data.h"

#include <CoreMinimal.h>

#include "CkItemTrait.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_InventoryItem_Definition;
class FDataValidationContext;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, CollapseCategories)
class CKINVENTORY_API UCk_ItemTrait : public UCk_Entity_ConstructionScript_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ItemTrait);

    friend class UCk_InventoryItem_Definition;

    UCk_ItemTrait();

    // ---- Stack / Split ----

public:
    auto
    CanStackWith(
        const FCk_Handle_Item& InSource,
        const FCk_Handle_Item& InTarget) const -> bool;

    auto
    OnSplit(
        const FCk_Handle_Item& InSourceItem,
        FCk_Handle_Item& InNewItem) const -> void;

protected:
    UFUNCTION(BlueprintNativeEvent,
              DisplayName = "Can Stack With")
    bool
    DoCanStackWith(
        const FCk_Handle_Item& InSource,
        const FCk_Handle_Item& InTarget) const;

    UFUNCTION(BlueprintNativeEvent,
              DisplayName = "On Split")
    void
    DoOnSplit(
        const FCk_Handle_Item& InSourceItem,
        UPARAM(ref) FCk_Handle_Item& InNewItem) const;

    // ---- Validation (Editor Only) ----

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

    // ---- Trait Query ----

public:
    template<typename T> requires std::is_base_of_v<UCk_ItemTrait, T>
    static auto Get(const FCk_Handle_Item& InItem) -> const T*;

    template<typename T> requires std::is_base_of_v<UCk_ItemTrait, T>
    static auto Has(const FCk_Handle_Item& InItem) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
