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
            HandleType& InHandle,
            const AttributeDataType& InBaseValue)
        -> void
    {
        InHandle.Add<AttributeFragmentType>(InBaseValue);
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Has(
            const HandleType& InHandle)
        -> bool
    {
        return InHandle.Has<AttributeFragmentType>();
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Ensure(
            const HandleType& InHandle)
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
            const HandleType& InHandle)
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
            const HandleType& InHandle)
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
            HandleType& InHandle)
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
            HandleType& InHandle)
        -> void
    {
        if (NOT Ensure(InHandle))
        { return; }

        InHandle.AddOrGet<typename AttributeFragmentType::FTag_FireSignals>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Add(
            HandleType& InHandle,
            AttributeDataType InModifierDelta,
            ECk_ArithmeticOperations_Basic InModifierOperation,
            ECk_ModifierOperation_RevocablePolicy InModifierOperationRevocablePolicy)
        -> void
    {
        // Attribute fragments live on Entity (A) and AttributeModifiers live on Entities UNDER Entity (A)
        // Here LifetimeOwner is Entity (A)
        auto LifetimeOwnerAsAttributeEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(LifetimeOwnerAsAttributeEntity),
            TEXT("Target Entity [{}] is NOT a valid Entity when adding AttributeModifier to Handle [{}]"),
            LifetimeOwnerAsAttributeEntity,
            InHandle)
        { return; }

        CK_ENSURE_IF_NOT(TUtils_Attribute<AttributeFragmentType>::Has(LifetimeOwnerAsAttributeEntity),
            TEXT("AttributeModifier Entity [{}] is targeting Entity [{}] who does NOT have an Attribute component!"),
            InHandle,
            LifetimeOwnerAsAttributeEntity)
        { return; }

        InHandle.template Add<AttributeModifierFragmentType>(InModifierDelta);

        switch (InModifierOperation)
        {
            case ECk_ArithmeticOperations_Basic::Add:
            {
                InHandle.template Add<typename AttributeModifierFragmentType::FTag_ModifyAdd>();
                break;
            }
            case ECk_ArithmeticOperations_Basic::Subtract:
            {
                InHandle.template Add<typename AttributeModifierFragmentType::FTag_ModifySubtract>();
                break;
            }
            case ECk_ArithmeticOperations_Basic::Multiply:
            {
                InHandle.template Add<typename AttributeModifierFragmentType::FTag_ModifyMultiply>();
                break;
            }
            case ECk_ArithmeticOperations_Basic::Divide:
            {
                InHandle.template Add<typename AttributeModifierFragmentType::FTag_ModifyDivide>();
                break;
            }
            default:
            {
                CK_INVALID_ENUM(InModifierOperation);
                break;
            }
        }

        switch (InModifierOperationRevocablePolicy)
        {
            case ECk_ModifierOperation_RevocablePolicy::NotRevocable:
            {
                InHandle.template Add<typename AttributeModifierFragmentType::FTag_IsNotRevocableModification>();
                Request_ComputeResult(InHandle);

                break;
            }
            case ECk_ModifierOperation_RevocablePolicy::Revocable:
            {
                InHandle.template Add<typename AttributeModifierFragmentType::FTag_IsRevocableModification>();
                RecordOfAttributeModifiers_Utils::AddIfMissing(LifetimeOwnerAsAttributeEntity, ECk_Record_EntryHandlingPolicy::Default);
                RecordOfAttributeModifiers_Utils::Request_Connect(LifetimeOwnerAsAttributeEntity, InHandle);

                break;
            }
            default:
            {
                CK_INVALID_ENUM(InModifierOperation);
                break;
            }
        }

        TUtils_Attribute<AttributeFragmentType>::Request_RecomputeFinalValue(LifetimeOwnerAsAttributeEntity);
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Has(
            const HandleType& InHandle)
        -> bool
    {
        return InHandle.template Has<AttributeModifierFragmentType>();
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Ensure(
            const HandleType& InHandle)
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
            const HandleType& InHandle)
        -> const AttributeDataType&
    {
        return InHandle.template Get<AttributeModifierFragmentType>().Get_ModifierDelta();
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Request_ComputeResult(
            HandleType& InHandle)
        -> void
    {
        if (NOT Ensure(InHandle))
        { return; }

        InHandle.template AddOrGet<typename AttributeModifierFragmentType::FTag_ComputeResult>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
