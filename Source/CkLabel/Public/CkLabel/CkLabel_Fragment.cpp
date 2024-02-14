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
