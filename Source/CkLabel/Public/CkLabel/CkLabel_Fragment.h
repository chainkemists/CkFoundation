#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <GameplayTagContainer.h>

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Concepts/CkConcepts.h"

#include "CkLabel/CkLabel_Utils.h"

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
        template <concepts::ValidHandleType T_Handle>
        auto operator()(const T_Handle& InHandle) const -> bool
        {
            if (NOT UCk_Utils_GameplayLabel_UE::Has(InHandle))
            { return false; }

            return UCk_Utils_GameplayLabel_UE::MatchesExact(InHandle, _Name);
        }

    private:
        FGameplayTag _Name;

    public:
        CK_DEFINE_CONSTRUCTORS(MatchesGameplayLabelExact, _Name)
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKLABEL_API MatchesGameplayLabel
    {
    public:
        template <concepts::ValidHandleType T_Handle>
        auto operator()(const T_Handle& InHandle) const -> bool
        {
            if (NOT UCk_Utils_GameplayLabel_UE::Has(InHandle))
            { return false; }

            return UCk_Utils_GameplayLabel_UE::Matches(InHandle, _Name);
        }

    private:
        FGameplayTag _Name;

    public:
        CK_DEFINE_CONSTRUCTORS(MatchesGameplayLabel, _Name)
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKLABEL_API MatchesAnyGameplayLabel
    {
    public:
        template <concepts::ValidHandleType T_Handle>
        auto operator()(const T_Handle& InHandle) const -> bool
        {
            if (NOT UCk_Utils_GameplayLabel_UE::Has(InHandle))
            { return false; }

            return UCk_Utils_GameplayLabel_UE::MatchesAny(InHandle, _Names);
        }

    private:
        FGameplayTagContainer _Names;

    public:
        CK_DEFINE_CONSTRUCTORS(MatchesAnyGameplayLabel, _Names)
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKLABEL_API MatchesAnyGameplayLabelExact
    {
    public:
        template <concepts::ValidHandleType T_Handle>
        auto operator()(const T_Handle& InHandle) const -> bool
        {
            if (NOT UCk_Utils_GameplayLabel_UE::Has(InHandle))
            { return false; }

            return UCk_Utils_GameplayLabel_UE::MatchesAnyExact(InHandle, _Names);
        }

    private:
        FGameplayTagContainer _Names;

    public:
        CK_DEFINE_CONSTRUCTORS(MatchesAnyGameplayLabelExact, _Names)
    };
}

// --------------------------------------------------------------------------------------------------------------------
