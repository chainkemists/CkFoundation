#pragma once

#include "CkAttribute_Utils.h"

#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkNet/CkNet_Utils.h"

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
        InHandle.template Add<AttributeFragmentType>(InBaseValue);
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Has(
            const HandleType& InHandle)
        -> bool
    {
        return InHandle.template Has<AttributeFragmentType>();
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

        return InHandle.template Get<AttributeFragmentType>().Get_Base();
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

        return InHandle.template Get<AttributeFragmentType>().Get_Final();
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Get_MayRequireReplicationThisFrame(
            const HandleType& InHandle)
        -> bool
    {
        if (NOT Ensure(InHandle))
        { return {}; }

        return InHandle.template Has<typename AttributeFragmentType::FTag_MayRequireReplication>();
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

        InHandle.template AddOrGet<typename AttributeFragmentType::FTag_RecomputeFinalValue>();
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Request_TryClamp(
            HandleType& InHandle)
        -> void
    {
        if (NOT Ensure(InHandle))
        { return; }

        InHandle.template AddOrGet<FTag_MayRequireClamping>();
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

        InHandle.template AddOrGet<typename AttributeFragmentType::FTag_FireSignals>();
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Request_TryReplicateAttribute(
            HandleType& InHandle)
            -> void
    {
        if (NOT Ensure(InHandle))
        { return; }

        if (NOT InHandle.template Has<FTag_ReplicatedAttribute>())
        { return; }

        if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(InHandle))
        { return; }

        InHandle.template AddOrGet<typename AttributeFragmentType::FTag_MayRequireReplication>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttribute>
    auto
        TUtils_AttributeRefill<T_DerivedAttribute>::
        Add(
            HandleType& InHandle,
            const AttributeDataType& InRefillRate,
            ECk_Attribute_RefillState InRefillState)
        -> void
    {
        InHandle.template Add<AttributeRefillFragmentType>(InRefillRate);
        Request_SetRefillState(InHandle, InRefillState);
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_AttributeRefill<T_DerivedAttribute>::
        Get_RefillState(
            HandleType& InHandle)
        -> ECk_Attribute_RefillState
    {
        if (NOT Ensure(InHandle))
        { return {}; }

        return InHandle.template Has<AttributeRefillFragmentType::FTag_RefillRunning>()
                ? ECk_Attribute_RefillState::Running
                : ECk_Attribute_RefillState::Paused;
    }

    template <typename T_DerivedAttributeRefill>
    auto
        TUtils_AttributeRefill<T_DerivedAttributeRefill>::
        Request_SetRefillState(
            HandleType& InHandle,
            ECk_Attribute_RefillState InRefillState)
        -> void
    {
        switch (InRefillState)
        {
            case ECk_Attribute_RefillState::Paused:
            {
                InHandle.template Try_Remove<AttributeRefillFragmentType::FTag_RefillRunning>();
                break;
            }
            case ECk_Attribute_RefillState::Running:
            {
                InHandle.template AddOrGet<AttributeRefillFragmentType::FTag_RefillRunning>();
                break;
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Add(
            HandleType& InHandle,
            AttributeDataType InModifierDelta,
            ECk_ArithmeticOperations_Basic InModifierOperation,
            ECk_ModifierOperation_RevocablePolicy InModifierOperationRevocablePolicy,
            ECk_AttributeValueChange_SyncPolicy InSyncPolicy)
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
        InHandle.template Add<ECk_MinMaxCurrent>(AttributeFragmentType::ComponentTagType);

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
            case ECk_ModifierOperation_RevocablePolicy::Override:
            {
                if (Utils_ExistingOverrideModifierEntity::Has(LifetimeOwnerAsAttributeEntity))
                {
                    if (auto MaybeExistingModifier = Utils_ExistingOverrideModifierEntity::Get_StoredEntity(LifetimeOwnerAsAttributeEntity);
                        ck::IsValid(MaybeExistingModifier))
                    {
                        MaybeExistingModifier.template Try_Remove<typename AttributeModifierFragmentType::FTag_ComputeResult>();
                        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(MaybeExistingModifier);
                    }

                    LifetimeOwnerAsAttributeEntity.template Remove<FExistingOverrideModifierEntity>();
                }

                Utils_ExistingOverrideModifierEntity::Add(LifetimeOwnerAsAttributeEntity, InHandle);

                InHandle.template Add<typename AttributeModifierFragmentType::FTag_IsOverrideModification>();
                RecordOfAttributeModifiers_Utils::AddIfMissing(LifetimeOwnerAsAttributeEntity, ECk_Record_EntryHandlingPolicy::Default);
                RecordOfAttributeModifiers_Utils::Request_Connect(LifetimeOwnerAsAttributeEntity, InHandle);

                Request_ComputeResult(InHandle);

                break;
            }
            case ECk_ModifierOperation_RevocablePolicy::NotRevocable:
            {
                InHandle.template Add<typename AttributeModifierFragmentType::FTag_IsNotRevocableModification>();
                RecordOfAttributeModifiers_Utils::AddIfMissing(LifetimeOwnerAsAttributeEntity, ECk_Record_EntryHandlingPolicy::Default);
                RecordOfAttributeModifiers_Utils::Request_Connect(LifetimeOwnerAsAttributeEntity, InHandle);

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

        switch (InSyncPolicy)
        {
            case ECk_AttributeValueChange_SyncPolicy::TrySyncToClients:
            {
                TUtils_Attribute<AttributeFragmentType>::Request_TryReplicateAttribute(LifetimeOwnerAsAttributeEntity);
                break;
            }
            case ECk_AttributeValueChange_SyncPolicy::DoNotSync:
            {
                break;
            }
            default:
            {
                CK_INVALID_ENUM(InSyncPolicy);
                break;
            }
        }

        TUtils_Attribute<AttributeFragmentType>::Request_RecomputeFinalValue(LifetimeOwnerAsAttributeEntity);
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Override(
            HandleType& InHandle,
            AttributeDataType InNewModifierDelta)
        -> void
    {
        if (NOT Ensure(InHandle))
        { return; }

        auto& ModifierFragment = InHandle.template Get<AttributeModifierFragmentType>();

        if (UCk_Utils_Arithmetic_UE::Get_IsNearlyEqual(ModifierFragment._ModifierDelta, InNewModifierDelta))
        { return; }

        ModifierFragment._ModifierDelta = InNewModifierDelta;

        auto LifetimeOwnerAsAttributeEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        TUtils_Attribute<AttributeFragmentType>::Request_RecomputeFinalValue(LifetimeOwnerAsAttributeEntity);
        TUtils_Attribute<AttributeFragmentType>::Request_TryReplicateAttribute(LifetimeOwnerAsAttributeEntity);
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
        if (NOT Ensure(InHandle))
        {
            static AttributeDataType Invalid;
            return Invalid;
        }

        return InHandle.template Get<AttributeModifierFragmentType>().Get_ModifierDelta();
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Get_IsModifierUnique(
            const HandleType& InHandle)
        -> ECk_Unique
    {
        if (NOT Ensure(InHandle))
        { return {}; }

        const auto& AttributeEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        const auto& NameOfModifier = UCk_Utils_GameplayLabel_UE::Get_Label(InHandle);

        const auto Predicate = [&](const HandleType& InCurrentModifier)
        {
            return UCk_Utils_GameplayLabel_UE::Matches(InCurrentModifier, NameOfModifier);
        };

        const auto& Count = RecordOfAttributeModifiers_Utils::Get_ValidEntriesCount_If(AttributeEntity, Predicate);

        if (Count == 0)
        { return ECk_Unique::DoesNotExist; }

        if (Count == 1)
        { return ECk_Unique::Unique; }

        return ECk_Unique::NotUnique;
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Get_ModifierOperation(
            const HandleType& InHandle)
        -> ECk_ArithmeticOperations_Basic
    {
        if (NOT Ensure(InHandle))
        { return {}; }

        if (InHandle.template Has<typename AttributeModifierFragmentType::FTag_ModifyAdd>())
        { return ECk_ArithmeticOperations_Basic::Add; }

        if (InHandle.template Has<typename AttributeModifierFragmentType::FTag_ModifySubtract>())
        { return ECk_ArithmeticOperations_Basic::Subtract; }

        if (InHandle.template Has<typename AttributeModifierFragmentType::FTag_ModifyMultiply>())
        { return ECk_ArithmeticOperations_Basic::Multiply; }

        if (InHandle.template Has<typename AttributeModifierFragmentType::FTag_ModifyDivide>())
        { return ECk_ArithmeticOperations_Basic::Divide; }

        CK_TRIGGER_ENSURE(TEXT("Attribute Modifier [{}] does not have any expected Operation tag"), InHandle);

        return {};
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
