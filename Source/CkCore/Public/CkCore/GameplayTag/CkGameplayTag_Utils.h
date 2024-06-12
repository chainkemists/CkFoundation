#pragma once

#include "CkCore/Enums/CkEnums.h"
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

    UFUNCTION(BlueprintPure,
              Category = "Ck|GameplayTag|Utils",
              DisplayName = "[Ck] Make Literal Gameplay Tag (From String)")
    static FGameplayTag
    Make_LiteralGameplayTag_FromString(
        const FString& InTagNameAsString);

    UFUNCTION(BlueprintPure,
              Category = "Ck|GameplayTag|Utils",
              DisplayName = "[Ck] Try Make Literal Gameplay Tag (From String)")
    static FGameplayTag
    TryMake_LiteralGameplayTag_FromString(
        const FString& InTagNameAsString);

    UFUNCTION(BlueprintPure,
              Category = "Ck|GameplayTag|Utils",
              DisplayName = "[Ck] Make Literal Gameplay Tag (From Name)")
    static FGameplayTag
    Make_LiteralGameplayTag_FromName(
        FName InTagNameAsString);

    UFUNCTION(BlueprintPure,
              Category = "Ck|GameplayTag|Utils",
              DisplayName = "[Ck] Try Make Literal Gameplay Tag (From Name)")
    static FGameplayTag
    TryMake_LiteralGameplayTag_FromName(
        FName InTagNameAsString);

public:
    static auto
    Get_TagsWithCount_FromContainer(
        const FGameplayTagCountContainer& InTagContainer) -> TMap<FGameplayTag, int32>;
};

// --------------------------------------------------------------------------------------------------------------------
