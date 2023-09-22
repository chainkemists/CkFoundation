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
        FCk_Handle InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
struct CKLABEL_API MatchesGameplayLabelExact
    {
    public:
        auto operator()(const FCk_Handle& InHandle) const -> bool;

    private:
        FGameplayTag _Name;

    public:
        CK_DEFINE_CONSTRUCTORS(MatchesGameplayLabelExact, _Name)
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKLABEL_API MatchesGameplayLabel
    {
    public:
        auto operator()(const FCk_Handle& InHandle) const -> bool;

    private:
        FGameplayTag _Name;

    public:
        CK_DEFINE_CONSTRUCTORS(MatchesGameplayLabel, _Name)
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKLABEL_API MatchesAnyGameplayLabel
    {
    public:
        auto operator()(const FCk_Handle& InHandle) const -> bool;

    private:
        FGameplayTagContainer _Names;

    public:
        CK_DEFINE_CONSTRUCTORS(MatchesAnyGameplayLabel, _Names)
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKLABEL_API MatchesAnyGameplayLabelExact
    {
    public:
        auto operator()(const FCk_Handle& InHandle) const -> bool;

    private:
        FGameplayTagContainer _Names;

    public:
        CK_DEFINE_CONSTRUCTORS(MatchesAnyGameplayLabelExact, _Names)
    };