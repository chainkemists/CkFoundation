#pragma once

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkInventory/ItemTrait/CkItemTrait.h"
#include "CkInventory/Item/CkItem_Validator.h"

#include <CoreMinimal.h>

#include "CkItem_Definition.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Read-only display info for an item definition: name, description, and icon.
 * Used by UI to populate slot visuals without pulling individual fields.
 */
USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_InventoryItem_CoreInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_InventoryItem_CoreInfo);

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FText _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FText _Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UTexture2D> _Icon;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY(_Description);
    CK_PROPERTY(_Icon);

    CK_DEFINE_CONSTRUCTORS(FCk_InventoryItem_CoreInfo, _Name);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class CKINVENTORY_API UCk_InventoryItem_Definition : public UCk_Entity_ConstructionScript_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_InventoryItem_Definition);

    UCk_InventoryItem_Definition();

public:
    auto
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const -> void override;

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Core Settings",
              meta = (AllowPrivateAccess = true, ShowOnlyInnerProperties))
    FCk_InventoryItem_CoreInfo _CoreInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, NoClear, Instanced,
              Category = "Core Settings",
              meta = (AllowPrivateAccess = true))
    TArray<TObjectPtr<const UCk_ItemTrait>> _ItemTraits;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Validation",
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_ItemValidator> _ItemValidatorClass;

public:
    CK_PROPERTY_GET(_CoreInfo);
    CK_PROPERTY_GET(_ItemTraits);

public:
    template<typename T> requires std::is_base_of_v<UCk_ItemTrait, T>
    auto Get_ItemTrait() const -> const T*;

    template<typename T> requires std::is_base_of_v<UCk_ItemTrait, T>
    auto Has_ItemTrait() const -> bool;

    auto Get_ItemTrait(const UClass* InTraitClass) const -> const UCk_ItemTrait*;
    auto Has_ItemTrait(const UClass* InTraitClass) const -> bool;

    auto CanStackWith(
        const FCk_Handle_Item& InSource,
        const FCk_Handle_Item& InTarget) const -> bool;

    auto OnSplit(
        const FCk_Handle_Item& InSourceItem,
        FCk_Handle_Item& InNewItem) const -> void;

#if WITH_EDITOR
public:
    auto
    IsDataValid(
        FDataValidationContext& Context) const -> EDataValidationResult override;

    virtual auto
    PreEditChange(
        FProperty* PropertyAboutToChange) -> void override;

    virtual auto
    PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent) -> void override;

private:
    TArray<TSubclassOf<UCk_ItemTrait>> _CachedTraitClasses;
#endif
};

// --------------------------------------------------------------------------------------------------------------------

template<typename T> requires std::is_base_of_v<UCk_ItemTrait, T>
auto
    UCk_InventoryItem_Definition::
    Get_ItemTrait() const
    -> const T*
{
    const auto Result = ck::algo::FindIf(_ItemTraits,
    [](const TObjectPtr<const UCk_ItemTrait>& InTrait)
    {
        return ck::IsValid(InTrait) && InTrait->IsA<T>();
    });

    if (ck::Is_NOT_Valid(Result))
    { return nullptr; }

    return Cast<T>(Result.GetValue().Get());
}

// --------------------------------------------------------------------------------------------------------------------

template<typename T> requires std::is_base_of_v<UCk_ItemTrait, T>
auto
    UCk_InventoryItem_Definition::
    Has_ItemTrait() const
    -> bool
{
    return ck::IsValid(Get_ItemTrait<T>(), ck::IsValid_Policy_NullptrOnly{});
}

// --------------------------------------------------------------------------------------------------------------------
