#pragma once

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

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

    UCk_InventoryItem_Definition();

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, NoClear,
              Category = "Core Settings",
              meta = (AllowPrivateAccess = true, ExcludeBaseStruct))
    TArray<TInstancedStruct<FCk_ItemFragment>> _ItemFragments;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_Description);
    CK_PROPERTY_GET(_Icon);
    CK_PROPERTY_GET(_ItemFragments);

public:
    template<typename T> requires std::is_base_of_v<FCk_ItemFragment, T>
    auto Get_ItemFragment() const -> const T*;

    template<typename T> requires std::is_base_of_v<FCk_ItemFragment, T>
    auto Has_ItemFragment() const -> bool;

    auto Get_ItemFragment(const UScriptStruct* InFragmentType) const -> TInstancedStruct<FCk_ItemFragment>;
    auto Has_ItemFragment(const UScriptStruct* InFragmentType) const -> bool;

    auto CanStackWith(
        const FCk_Handle_Item& InSource,
        const FCk_Handle_Item& InTarget) const -> bool;

#if WITH_EDITOR
public:
    virtual auto
    PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent) -> void override;
#endif
};

// --------------------------------------------------------------------------------------------------------------------

template<typename T> requires std::is_base_of_v<FCk_ItemFragment, T>
auto
    UCk_InventoryItem_Definition::
    Get_ItemFragment() const
    -> const T*
{
    const auto Result = ck::algo::FindIf(_ItemFragments,
    [](const TInstancedStruct<FCk_ItemFragment>& InFragment)
    {
        return InFragment.GetScriptStruct() == T::StaticStruct();
    });

    if (ck::Is_NOT_Valid(Result))
    { return nullptr; }

    return Result.GetValue().template GetPtr<T>();
}

// --------------------------------------------------------------------------------------------------------------------

template<typename T> requires std::is_base_of_v<FCk_ItemFragment, T>
auto
    UCk_InventoryItem_Definition::
    Has_ItemFragment() const
    -> bool
{
    return ck::IsValid(Get_ItemFragment<T>(), ck::IsValid_Policy_NullptrOnly{});
}

// --------------------------------------------------------------------------------------------------------------------
