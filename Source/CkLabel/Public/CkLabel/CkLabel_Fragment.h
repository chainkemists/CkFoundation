#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <GameplayTagContainer.h>

#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FFragment_GameplayLabel
    {
        CK_GENERATED_BODY(FFragment_GameplayLabel);

    public:
        FFragment_GameplayLabel() = default;
        explicit FFragment_GameplayLabel(FGameplayTag InLabel);

    public:
        auto operator==(const ThisType& InOther) const -> bool;
        CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

        auto operator==(const FGameplayTag& InOther) const -> bool;
        CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(FGameplayTag);

    private:
        FGameplayTag _Label;

    public:
        CK_PROPERTY_GET(_Label);
    };

}

// --------------------------------------------------------------------------------------------------------------------
// Algos

namespace ck::algo
{
    struct CKLABEL_API MatchesGameplayLabel
    {
        explicit MatchesGameplayLabel(FGameplayTag InName);

    public:
        auto operator()(const FCk_Handle& InHandle) const -> bool;

    private:
        FGameplayTag _Name;
    };

    struct CKLABEL_API MatchesAnyGameplayLabel
    {
        explicit MatchesAnyGameplayLabel(FGameplayTagContainer InNames);

    public:
        auto operator()(const FCk_Handle& InHandle) const -> bool;

    private:
        FGameplayTagContainer _Names;
    };
}

// --------------------------------------------------------------------------------------------------------------------
