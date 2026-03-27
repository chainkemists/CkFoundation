#pragma once

#include "CkInventory/ItemTrait/CkItemTrait.h"

#include <GameplayTagContainer.h>

#include "CkItemTrait_Tags.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DisplayName = "🏷️ Tags")
class CKINVENTORY_API UCk_ItemTrait_Tags : public UCk_ItemTrait
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ItemTrait_Tags);

public:
    auto
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const -> void override;

    auto
    DoCanStackWith_Implementation(
        const FCk_Handle_Item& InSource,
        const FCk_Handle_Item& InTarget) const -> bool override;

    auto
    DoOnSplit_Implementation(
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
