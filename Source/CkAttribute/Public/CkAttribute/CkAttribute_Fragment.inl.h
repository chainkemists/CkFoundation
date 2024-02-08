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
        TFragment_AttributeModifier(
            AttributeDataType InModifierDelta)
        : _ModifierDelta(InModifierDelta)
    {
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttribute>
    TPayload_Attribute_OnValueChanged<T_DerivedAttribute>::
        TPayload_Attribute_OnValueChanged(
            HandleType InHandle,
            AttributeDataType InBaseValue,
            AttributeDataType InFinalValue)
        : _Handle(InHandle)
        , _BaseValue(InBaseValue)
        , _FinalValue(InFinalValue)
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------
