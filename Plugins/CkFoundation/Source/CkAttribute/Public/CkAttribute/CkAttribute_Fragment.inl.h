#pragma once

#include "CkAttribute_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_AttributeType>
    TFragment_Attribute<T_AttributeType>::
        TFragment_Attribute(
            AttributeDataType InBase)
        : _Base(InBase)
        , _Final(InBase)
    {
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttribute>
    TFragment_AttributeModifier<T_DerivedAttribute>::
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
