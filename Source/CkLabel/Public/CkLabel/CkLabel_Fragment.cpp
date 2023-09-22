#include "CkLabel_Fragment.h"

#include "CkCore/Functional/CkFunctional.h"
#include "CkLabel/CkLabel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FFragment_GameplayLabel::
        operator==(
            const ThisType& InOther) const
        -> bool
    {
        return comparators::GameplayTag_MatchesTagExact{}(Get_Label(), InOther.Get_Label());
    }

    auto
        FFragment_GameplayLabel::
        operator==(
            const FGameplayTag& InOther) const
        -> bool
    {
        return comparators::GameplayTag_MatchesTagExact{}(Get_Label(), InOther);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    auto
        MatchesGameplayLabelExact::
        operator()(
            const FCk_Handle& InHandle) const
        -> bool
    {
        if (NOT UCk_Utils_GameplayLabel_UE::Has(InHandle))
        { return false; }

        return UCk_Utils_GameplayLabel_UE::Get_Label(InHandle).MatchesTagExact(_Name);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        MatchesGameplayLabel::
        operator()(
            const FCk_Handle& InHandle) const
        -> bool
    {
        if (NOT UCk_Utils_GameplayLabel_UE::Has(InHandle))
        { return false; }

        return UCk_Utils_GameplayLabel_UE::Get_Label(InHandle).MatchesTag(_Name);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        MatchesAnyGameplayLabel::
        operator()(
            const FCk_Handle& InHandle) const
        -> bool
    {
        if (NOT UCk_Utils_GameplayLabel_UE::Has(InHandle))
        { return false; }

        return _Names.HasTag(UCk_Utils_GameplayLabel_UE::Get_Label(InHandle));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        MatchesAnyGameplayLabelExact::
        operator()(
            const FCk_Handle& InHandle) const
        -> bool
    {
        if (NOT UCk_Utils_GameplayLabel_UE::Has(InHandle))
        { return false; }

        return _Names.HasTagExact(UCk_Utils_GameplayLabel_UE::Get_Label(InHandle));
    }
}

// --------------------------------------------------------------------------------------------------------------------
