#pragma once

#include "CkAttribute_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Add(
            HandleType InHandle,
            const AttributeDataType& InBaseValue)
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

        InHandle.AddOrGet<typename AttributeFragmentType::FTag_RecomputeFinalValue>();
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

        InHandle.Add<typename AttributeFragmentType::FTag_FireSignals>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Add(
            HandleType            InHandle,
            AttributeDataType     InModifierDelta,
            ECk_ModifierOperation InModifierOperation,
            ECk_ModifierOperation_RevocablePolicy InModifierOperationRevokablePolicy)
        -> void
    {
        // Attribute fragments live on Entity (A) and AttributeModifiers live on Entities UNDER Entity (A)
        // Here LifetimeOwner is Entity (A)
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(LifetimeOwner),
            TEXT("Target Entity [{}] is NOT a valid Entity when adding AttributeModifier to Handle [{}]"),
            LifetimeOwner,
            InHandle)
        { return; }

        CK_ENSURE_IF_NOT(TUtils_Attribute<AttributeFragmentType>::Has(LifetimeOwner),
            TEXT("AttributeModifier Entity [{}] is targeting Entity [{}] who does NOT have an Attribute component!"),
            InHandle,
            LifetimeOwner)
        { return; }

        InHandle.template Add<AttributeModifierFragmentType>(InModifierDelta);

        switch (InModifierOperation)
        {
            case ECk_ModifierOperation::Additive:
            {
                InHandle.template Add<typename AttributeModifierFragmentType::FTag_AdditiveModification>();
                break;
            }
            case ECk_ModifierOperation::Multiplicative:
            {
                InHandle.template Add<typename AttributeModifierFragmentType::FTag_MultiplicativeModification>();
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
            case ECk_ModifierOperation_RevocablePolicy::NotRevocable:
            {
                InHandle.template Add<typename AttributeModifierFragmentType::FTag_IsNotRevokableModification>();
                Request_ComputeResult(InHandle);

                break;
            }
            case ECk_ModifierOperation_RevocablePolicy::Revocable:
            {
                InHandle.template Add<typename AttributeModifierFragmentType::FTag_IsRevokableModification>();
                RecordOfAttributeModifiers_Utils::AddIfMissing(LifetimeOwner, ECk_Record_EntryHandlingPolicy::Default);
                RecordOfAttributeModifiers_Utils::Request_Connect(LifetimeOwner, InHandle);

                break;
            }
            default:
            {
                CK_INVALID_ENUM(InModifierOperation);
                break;
            }
        }

        TUtils_Attribute<AttributeFragmentType>::Request_RecomputeFinalValue(LifetimeOwner);
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Has(
            HandleType InHandle)
        -> bool
    {
        return InHandle.template Has<AttributeModifierFragmentType>();
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
        return InHandle.template Get<AttributeModifierFragmentType>().Get_ModifierDelta();
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

        InHandle.template Add<typename AttributeModifierFragmentType::FTag_ComputeResult>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
