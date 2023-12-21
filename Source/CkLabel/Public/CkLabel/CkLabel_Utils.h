#pragma once

#include <CkEcs/Handle/CkHandle.h>

#include <CkCore/Macros/CkMacros.h>

#include "CkLabel_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKLABEL_API UCk_Utils_GameplayLabel_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_GameplayLabel_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Label",
              DisplayName="Add Gameplay Label")
    static void
    Add(
        FCk_Handle InHandle,
        const FGameplayTag& InLabel);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Label",
              DisplayName="Has Gameplay Label")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Label",
              DisplayName="Ensure Has Gameplay Label")
    static bool
    Ensure(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Label",
              DisplayName="Get GameplayTag Label")
    static FGameplayTag
    Get_Label(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Label",
              DisplayName="Matches Label")
    static bool
    Matches(
        FCk_Handle InHandle,
        FGameplayTag InTagToMatch);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Label",
              DisplayName="Matches Label Exactly")
    static bool
    MatchesExact(
        FCk_Handle InHandle,
        FGameplayTag InTagToMatch);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Label",
              DisplayName="Matches Any Label")
    static bool
    MatchesAny(
        FCk_Handle                   InHandle,
        const FGameplayTagContainer& InTagsToMatch);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Label",
              DisplayName="Matches Any Label Exactly")
    static bool
    MatchesAnyExact(
        FCk_Handle                   InHandle,
        const FGameplayTagContainer& InTagsToMatch);

private:
    static auto
    DoGet_LabelOrNone(
        FGameplayTag InTag) -> FGameplayTag;
};

// --------------------------------------------------------------------------------------------------------------------
