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
        using HandleType                         = FCk_Handle;

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
        friend class detail::TProcessor_AttributeRefill_Update;

    public:
        static auto
        Add(
            HandleType& InHandle,
            const AttributeDataType& InBaseValue) -> void;

        static auto
        Has(
            const HandleType& InHandle) -> bool;

        static auto
        Ensure(
            const HandleType& InHandle) -> bool;

        static auto
        Get_BaseValue(
            const HandleType& InHandle) -> AttributeDataType;

        static auto
        Get_FinalValue(
            const HandleType& InHandle) -> AttributeDataType;

        static auto
        Get_MayRequireReplicationThisFrame(
            const HandleType& InHandle) -> bool;

    private:
        static auto
        Request_RecomputeFinalValue(
            HandleType& InHandle) -> void;

        static auto
        Request_TryClamp(
            HandleType& InHandle) -> void;

        static auto
        Request_FireSignals(
            HandleType& InHandle) -> void;

        static auto
        Request_TryReplicateAttribute(
            HandleType& InHandle) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttributeRefill>
    class TUtils_AttributeRefill
    {
    public:
        using AttributeRefillFragmentType = T_DerivedAttributeRefill;
        using AttributeFragmentType       = typename AttributeRefillFragmentType::AttributeFragmentType;
        using AttributeDataType           = typename AttributeFragmentType::AttributeDataType;
        using HandleType                  = typename AttributeFragmentType::HandleType;

    public:
        static auto
        Add(
            HandleType& InHandle,
            const AttributeDataType& InRefillRate,
            ECk_Attribute_RefillState InRefillState) -> void;

        static auto
        Get_RefillState(
            HandleType& InHandle) -> ECk_Attribute_RefillState;

    public:
        static auto
        Request_SetRefillState(
            HandleType& InHandle,
            ECk_Attribute_RefillState InRefillState) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttributeModifier>
    class TUtils_AttributeModifier
    {
    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using HandleType                    = typename AttributeModifierFragmentType::HandleType;
        using AttributeHandleType           = typename AttributeFragmentType::HandleType;

    public:
        template <typename, typename>
        friend class detail::TProcessor_Attribute_RecomputeAll;

        template <typename, typename>
        friend class detail::TProcessor_AttributeModifier_Teardown;

    public:
        struct DerivedRecordType : TFragment_RecordOfAttributeModifiers<HandleType>{
            using TFragment_RecordOfAttributeModifiers<HandleType>::TFragment_RecordOfAttributeModifiers;};

        struct RecordOfAttributeModifiers_Utils : TUtils_RecordOfEntities<DerivedRecordType>{};
        struct RecordOfAttributeModifiersTransient_Utils : TUtils_RecordOfEntities<DerivedRecordType>{};

    public:
        static auto
        Add_Revocable(
            AttributeHandleType& InAttributeHandle,
            AttributeDataType InModifierDelta,
            ECk_AttributeModifier_Operation InModifierOperation,
            ECk_AttributeValueChange_SyncPolicy InSyncPolicy = ECk_AttributeValueChange_SyncPolicy::TrySyncToClients) -> HandleType;

        static auto
        Add_NotRevocable(
            AttributeHandleType& InAttributeHandle,
            AttributeDataType InModifierDelta,
            ECk_AttributeModifier_Operation InModifierOperation,
            ECk_AttributeValueChange_SyncPolicy InSyncPolicy = ECk_AttributeValueChange_SyncPolicy::TrySyncToClients) -> void;

        static auto
        Override(
            HandleType& InHandle,
            AttributeDataType InNewModifierDelta,
            ECk_AttributeValueChange_SyncPolicy InSyncPolicy = ECk_AttributeValueChange_SyncPolicy::TrySyncToClients) -> void;

        static auto
        Has(
            const FCk_Handle& InHandle) -> bool;

        static auto
        Get_ModifierDeltaValue(
            const HandleType& InHandle) -> const AttributeDataType&;

        static auto
        Get_IsModifierUnique(
            const HandleType& InHandle) -> ECk_Unique;

        static auto
        Get_ModifierOperation(
            const HandleType& InHandle) -> ECk_ArithmeticOperations_Basic;

    private:
        static auto
        Request_ComputeResult(
            HandleType& InHandle) -> void;

        static auto
        DoAddNewModifierToAttribute(
            AttributeHandleType& InAttributeHandle,
            AttributeDataType InModifierDelta,
            ECk_AttributeModifier_Operation InModifierOperation,
            ECk_ModifierOperation_RevocablePolicy InRevocablePolicy) -> HandleType;
    };
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkAttribute_Utils.inl.h"