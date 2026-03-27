#pragma once

#include "CkAttribute_Fragment_Data.h"

#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"
#include "CkRecord/Record/CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <concepts::ValidAttributeFragment T_DerivedAttribute>
    class TUtils_Attribute
    {
    public:
        using AttributeFragmentType              = T_DerivedAttribute;
        using AttributeFragmentPreviousValueType = TFragment_Attribute_PreviousValues<T_DerivedAttribute>;
        using AttributeDataType                  = typename AttributeFragmentType::AttributeDataType;
        using AttributeHandleType                = typename AttributeFragmentType::HandleType;

    public:
        template <concepts::ValidAttributeModifierFragment>
        friend class TUtils_AttributeModifier;

        template <typename, concepts::ValidAttributeFragment, concepts::ValidAttributeFragment>
        friend class detail::TProcessor_Attribute_OverrideBaseValue;

        template <typename, concepts::ValidAttributeModifierFragment>
        friend class detail::TProcessor_Attribute_RecomputeAll;

        template <typename, concepts::ValidAttributeModifierFragment>
        friend class detail::TProcessor_AttributeModifier_EndPlay;

        template <typename, concepts::ValidAttributeFragment, concepts::ValidAttributeFragment, ECk_AttributeClamp_Direction>
        friend class detail::TProcessor_Attribute_Clamp;

        template <typename, concepts::ValidAttributeModifierFragment, ECk_Attribute_Refill_Policy>
        friend class detail::TProcessor_Attribute_Refill_Impl;

    public:
        static auto
        Add(
            AttributeHandleType& InHandle,
            const AttributeDataType& InBaseValue) -> void;

        static auto
        Has(
            const FCk_Handle& InHandle) -> bool;

        static auto
        Get_BaseValue(
            const AttributeHandleType& InHandle) -> AttributeDataType;

        static auto
        Get_FinalValue(
            const AttributeHandleType& InHandle) -> AttributeDataType;

        static auto
        Get_MayRequireReplicationThisFrame(
            const AttributeHandleType& InHandle) -> bool;

    private:
        static auto
        Request_RecomputeFinalValue(
            AttributeHandleType& InHandle) -> void;

        static auto
        Request_TryClamp(
            AttributeHandleType& InHandle) -> void;

        static auto
        Request_FireSignals(
            AttributeHandleType& InHandle) -> void;

        static auto
        Request_TryReplicateAttribute(
            AttributeHandleType& InHandle) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <concepts::ValidAttributeRefillHandleType T_RefillHandleType>
    using TUtils_RefillAttribute = TUtils_EntityHolder<TFragment_RefillAttribute<T_RefillHandleType>>;

    template <concepts::ValidAttributeHandleType T_AttributeHandleType>
    using TUtils_RefillAttributeTarget = TUtils_EntityHolder<TFragment_RefillAttributeTarget<T_AttributeHandleType>>;

    // --------------------------------------------------------------------------------------------------------------------

    template <concepts::ValidAttributeModifierFragment T_DerivedAttributeModifier>
    class TUtils_AttributeModifier
    {
    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using AttributeModifierHandleType   = typename AttributeModifierFragmentType::HandleType;
        using AttributeHandleType           = typename AttributeFragmentType::HandleType;

    public:
        template <typename, concepts::ValidAttributeModifierFragment>
        friend class detail::TProcessor_Attribute_RecomputeAll;

        template <typename, concepts::ValidAttributeModifierFragment>
        friend class detail::TProcessor_AttributeModifier_EndPlay;

    public:
        struct DerivedRecordType : TFragment_RecordOfAttributeModifiers<AttributeModifierHandleType>{
            using TFragment_RecordOfAttributeModifiers<AttributeModifierHandleType>::TFragment_RecordOfAttributeModifiers;};

        struct RecordOfAttributeModifiers_Utils : TUtils_RecordOfEntities<DerivedRecordType>{};

    public:
        static auto
        Add_Revocable(
            AttributeHandleType& InAttributeHandle,
            AttributeDataType InModifierDelta,
            ECk_AttributeModifier_Operation InModifierOperation,
            const FGameplayTag& InModifierGameplayLabel,
            ECk_AttributeValueChange_SyncPolicy InSyncPolicy = ECk_AttributeValueChange_SyncPolicy::TrySyncToClients) -> AttributeModifierHandleType;

        static auto
        Add_NotRevocable(
            AttributeHandleType& InAttributeHandle,
            AttributeDataType InModifierDelta,
            ECk_AttributeModifier_Operation InModifierOperation,
            ECk_AttributeValueChange_SyncPolicy InSyncPolicy = ECk_AttributeValueChange_SyncPolicy::TrySyncToClients) -> void;

        static auto
        Request_ClearAllModifiers(
            AttributeHandleType& InAttributeHandle) -> void;

        static auto
        Override(
            AttributeModifierHandleType& InHandle,
            AttributeDataType InNewModifierDelta,
            ECk_AttributeValueChange_SyncPolicy InSyncPolicy = ECk_AttributeValueChange_SyncPolicy::TrySyncToClients) -> void;

        static auto
        Has(
            const FCk_Handle& InHandle) -> bool;

        static auto
        Get_ModifierDeltaValue(
            const AttributeModifierHandleType& InHandle) -> const AttributeDataType&;

        static auto
        Get_IsModifierUnique(
            const AttributeModifierHandleType& InHandle) -> ECk_Unique;

        static auto
        Get_ModifierOperation(
            const AttributeModifierHandleType& InHandle) -> ECk_ArithmeticOperations_Basic;

    private:
        static auto
        Request_ComputeResult(
            AttributeModifierHandleType& InHandle) -> void;

        static auto
        DoAddNewModifierToAttribute(
            AttributeHandleType& InAttributeHandle,
            AttributeDataType InModifierDelta,
            ECk_AttributeModifier_Operation InModifierOperation,
            const FGameplayTag& InModifierGameplayLabel,
            ECk_ModifierOperation_RevocablePolicy InRevocablePolicy) -> AttributeModifierHandleType;
    };
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkAttribute_Utils.inl.h"