#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <GameplayTagContainer.h>
#include <GameplayEffectTypes.h>

#include "CkGameplayTag_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_GameplayTag_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_GameplayTag_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|GameplayTag|Utils",
              DisplayName = "[Ck] Get Are Tag Requirements Met")
    static bool
    Get_AreTagRequirementsMet(
        const FGameplayTagRequirements& InTagRequirements,
        const FGameplayTagContainer& InTagsToTest);

public:
    static auto
    Get_TagsWithCount_FromContainer(
        const FGameplayTagCountContainer& InTagContainer) -> TMap<FGameplayTag, int32>;
};

// --------------------------------------------------------------------------------------------------------------------
