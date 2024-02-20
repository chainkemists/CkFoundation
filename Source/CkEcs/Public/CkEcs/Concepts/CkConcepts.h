#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include <concepts>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::concepts
{
    template<typename T_HandleType>
    concept ValidHandleType = requires(T_HandleType T)
    {
        std::same_as<T_HandleType, FCk_Handle>;
        std::derived_from<T_HandleType, FCk_Handle_TypeSafe>;
    };
}
// --------------------------------------------------------------------------------------------------------------------