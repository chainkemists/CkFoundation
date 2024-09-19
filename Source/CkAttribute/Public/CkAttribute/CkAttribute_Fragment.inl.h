#pragma once

#include "CkAttribute_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_HandleType, typename T_AttributeType, ECk_MinMaxCurrent T_ComponentTag>
    TFragment_Attribute<T_HandleType, T_AttributeType, T_ComponentTag>::
        TFragment_Attribute(
            AttributeDataType InBase)
        : _Base(InBase)
        , _Final(InBase)
    {
    }

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
