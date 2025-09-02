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

        CK_GENERATED_BODY(TFragment_Variables<T COMMA T_ArgType>);

    public:
        using ValueType = T;
        using ArgType = T_ArgType;

    private:
        TMap<FName, ValueType> _Variables;

    public:
        CK_PROPERTY(_Variables);
    };
}

// --------------------------------------------------------------------------------------------------------------------
