#pragma once

#include "CkAttribute_Processor.h"
#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/CkAttribute_Utils.h"
#include "CkCore/Payload/CkPayload.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedProcessor, typename T_DerivedAttribute, typename T_MulticastType>
    auto
        TProcessor_Attribute_FireSignals<T_DerivedProcessor, T_DerivedAttribute, T_MulticastType>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType& InAttribute) const
        -> void
    {
        InHandle.template Remove<MarkedDirtyBy>();

        attribute::VeryVerbose
        (
            TEXT("Dispatching Attribute Delegates of Entity [{}]"),
            InHandle
        );

        TUtils_Signal_OnAttributeValueChanged<T_DerivedAttribute, T_MulticastType>::Broadcast
        (
            InHandle,
            ck::MakePayload
            (
                InHandle,
                TPayload_Attribute_OnValueChanged<T_DerivedAttribute>
                {
                    InHandle,
                    InAttribute._Base,
                    InAttribute._Final
                }
            )
        );
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_AttributeModifierFragment>
    auto
        TProcessor_Attribute_RecomputeAll<T_DerivedProcessor, T_AttributeModifierFragment>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType& InAttribute) const
        -> void
    {
        InHandle.template Remove<MarkedDirtyBy>();

        attribute::VeryVerbose
        (
            TEXT("Resetting Attribute FinalValue of Entity [{}] and requesting a new computation from all its Attribute Modifiers."),
            InHandle
        );

        InAttribute._Final = InAttribute._Base;

        TUtils_AttributeModifier<AttributeModifierFragmentType>::RecordOfAttributeModifiers_Utils::ForEachEntry
        (
            InHandle,
            [&](auto InAttributeModifier) -> void
            {
                TUtils_AttributeModifier<AttributeModifierFragmentType>::Request_ComputeResult(InAttributeModifier);
            }
        );

        TUtils_Attribute<AttributeFragmentType>::Request_FireSignals(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_AttributeModifierFragment>
    auto
        TProcessor_AttributeModifier_Additive_Compute<T_DerivedProcessor ,T_AttributeModifierFragment>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier,
            const AttributeModifierTargetType& InAttributeTarget) const
        -> void
    {
        InHandle.template Remove<MarkedDirtyBy>();

        auto targetEntity = InAttributeTarget.Get_Entity();
        auto& attributeComp = targetEntity.Get<AttributeFragmentType>();

        attribute::VeryVerbose
        (
            TEXT("Computing Additive AttributeModifier for Entity [{}] to Attribute component of target Entity [{}]"),
            InHandle,
            targetEntity
        );

        attributeComp._Final = TAttributeModifierOperators<AttributeDataType>::Add(attributeComp._Final, InAttributeModifier._ModifierDelta);
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_AttributeModifierFragment>
    auto
        TProcessor_AttributeModifier_Additive_Teardown<T_DerivedProcessor, T_AttributeModifierFragment>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier,
            const AttributeModifierTargetType& InAttributeTarget) const
        -> void
    {
        auto targetEntity = InAttributeTarget.Get_Entity();

        attribute::VeryVerbose
        (
            TEXT("Removing Additive AttributeModifier value of Entity [{}] from Attribute component of target Entity [{}]. Forcing final value calculation again"),
            InHandle,
            targetEntity
        );

        TUtils_AttributeModifier<AttributeModifierFragmentType>::RecordOfAttributeModifiers_Utils::Request_Disconnect(targetEntity, InHandle);

        TUtils_Attribute<AttributeFragmentType>::Request_RecomputeFinalValue(targetEntity);
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_AttributeModifierFragment>
    auto
        TProcessor_AttributeModifier_Multiplicative_Compute<T_DerivedProcessor, T_AttributeModifierFragment>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier,
            const AttributeModifierTargetType& InAttributeTarget) const
        -> void
    {
        InHandle.template Remove<MarkedDirtyBy>();

        auto targetEntity = InAttributeTarget.Get_Entity();
        auto& attributeComp = targetEntity.Get<AttributeFragmentType>();

        attribute::VeryVerbose
        (
            TEXT("Computing Multiplicative AttributeModifier for Entity [{}] to Attribute component of target Entity [{}]"),
            InHandle,
            targetEntity
        );

        attributeComp._Final = TAttributeModifierOperators<AttributeDataType>::Multiply(attributeComp._Final, InAttributeModifier._ModifierDelta);
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_AttributeModifierFragment>
    auto
        TProcessor_AttributeModifier_Multiplicative_Teardown<T_DerivedProcessor, T_AttributeModifierFragment>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier,
            const AttributeModifierTargetType& InAttributeTarget) const
        -> void
    {
        auto targetEntity = InAttributeTarget.Get_Entity();

        attribute::VeryVerbose
        (
            TEXT("Removing Multiplicative AttributeModifier value of Entity [{}] from Attribute component of target Entity [{}]. Forcing final value calculation again"),
            InHandle,
            targetEntity
        );

        TUtils_AttributeModifier<AttributeModifierFragmentType>::RecordOfAttributeModifiers_Utils::Request_Disconnect(targetEntity, InHandle);

        TUtils_Attribute<AttributeFragmentType>::Request_RecomputeFinalValue(targetEntity);
    }
}

// --------------------------------------------------------------------------------------------------------------------
