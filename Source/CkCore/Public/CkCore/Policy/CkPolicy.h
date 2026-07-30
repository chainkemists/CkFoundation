#pragma once

#include "CkCore/TypeTraits/CkTypeTraits.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::policy
{
    struct All {};
    struct Any {};

    struct TransientPackage {};

    struct ReturnOptional {};
    struct DontResetContainer {};

    // --------------------------------------------------------------------------------------------------------------------

    struct ForceErase {};

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T>
    struct TMutability
    {
        static_assert
        (
            std::is_same<type_traits::Const, T>() || std::is_same<type_traits::NonConst, T>(),
            "Mutability can only accept `Const` or `NonConst` as policy params"
        );
    };

    template<>
    struct TMutability<type_traits::Const>{};

    template<>
    struct TMutability<type_traits::NonConst>{};

    using Mutability_Const = TMutability<type_traits::Const>;
    using Mutability_NonConst = TMutability<type_traits::NonConst>;
}

// --------------------------------------------------------------------------------------------------------------------
