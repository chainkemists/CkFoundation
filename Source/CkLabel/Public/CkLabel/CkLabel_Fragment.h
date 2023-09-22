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
        auto operator==(const ThisType& InOther) const -> bool;
        CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

        auto operator==(const FGameplayTag& InOther) const -> bool;
        CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(FGameplayTag);

    private:
        FGameplayTag _Label;

    public:
        CK_PROPERTY_GET(_Label);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_GameplayLabel, _Label)
    };
}

// --------------------------------------------------------------------------------------------------------------------
// Algos

namespace ck::algo
{
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
}

// --------------------------------------------------------------------------------------------------------------------
