#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <GameplayTagContainer.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T, typename T_ArgType = T>
    struct TFragment_Variables
    {
        template <typename T>
        friend class TUtils_Variables;

    public:
        using ValueType = T;
        using ArgType = T_ArgType;

    private:
        TMap<FGameplayTag, ValueType> _Variables;

    private:
        CK_PROPERTY_GET_NON_CONST(_Variables);

    public:
        CK_PROPERTY_GET(_Variables);
    };
}

// --------------------------------------------------------------------------------------------------------------------
