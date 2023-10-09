#pragma once

#include "CkAttribute_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Add(
            HandleType InHandle,
            AttributeDataType InBaseValue)
        -> void
    {
        InHandle.Add<AttributeFragmentType>(InBaseValue);
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Has(
            HandleType InHandle)
        -> bool
    {
        return InHandle.Has<AttributeFragmentType>();
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Ensure(
            HandleType InHandle)
        -> bool
    {
        CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have an Attribute [{}]"), InHandle, ck::TypeToString<T_DerivedAttribute>)
        { return false; }

        return true;
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Get_BaseValue(
            HandleType InHandle)
        -> AttributeDataType
    {
        if (NOT Ensure(InHandle))
        { return {}; }

        return InHandle.Get<AttributeFragmentType>().Get_Base();
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Get_FinalValue(
            HandleType InHandle)
        -> AttributeDataType
    {
        if (NOT Ensure(InHandle))
        { return {}; }

        return InHandle.Get<AttributeFragmentType>().Get_Final();
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Request_RecomputeFinalValue(
            HandleType InHandle)
        -> void
    {
        if (NOT Ensure(InHandle))
        { return; }

        InHandle.Add<typename AttributeFragmentType::Tag_RecomputeFinalValue>();
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Request_FireSignals(
            HandleType InHandle)
        -> void
    {
        if (NOT Ensure(InHandle))
        { return; }

        InHandle.Add<typename AttributeFragmentType::Tag_FireSignals>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Add(
            HandleType            InHandle,
            AttributeDataType     InModifierDelta,
            HandleType            InTarget,
            ECk_ModifierOperation InModifierOperation,
            ECk_ModifierOperation_RevokablePolicy InModifierOperationRevokablePolicy)
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InTarget),
            TEXT("Target Entity [{}] is NOT a valid Entity when adding AttributeModifier to Handle [{}]"),
            InTarget,
            InHandle)
        { return; }

        CK_ENSURE_IF_NOT(TUtils_Attribute<AttributeFragmentType>::Has(InTarget),
            TEXT("AttributeModifier Entity [{}] is targeting Entity [{}] who does NOT have an Attribute component!"),
            InHandle,
            InTarget)
        { return; }

        InHandle.Add<AttributeModifierFragmentType>(InModifierDelta);

        AttributeModifierTarget_Utils::Add(InHandle, InTarget);

        switch (InModifierOperation)
        {
            case ECk_ModifierOperation::Additive:
            {
                InHandle.Add<typename AttributeModifierFragmentType::Tag_AdditiveModification>();
                break;
            }
            case ECk_ModifierOperation::Multiplicative:
            {
                InHandle.Add<typename AttributeModifierFragmentType::Tag_MultiplicativeModification>();
                break;
            }
            default:
            {
                CK_INVALID_ENUM(InModifierOperation);
                break;
            }
        }

        switch (InModifierOperationRevokablePolicy)
        {
            case ECk_ModifierOperation_RevokablePolicy::NotRevokable:
            {
                InHandle.Add<typename AttributeModifierFragmentType::Tag_IsNotRevokableModification>();
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);

                break;
            }
            case ECk_ModifierOperation_RevokablePolicy::Revokable:
            {
                InHandle.Add<typename AttributeModifierFragmentType::Tag_IsRevokableModification>();
                RecordOfAttributeModifiers_Utils::AddIfMissing(InTarget, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
                RecordOfAttributeModifiers_Utils::Request_Connect(InTarget, InHandle);

                break;
            }
            default:
            {
                CK_INVALID_ENUM(InModifierOperation);
                break;
            }
        }

        TUtils_Attribute<AttributeFragmentType>::Request_RecomputeFinalValue(InTarget);
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Has(
            HandleType InHandle)
        -> bool
    {
        return InHandle.Has<AttributeModifierFragmentType>();
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Ensure(
            HandleType InHandle)
        -> bool
    {
        CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have an AttributeModifier [{}]"), InHandle, ck::TypeToString<T_DerivedAttributeModifier>)
        { return false; }

        return true;
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Get_ModifierDeltaValue(
            HandleType InHandle)
        -> const AttributeDataType&
    {
        return InHandle.Get<AttributeModifierFragmentType>().Get_ModifierDelta();
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Request_ComputeResult(
            HandleType InHandle)
        -> void
    {
        if (NOT Ensure(InHandle))
        { return; }

        InHandle.Add<typename AttributeModifierFragmentType::Tag_ComputeResult>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
