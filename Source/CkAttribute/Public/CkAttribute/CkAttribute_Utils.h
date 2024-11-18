#pragma once

#include "CkAttribute_Fragment_Data.h"

#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"
#include "CkRecord/Record/CkRecord_Utils.h"
#include "CkSignal/CkSignal_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedAttribute>
    class TUtils_Attribute
    {
    public:
        using AttributeFragmentType              = T_DerivedAttribute;
        using AttributeFragmentPreviousValueType = TFragment_Attribute_PreviousValues<T_DerivedAttribute>;
        using AttributeDataType                  = typename AttributeFragmentType::AttributeDataType;
        using AttributeHandleType                = typename AttributeFragmentType::HandleType;

    public:
        template <typename>
        friend class TUtils_AttributeModifier;

        template <typename, typename, typename>
        friend class detail::TProcessor_Attribute_OverrideBaseValue;

        template <typename, typename>
        friend class detail::TProcessor_Attribute_RecomputeAll;

        template <typename, typename>
        friend class detail::TProcessor_AttributeModifier_Teardown;

        template <typename, typename, typename>
        friend class detail::TProcessor_Attribute_MinClamp;

        template <typename, typename, typename>
        friend class detail::TProcessor_Attribute_MaxClamp;

        template <typename, typename>
        friend class detail::TProcessor_Attribute_Refill;

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

    template <typename T_DerivedAttributeModifier>
    class TUtils_AttributeModifier
    {
    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using AttributeModifierHandleType   = typename AttributeModifierFragmentType::HandleType;
        using AttributeHandleType           = typename AttributeFragmentType::HandleType;

    public:
        template <typename, typename>
        friend class detail::TProcessor_Attribute_RecomputeAll;

        template <typename, typename>
        friend class detail::TProcessor_AttributeModifier_Teardown;

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
            ECk_AttributeValueChange_SyncPolicy InSyncPolicy = ECk_AttributeValueChange_SyncPolicy::TrySyncToClients) -> AttributeModifierHandleType;

        static auto
        Add_NotRevocable(
            AttributeHandleType& InAttributeHandle,
            AttributeDataType InModifierDelta,
            ECk_AttributeModifier_Operation InModifierOperation,
            ECk_AttributeValueChange_SyncPolicy InSyncPolicy = ECk_AttributeValueChange_SyncPolicy::TrySyncToClients) -> void;

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
            ECk_ModifierOperation_RevocablePolicy InRevocablePolicy) -> AttributeModifierHandleType;
    };
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkAttribute_Utils.inl.h"