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

    template <typename T_HandleType, typename T_DerivedAttribute>
        TFragment_AttributeModifier<T_HandleType, T_DerivedAttribute>::
        TFragment_AttributeModifier()
        : _ModifierDelta(AttributeDataType{})
        , _Operation(ECk_AttributeModifier_Operation::Add)
    {
    }

    template <typename T_HandleType, typename T_DerivedAttribute>
        TFragment_AttributeModifier<T_HandleType, T_DerivedAttribute>::
        TFragment_AttributeModifier(
            AttributeDataType InModifierDelta,
            ECk_AttributeModifier_Operation InOperation)
        : _ModifierDelta(InModifierDelta)
        , _Operation(InOperation)
    {
    }

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
