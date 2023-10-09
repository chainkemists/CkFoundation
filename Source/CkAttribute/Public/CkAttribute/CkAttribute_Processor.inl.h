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
            const TimeType&,
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

        const auto& AttributeOwningEntity = UCk_Utils_OwningEntity::Get_StoredEntity(InHandle);

        TUtils_Signal_OnAttributeValueChanged<T_DerivedAttribute, T_MulticastType>::Broadcast
        (
            InHandle,
            ck::MakePayload
            (
                AttributeOwningEntity,
                TPayload_Attribute_OnValueChanged<T_DerivedAttribute>
                {
                    AttributeOwningEntity,
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
            const TimeType&,
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

        if (TUtils_AttributeModifier<AttributeModifierFragmentType>::RecordOfAttributeModifiers_Utils::Has(InHandle))
        {
            TUtils_AttributeModifier<AttributeModifierFragmentType>::RecordOfAttributeModifiers_Utils::ForEachEntry
            (
                InHandle,
                [&](auto InAttributeModifier) -> void
                {
                    TUtils_AttributeModifier<AttributeModifierFragmentType>::Request_ComputeResult(InAttributeModifier);
                }
            );
        }

        TUtils_Attribute<AttributeFragmentType>::Request_FireSignals(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_AttributeModifierFragment>
    auto
        TProcessor_AttributeModifier_RevokableAdditive_Compute<T_DerivedProcessor ,T_AttributeModifierFragment>::
        ForEachEntity(
            const TimeType&,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier,
            const AttributeModifierTargetType& InAttributeTarget) const
        -> void
    {
        InHandle.template Remove<MarkedDirtyBy>();

        auto TargetEntity = InAttributeTarget.Get_Entity();
        auto& AttributeComp = TargetEntity.Get<AttributeFragmentType>();

        attribute::VeryVerbose
        (
            TEXT("Computing REVOKABLE Additive AttributeModifier for Entity [{}] to Attribute component of target Entity [{}]"),
            InHandle,
            TargetEntity
        );

        AttributeComp._Final = TAttributeModifierOperators<AttributeDataType>::Add(AttributeComp._Final, InAttributeModifier.Get_ModifierDelta());
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    auto
        TProcessor_AttributeModifier_NotRevokableAdditive_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier,
            const AttributeModifierTargetType& InAttributeTarget) const -> void
    {
        InHandle.template Remove<MarkedDirtyBy>();

        auto TargetEntity = InAttributeTarget.Get_Entity();
        auto& AttributeComp = TargetEntity.Get<AttributeFragmentType>();

        attribute::VeryVerbose
        (
            TEXT("Computing NOT REVOKABLE Additive AttributeModifier for Entity [{}] to Attribute component of target Entity [{}]"),
            InHandle,
            TargetEntity
        );

        AttributeComp._Base = TAttributeModifierOperators<AttributeDataType>::Add(AttributeComp._Base, InAttributeModifier.Get_ModifierDelta());
        AttributeComp._Final = AttributeComp._Base;
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_AttributeModifierFragment>
    auto
        TProcessor_AttributeModifier_Additive_Teardown<T_DerivedProcessor, T_AttributeModifierFragment>::
        ForEachEntity(
            const TimeType&,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier,
            const AttributeModifierTargetType& InAttributeTarget) const
        -> void
    {
        auto TargetEntity = InAttributeTarget.Get_Entity();

        attribute::VeryVerbose
        (
            TEXT("Removing REVOKABLE Additive AttributeModifier value of Entity [{}] from Attribute component of target Entity [{}]. Forcing final value calculation again"),
            InHandle,
            TargetEntity
        );

        TUtils_AttributeModifier<AttributeModifierFragmentType>::RecordOfAttributeModifiers_Utils::Request_Disconnect(TargetEntity, InHandle);

        TUtils_Attribute<AttributeFragmentType>::Request_RecomputeFinalValue(TargetEntity);
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_AttributeModifierFragment>
    auto
        TProcessor_AttributeModifier_RevokableMultiplicative_Compute<T_DerivedProcessor, T_AttributeModifierFragment>::
        ForEachEntity(
            const TimeType&,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier,
            const AttributeModifierTargetType& InAttributeTarget) const
        -> void
    {
        InHandle.template Remove<MarkedDirtyBy>();

        auto TargetEntity = InAttributeTarget.Get_Entity();
        auto& AttributeComp = TargetEntity.Get<AttributeFragmentType>();

        attribute::VeryVerbose
        (
            TEXT("Computing REVOKABLE Multiplicative AttributeModifier for Entity [{}] to Attribute component of target Entity [{}]"),
            InHandle,
            TargetEntity
        );

        AttributeComp._Final = TAttributeModifierOperators<AttributeDataType>::Multiply(AttributeComp._Final, InAttributeModifier.Get_ModifierDelta());
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_AttributeModifierFragment>
    auto
        TProcessor_AttributeModifier_NotRevokableMultiplicative_Compute<T_DerivedProcessor, T_AttributeModifierFragment>::
        ForEachEntity(
            const TimeType&,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier,
            const AttributeModifierTargetType& InAttributeTarget) const
        -> void
    {
        InHandle.template Remove<MarkedDirtyBy>();

        auto TargetEntity = InAttributeTarget.Get_Entity();
        auto& AttributeComp = TargetEntity.Get<AttributeFragmentType>();

        attribute::VeryVerbose
        (
            TEXT("Computing NOT REVOKABLE Multiplicative AttributeModifier for Entity [{}] to Attribute component of target Entity [{}]"),
            InHandle,
            TargetEntity
        );

        AttributeComp._Base = TAttributeModifierOperators<AttributeDataType>::Multiply(AttributeComp._Base, InAttributeModifier.Get_ModifierDelta());
        AttributeComp._Final = AttributeComp._Base;
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_AttributeModifierFragment>
    auto
        TProcessor_AttributeModifier_Multiplicative_Teardown<T_DerivedProcessor, T_AttributeModifierFragment>::
        ForEachEntity(
            const TimeType&,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier,
            const AttributeModifierTargetType& InAttributeTarget) const
        -> void
    {
        auto TargetEntity = InAttributeTarget.Get_Entity();

        attribute::VeryVerbose
        (
            TEXT("Removing REVOKABLE Multiplicative AttributeModifier value of Entity [{}] from Attribute component of target Entity [{}]. Forcing final value calculation again"),
            InHandle,
            TargetEntity
        );

        TUtils_AttributeModifier<AttributeModifierFragmentType>::RecordOfAttributeModifiers_Utils::Request_Disconnect(TargetEntity, InHandle);

        TUtils_Attribute<AttributeFragmentType>::Request_RecomputeFinalValue(TargetEntity);
    }
}

// --------------------------------------------------------------------------------------------------------------------
