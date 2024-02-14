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
              DisplayName="[Ck][Label] Add Feature")
    static void
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FGameplayTag& InLabel);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Label",
              DisplayName="[Ck][Label] Has Feature")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Label",
              DisplayName="[Ck][Label] Ensure Has Feature")
    static bool
    Ensure(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Label",
              DisplayName="[Ck][Label] Get Label Name")
    static FGameplayTag
    Get_Label(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Label",
              DisplayName="[Ck][Label] Matches")
    static bool
    Matches(
        FCk_Handle InHandle,
        FGameplayTag InTagToMatch);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Label",
              DisplayName="[Ck][Label] Matches Exactly")
    static bool
    MatchesExact(
        FCk_Handle InHandle,
        FGameplayTag InTagToMatch);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Label",
              DisplayName="[Ck][Label] Matches Any")
    static bool
    MatchesAny(
        FCk_Handle InHandle,
        const FGameplayTagContainer& InTagsToMatch);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Label",
              DisplayName="[Ck][Label] Matches Any Exactly")
    static bool
    MatchesAnyExact(
        FCk_Handle InHandle,
        const FGameplayTagContainer& InTagsToMatch);

private:
    static auto
    DoGet_LabelOrNone(
        FGameplayTag InTag) -> FGameplayTag;
};

// --------------------------------------------------------------------------------------------------------------------
