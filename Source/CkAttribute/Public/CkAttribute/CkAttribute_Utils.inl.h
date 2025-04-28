#pragma once

#include "CkAttribute_Utils.h"

#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"
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
            AttributeHandleType& InHandle,
            const AttributeDataType& InBaseValue)
        -> void
    {
        InHandle.template Add<AttributeFragmentType>(InBaseValue);
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Has(
            const FCk_Handle& InHandle)
        -> bool
    {
        return InHandle.template Has<AttributeFragmentType>();
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Get_BaseValue(
            const AttributeHandleType& InHandle)
        -> AttributeDataType
    {
        return InHandle.template Get<AttributeFragmentType>().Get_Base();
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Get_FinalValue(
            const AttributeHandleType& InHandle)
        -> AttributeDataType
    {
        return InHandle.template Get<AttributeFragmentType>().Get_Final();
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Get_MayRequireReplicationThisFrame(
            const AttributeHandleType& InHandle)
        -> bool
    {
        return InHandle.template Has<typename AttributeFragmentType::FTag_MayRequireReplication>();
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Request_RecomputeFinalValue(
            AttributeHandleType& InHandle)
        -> void
    {
        InHandle.template AddOrGet<typename AttributeFragmentType::FTag_RecomputeFinalValue>();
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Request_TryClamp(
            AttributeHandleType& InHandle)
        -> void
    {
        InHandle.template AddOrGet<FTag_MayRequireClamping>();
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Request_FireSignals(
            AttributeHandleType& InHandle)
        -> void
    {
        InHandle.template AddOrGet<typename AttributeFragmentType::FTag_FireSignals>();
    }

    template <typename T_DerivedAttribute>
    auto
        TUtils_Attribute<T_DerivedAttribute>::
        Request_TryReplicateAttribute(
            AttributeHandleType& InHandle)
        -> void
    {
        if (NOT InHandle.template Has<FTag_ReplicatedAttribute>())
        { return; }

        InHandle.template AddOrGet<typename AttributeFragmentType::FTag_MayRequireReplication>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Add_Revocable(
            AttributeHandleType& InAttributeHandle,
            AttributeDataType InModifierDelta,
            ECk_AttributeModifier_Operation InModifierOperation,
            const FGameplayTag& InModifierGameplayLabel,
            ECk_AttributeValueChange_SyncPolicy InSyncPolicy)
        -> AttributeModifierHandleType
    {
        TUtils_Attribute<AttributeFragmentType>::Request_RecomputeFinalValue(InAttributeHandle);

        if (InSyncPolicy == ECk_AttributeValueChange_SyncPolicy::TrySyncToClients)
        { TUtils_Attribute<AttributeFragmentType>::Request_TryReplicateAttribute(InAttributeHandle); }

        return DoAddNewModifierToAttribute(InAttributeHandle, InModifierDelta, InModifierOperation, InModifierGameplayLabel, ECk_ModifierOperation_RevocablePolicy::Revocable);
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Add_NotRevocable(
            AttributeHandleType& InAttributeHandle,
            AttributeDataType InModifierDelta,
            ECk_AttributeModifier_Operation InModifierOperation,
            ECk_AttributeValueChange_SyncPolicy InSyncPolicy)
        -> void
    {
        if (auto MaybeExistingModifier = RecordOfAttributeModifiers_Utils::Get_ValidEntry_If(
            InAttributeHandle, ck::algo::MatchesAttributeModifierWithOperation<AttributeModifierFragmentType, typename AttributeModifierFragmentType::FTag_IsNotRevocableModification>{InModifierOperation});
            ck::IsValid(MaybeExistingModifier))
        {
            const auto& CurrentModifierValue = MaybeExistingModifier.template Get<AttributeModifierFragmentType>().Get_ModifierDelta();

            switch (InModifierOperation)
            {
                case ECk_AttributeModifier_Operation::Add:
                case ECk_AttributeModifier_Operation::Subtract:
                {
                    if (ck::IsValid(CurrentModifierValue))
                    {
                        Override(MaybeExistingModifier, TAttributeModifierOperators<AttributeDataType>::Add(*CurrentModifierValue, InModifierDelta), InSyncPolicy);
                    }
                    else
                    {
                        Override(MaybeExistingModifier, InModifierDelta, InSyncPolicy);
                    }
                    break;
                }
                case ECk_AttributeModifier_Operation::Multiply:
                case ECk_AttributeModifier_Operation::Divide:
                {
                    if (ck::IsValid(CurrentModifierValue))
                    {
                        Override(MaybeExistingModifier, TAttributeModifierOperators<AttributeDataType>::Mul(*CurrentModifierValue, InModifierDelta), InSyncPolicy);
                    }
                    else
                    {
                        Override(MaybeExistingModifier, InModifierDelta, InSyncPolicy);
                    }
                    break;
                }
                case ECk_AttributeModifier_Operation::Override:
                {
                    Override(MaybeExistingModifier, InModifierDelta, InSyncPolicy);
                    break;
                }
                default:
                {
                    CK_INVALID_ENUM(InModifierOperation);
                    break;
                }
            }

            return;
        }

        TUtils_Attribute<AttributeFragmentType>::Request_RecomputeFinalValue(InAttributeHandle);

        if (InSyncPolicy == ECk_AttributeValueChange_SyncPolicy::TrySyncToClients)
        { TUtils_Attribute<AttributeFragmentType>::Request_TryReplicateAttribute(InAttributeHandle); }

        std::ignore = DoAddNewModifierToAttribute(InAttributeHandle, InModifierDelta, InModifierOperation, FGameplayTag::EmptyTag, ECk_ModifierOperation_RevocablePolicy::NotRevocable);
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Request_ClearAllModifiers(
            AttributeHandleType& InAttributeHandle)
        -> void
    {
        static const auto TagsToIgnore = UCk_Utils_GameplayTag_UE::Get_GameplayTagContainerFromTags(
            FAttributeModifier_ReplicationTags::Get_BaseTag(), FAttributeModifier_ReplicationTags::Get_FinalTag());

        RecordOfAttributeModifiers_Utils::ForEach_Entry_If(InAttributeHandle, [](
            AttributeModifierHandleType InAttributeModifier)
            {
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InAttributeModifier);
            }, [](
            AttributeModifierHandleType InAttributeModifier)
            {
                if (UCk_Utils_GameplayLabel_UE::Has(InAttributeModifier) &&
                    UCk_Utils_GameplayLabel_UE::MatchesAny(InAttributeModifier, TagsToIgnore))
                { return false; }

                return true;
            });
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Override(
            AttributeModifierHandleType& InHandle,
            AttributeDataType InNewModifierDelta,
            ECk_AttributeValueChange_SyncPolicy InSyncPolicy)
        -> void
    {
        auto& ModifierFragment = InHandle.template Get<AttributeModifierFragmentType>();

        const auto& ModifierDelta = ModifierFragment.Get_ModifierDelta();

        if (ck::IsValid(ModifierDelta) && UCk_Utils_Arithmetic_UE::Get_IsNearlyEqual(*ModifierDelta, InNewModifierDelta))
        { return; }

        ModifierFragment._ModifierDelta = InNewModifierDelta;

        auto LifetimeOwnerAsAttributeEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner_AsTypeSafe<AttributeHandleType>(InHandle);

        TUtils_Attribute<AttributeFragmentType>::Request_RecomputeFinalValue(LifetimeOwnerAsAttributeEntity);

        if (InSyncPolicy == ECk_AttributeValueChange_SyncPolicy::TrySyncToClients)
        {
            TUtils_Attribute<AttributeFragmentType>::Request_TryReplicateAttribute(LifetimeOwnerAsAttributeEntity);;
        }
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Has(
            const FCk_Handle& InHandle)
        -> bool
    {
        return InHandle.template Has<AttributeModifierFragmentType>();
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Get_ModifierDeltaValue(
            const AttributeModifierHandleType& InHandle)
        -> const AttributeDataType&
    {
        return InHandle.template Get<AttributeModifierFragmentType>().Get_ModifierDelta().Get(AttributeDataType{});
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        Get_IsModifierUnique(
            const AttributeModifierHandleType& InHandle)
        -> ECk_Unique
    {
        const auto& AttributeEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        const auto& NameOfModifier = UCk_Utils_GameplayLabel_UE::Get_Label(InHandle);

        const auto& Count = RecordOfAttributeModifiers_Utils::Get_ValidEntriesCount_If(AttributeEntity, ck::algo::MatchesGameplayLabelExact{NameOfModifier});

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
            const AttributeModifierHandleType& InHandle)
        -> ECk_ArithmeticOperations_Basic
    {
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
            AttributeModifierHandleType& InHandle)
        -> void
    {
        InHandle.template AddOrGet<typename AttributeModifierFragmentType::FTag_ComputeResult>();
    }

    template <typename T_DerivedAttributeModifier>
    auto
        TUtils_AttributeModifier<T_DerivedAttributeModifier>::
        DoAddNewModifierToAttribute(
            AttributeHandleType& InAttributeHandle,
            AttributeDataType InModifierDelta,
            ECk_AttributeModifier_Operation InModifierOperation,
            const FGameplayTag& InModifierGameplayLabel,
            ECk_ModifierOperation_RevocablePolicy InRevocablePolicy)
        -> AttributeModifierHandleType
    {
        auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeHandle);
        auto NewModifierEntity = ck::StaticCast<AttributeModifierHandleType>(NewEntity);

        NewModifierEntity.template Add<AttributeModifierFragmentType>(InModifierDelta, InModifierOperation);
        NewModifierEntity.template Add<ECk_MinMaxCurrent>(AttributeFragmentType::ComponentTagType);

        switch (InModifierOperation)
        {
            case ECk_AttributeModifier_Operation::Add:
            {
                NewModifierEntity.template Add<typename AttributeModifierFragmentType::FTag_ModifyAdd>();
                break;
            }
            case ECk_AttributeModifier_Operation::Subtract:
            {
                NewModifierEntity.template Add<typename AttributeModifierFragmentType::FTag_ModifySubtract>();
                break;
            }
            case ECk_AttributeModifier_Operation::Multiply:
            {
                NewModifierEntity.template Add<typename AttributeModifierFragmentType::FTag_ModifyMultiply>();
                break;
            }
            case ECk_AttributeModifier_Operation::Divide:
            {
                NewModifierEntity.template Add<typename AttributeModifierFragmentType::FTag_ModifyDivide>();
                break;
            }
            case ECk_AttributeModifier_Operation::Override:
            {
                NewModifierEntity.template Add<typename AttributeModifierFragmentType::FTag_ModifyOverride>();
                break;
            }
            default:
            {
                CK_INVALID_ENUM(InModifierOperation);
                break;
            }
        }

        switch (InRevocablePolicy)
        {
            case ECk_ModifierOperation_RevocablePolicy::NotRevocable:
            {
                NewModifierEntity.template Add<typename AttributeModifierFragmentType::FTag_IsNotRevocableModification>();
                break;
            }
            case ECk_ModifierOperation_RevocablePolicy::Revocable:
            {
                NewModifierEntity.template Add<typename AttributeModifierFragmentType::FTag_IsRevocableModification>();
                break;
            }
            default:
            {
                CK_INVALID_ENUM(InModifierOperation);
                break;
            }
        }

        if (ck::IsValid(InModifierGameplayLabel))
        {
            UCk_Utils_GameplayLabel_UE::Add(NewModifierEntity, InModifierGameplayLabel);
        }

        RecordOfAttributeModifiers_Utils::AddIfMissing(InAttributeHandle, ECk_Record_EntryHandlingPolicy::Default);
        RecordOfAttributeModifiers_Utils::Request_Connect(InAttributeHandle, NewModifierEntity);

        return NewModifierEntity;
    }
}

// --------------------------------------------------------------------------------------------------------------------
