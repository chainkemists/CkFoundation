#pragma once

#include "CkCore/GameplayTag/CkGameplayTagStack.h"
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
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Get Are Tag Requirements Met")
    static bool
    Get_AreTagRequirementsMet(
        const FGameplayTagRequirements& InTagRequirements,
        const FGameplayTagContainer& InTagsToTest);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Make Literal GameplayTag (From String)")
    static FGameplayTag
    Make_LiteralGameplayTag_FromString(
        const FString& InTagNameAsString);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Try Make Literal GameplayTag (From String)")
    static FGameplayTag
    TryMake_LiteralGameplayTag_FromString(
        const FString& InTagNameAsString);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Make Literal GameplayTag (From Name)")
    static FGameplayTag
    Make_LiteralGameplayTag_FromName(
        FName InTagNameAsString);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Try Make Literal GameplayTag (From Name)")
    static FGameplayTag
    TryMake_LiteralGameplayTag_FromName(
        FName InTagNameAsString);

public:
    static auto
    Get_TagsWithCount_FromContainer(
        const FGameplayTagCountContainer& InTagContainer) -> TMap<FGameplayTag, int32>;
};

// --------------------------------------------------------------------------------------------------------------------


UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_GameplayTagStack_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_GameplayTagStack_UE);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameplayTagStack",
              DisplayName = "[Ck] Add Stack to GameplayTagStackContainer")
    void
    Request_AddStack(
        UPARAM(ref) FCk_GameplayTagStackContainer& InStackContainer,
        const FCk_GameplayTagStack& InStack);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameplayTagStack",
              DisplayName = "[Ck] Remove Stack from GameplayTagStackContainer")
    void
    Request_RemoveStack(
        UPARAM(ref) FCk_GameplayTagStackContainer& InStackContainer,
        const FCk_GameplayTagStack& InStack);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameplayTagStack",
              DisplayName = "[Ck] Get Contains Tag in GameplayTagStackContainer")
    bool
    Get_ContainsTag(
        const FCk_GameplayTagStackContainer& InStackContainer,
        const FGameplayTag& InTag);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameplayTagStack",
              DisplayName = "[Ck] Get Stack Count in GameplayTagStackContainer")
    int32
    Get_StackCount(
        const FCk_GameplayTagStackContainer& InStackContainer,
        const FGameplayTag& InTag);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTagStack",
              DisplayName = "[Ck] Make GameplayTagStackContainer",
              meta = (NativeMakeFunc))
    static FCk_GameplayTagStackContainer
    Make_GameplayTagStackContainer(
        const TArray<FCk_GameplayTagStack>& InStacks);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTagStack",
              DisplayName = "[Ck] Break GameplayTagStackContainer",
              meta = (NativeBreakFunc))
    static TArray<FCk_GameplayTagStack>
    Break_GameplayTagStackContainer(
        const FCk_GameplayTagStackContainer& InStackContainer);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTagStack",
              DisplayName = "[Ck] Break GameplayTagStackContainer (As Map)")
    static TMap<FGameplayTag, int32>
    Break_GameplayTagStackContainer_AsMap(
        const FCk_GameplayTagStackContainer& InStackContainer);
};

// --------------------------------------------------------------------------------------------------------------------
