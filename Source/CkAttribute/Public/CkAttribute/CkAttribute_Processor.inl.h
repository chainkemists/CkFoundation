#pragma once

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/CkAttribute_Processor.h"
#include "CkAttribute/CkAttribute_Utils.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Payload/CkPayload.h"

#include "CkEcs/Net/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::detail
{
    template <typename T_DerivedProcessor, concepts::ValidAttributeFragment T_DerivedAttribute>
    auto
        TProcessor_Attribute_StorePreviousValue<T_DerivedProcessor, T_DerivedAttribute>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType& InAttribute) const
        -> void
    {
        using AttributePreviousType = TFragment_Attribute_PreviousValues<AttributeFragmentType>;

        auto& PreviousValue = InHandle.template AddOrGet<AttributePreviousType>();
        PreviousValue = AttributePreviousType{InAttribute.Get_Base(), InAttribute.Get_Final()};
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, concepts::ValidAttributeFragment T_DerivedAttribute, typename T_MulticastType>
    auto
        TProcessor_Attribute_FireSignals_ValueChanged<T_DerivedProcessor, T_DerivedAttribute, T_MulticastType>::
        ForEachEntity(
            const TimeType&,
            HandleType InHandle,
            AttributeFragmentType& InAttribute,
            AttributeFragmentPreviousType& InAttributePrevious) const
        -> void
    {
        InHandle.template Remove<MarkedDirtyBy>();

        if (InAttribute.Get_Base() == InAttributePrevious.Get_Base() && InAttribute.Get_Final() == InAttributePrevious.Get_Final())
        { return; }

        attribute::VeryVerbose
        (
            TEXT("Dispatching Delegates for [{}] AttributeComponent of Attribute Entity [{}]"),
            AttributeFragmentType::ComponentTagType,
            InHandle
        );

        const auto& AttributeLifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        TUtils_Signal_OnAttributeValueChanged<T_DerivedAttribute, T_MulticastType>::Broadcast
        (
            InHandle,
            ck::MakePayload
            (
                AttributeLifetimeOwner,
                TPayload_Attribute_OnValueChanged<T_DerivedAttribute>
                {
                    InHandle,
                    InAttribute.Get_Base(),
                    InAttribute.Get_Final(),

                    InAttributePrevious.Get_Base(),
                    InAttributePrevious.Get_Final()
                }
            )
        );
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, concepts::ValidAttributeFragment T_DerivedAttributeCurrent, concepts::ValidAttributeFragment T_DerivedAttributeBound,
              typename T_MulticastType, ECk_AttributeClamp_Direction T_Direction>
    auto
        TProcessor_Attribute_FireSignals_Clamped<T_DerivedProcessor, T_DerivedAttributeCurrent,
        T_DerivedAttributeBound, T_MulticastType, T_Direction>::ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType_Current& InAttribute_Current,
            AttributeFragmentType_Bound& InAttribute_Bound) const
        -> void
    {
        InHandle.template Remove<MarkedDirtyBy>();

        auto AreAllComponentsUnchanged = true;

        if (InHandle.template Has<AttributeFragmentPreviousType_Current>())
        {
            auto& PreviousValue = InHandle.template Get<AttributeFragmentPreviousType_Current>();
            AreAllComponentsUnchanged &= InAttribute_Current.Get_Base() == PreviousValue.Get_Base() && InAttribute_Current.Get_Final() == PreviousValue.Get_Final();
        }
        if (InHandle.template Has<AttributeFragmentPreviousType_Bound>())
        {
            auto& PreviousValue = InHandle.template Get<AttributeFragmentPreviousType_Bound>();
            AreAllComponentsUnchanged &= InAttribute_Bound.Get_Base() == PreviousValue.Get_Base() && InAttribute_Bound.Get_Final() == PreviousValue.Get_Final();
        }
        if (AreAllComponentsUnchanged)
        { return; }

        if (InAttribute_Current.Get_Final() == InAttribute_Bound.Get_Final())
        {
            const auto& AttributeLifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

            if constexpr (T_Direction == ECk_AttributeClamp_Direction::Min)
            {
                attribute::VeryVerbose
                (
                    TEXT("Dispatching Delegates for MinClamp Attribute Entity [{}]"),
                    InHandle
                );
            }
            else
            {
                attribute::VeryVerbose
                (
                    TEXT("Dispatching Delegates for MaxClamp Attribute Entity [{}]"),
                    InHandle
                );
            }

            TUtils_Signal_OnAttributeClamped<T_DerivedAttributeCurrent, T_DerivedAttributeBound, T_MulticastType>::Broadcast
            (
                InHandle,
                ck::MakePayload
                (
                    AttributeLifetimeOwner,
                    TPayload_Attribute_OnClamped<T_DerivedAttributeCurrent>
                    {
                        InHandle,
                        InAttribute_Current.Get_Final()
                    }
                )
            );
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, concepts::ValidAttributeFragment T_DerivedAttributeCurrent, concepts::ValidAttributeFragment T_DerivedAttributeBound,
              ECk_AttributeClamp_Direction T_Direction>
    auto
        TProcessor_Attribute_Clamp<T_DerivedProcessor, T_DerivedAttributeCurrent, T_DerivedAttributeBound, T_Direction>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType_Current& InAttributeCurrent,
            const AttributeFragmentType_Bound& InAttributeBound) const
        -> void
    {
        const auto BaseValue = InAttributeCurrent._Base;
        const auto FinalValue = InAttributeCurrent._Final;

        using Current_AttributePreviousType = ck::TFragment_Attribute_PreviousValues<T_DerivedAttributeCurrent>;

        if constexpr (T_Direction == ECk_AttributeClamp_Direction::Min)
        {
            // If the Current has not been changed yet, but the min was then this processor will run without a PreviousValue existing for Current
            if (NOT InHandle.template Has<Current_AttributePreviousType>())
            { InHandle.template Add<Current_AttributePreviousType>(InAttributeCurrent.Get_Base(), InAttributeCurrent.Get_Final()); }

            const auto& PreviousValue = InHandle.template Get<Current_AttributePreviousType>();

            // Clamping on the client side is bypassed because the server might update both 'Min' and 'Current' values.
            // In cases where 'Current' needs to match the new 'Min', if the client receives 'Current' before 'Min' and clamps it,
            // the value could be incorrectly constrained to the previous 'Min' when 'Min' is replicated after clamping.
            // However, if the attribute is refilling and the change does not require replication, client-side clamping is NOT bypassed.
            if (InHandle.template Has<ck::FTag_ReplicatedAttribute>() &&
                NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InHandle) &&
                TUtils_Attribute<T_DerivedAttributeCurrent>::Get_MayRequireReplicationThisFrame(InHandle))
            {
                if (PreviousValue.Get_Base() != BaseValue || PreviousValue.Get_Final() != FinalValue)
                { TUtils_Attribute<T_DerivedAttributeCurrent>::Request_FireSignals(InHandle); }

                return;
            }

            const auto FinalValue_Bound = InAttributeBound._Final;

            InAttributeCurrent._Base = TAttributeMinMax<AttributeDataType>::Max(BaseValue, FinalValue_Bound);
            InAttributeCurrent._Final = TAttributeMinMax<AttributeDataType>::Max(FinalValue, FinalValue_Bound);

            if (PreviousValue.Get_Base() != BaseValue || PreviousValue.Get_Final() != FinalValue)
            { TUtils_Attribute<T_DerivedAttributeCurrent>::Request_FireSignals(InHandle); }
        }
        else // Max
        {
            // If the Current has not been changed yet, but the Max was then this processor will run without a PreviousValue existing for Current
            if (NOT InHandle.template Has<Current_AttributePreviousType>())
            { InHandle.template Add<Current_AttributePreviousType>(InAttributeCurrent.Get_Base(), InAttributeCurrent.Get_Final()); }

            auto& PreviousValue = InHandle.template Get<Current_AttributePreviousType>();

            // Clamping on the client side is bypassed because the server might update both 'Max' and 'Current' values.
            // In cases where 'Current' needs to match the new 'Max', if the client receives 'Current' before 'Max' and clamps it,
            // the value could be incorrectly constrained to the previous 'Max' when 'Max' is replicated after clamping.
            // However, if the attribute is refilling and the change does not require replication, client-side clamping is NOT bypassed.
            if (InHandle.template Has<ck::FTag_ReplicatedAttribute>() &&
                NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InHandle) &&
                TUtils_Attribute<T_DerivedAttributeCurrent>::Get_MayRequireReplicationThisFrame(InHandle))
            {
                if (PreviousValue.Get_Base() != BaseValue || PreviousValue.Get_Final() != FinalValue)
                { TUtils_Attribute<T_DerivedAttributeCurrent>::Request_FireSignals(InHandle); }

                return;
            }

            const auto FinalValue_Bound = InAttributeBound._Final;

            InAttributeCurrent._Base = TAttributeMinMax<AttributeDataType>::Min(BaseValue, FinalValue_Bound);
            InAttributeCurrent._Final = TAttributeMinMax<AttributeDataType>::Min(FinalValue, FinalValue_Bound);

            if (PreviousValue.Get_Base() != BaseValue || PreviousValue.Get_Final() != FinalValue)
            { TUtils_Attribute<T_DerivedAttributeCurrent>::Request_FireSignals(InHandle); }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, concepts::ValidAttributeFragment T_DerivedAttribute, typename T_RepDataStruct>
    auto
        TProcessor_Attribute_Replicate<T_DerivedProcessor, T_DerivedAttribute, T_RepDataStruct>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const T_DerivedAttribute& InAttribute) const
            -> void
    {
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        UCk_Utils_Net_UE::TryUpdateContainerFragment<T_RepDataStruct>(
            LifetimeOwner, [&](T_RepDataStruct& Data)
        {
            const auto& AttributeName = UCk_Utils_GameplayLabel_UE::Get_Label(InHandle);
            const auto& BaseValue = InAttribute.Get_Base();
            const auto& FinalValue = InAttribute.Get_Final();
            const auto ComponentType = T_DerivedAttribute::ComponentTagType;

            using EntryType = typename decltype(Data.Attributes)::ElementType;
            const auto ToReplicate = EntryType{AttributeName, BaseValue, FinalValue, ComponentType};

            const auto Found = Data.Attributes.FindByPredicate([&](const EntryType& InElement)
            {
                return InElement.Get_AttributeName() == AttributeName && InElement.Get_Component() == ComponentType;
            });

            if (ck::Is_NOT_Valid(Found, ck::IsValid_Policy_NullptrOnly{}))
            { Data.Attributes.Emplace(ToReplicate); }
            else
            { *Found = ToReplicate; }
        });

        InHandle.template Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, concepts::ValidAttributeModifierFragment T_DerivedAttributeModifier,
              ECk_Attribute_Refill_Policy T_RefillMode>
    auto
        TProcessor_Attribute_Refill_Impl<T_DerivedProcessor, T_DerivedAttributeModifier, T_RefillMode>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType& InAttribute) const
        -> void
    {
        if constexpr (T_RefillMode == ECk_Attribute_Refill_Policy::Variable)
        {
            const auto& RefillValue = InAttribute.Get_Final() * InDeltaT.Get_Seconds();

            auto RefillAttributeTarget = TUtils_RefillAttributeTarget<HandleType>::Get_StoredEntity(InHandle);

            TUtils_AttributeModifier<AttributeModifierFragmentType>::Add_NotRevocable
            (
                RefillAttributeTarget,
                RefillValue,
                ECk_AttributeModifier_Operation::Add,
                ECk_AttributeValueChange_SyncPolicy::DoNotSync
            );
        }
        else // AlwaysToZero
        {
            auto RefillValue = FMath::Abs(InAttribute.Get_Final() * InDeltaT.Get_Seconds());
            auto RefillAttributeTarget = TUtils_RefillAttributeTarget<HandleType>::Get_StoredEntity(InHandle);

            const auto AttributeValue = TUtils_Attribute<AttributeFragmentType>::Get_FinalValue(RefillAttributeTarget);

            if (FMath::IsNearlyZero(AttributeValue))
            { return; }

            if (FMath::Abs(AttributeValue) < RefillValue)
            { RefillValue = FMath::Abs(AttributeValue); }

            TUtils_AttributeModifier<AttributeModifierFragmentType>::Add_NotRevocable
            (
                RefillAttributeTarget,
                AttributeValue > 0 ? -RefillValue : RefillValue,
                ECk_AttributeModifier_Operation::Add,
                ECk_AttributeValueChange_SyncPolicy::DoNotSync
            );
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, concepts::ValidAttributeModifierFragment T_TargetAttributeModifier, concepts::ValidAttributeFragment T_FloatAttribute,
              ECk_Attribute_Refill_Policy T_RefillMode>
    auto
        TProcessor_Attribute_AccumulatedRefill_Impl<T_DerivedProcessor, T_TargetAttributeModifier, T_FloatAttribute, T_RefillMode>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            FloatAttributeFragmentType& InFloatAttribute,
            FFragment_RefillAccumulator& InAccumulator) const
        -> void
    {
        if constexpr (T_RefillMode == ECk_Attribute_Refill_Policy::Variable)
        {
            InAccumulator._Accumulator += InFloatAttribute.Get_Final() * InDeltaT.Get_Seconds();

            const auto IntegerPart = static_cast<int32>(InAccumulator._Accumulator);
            if (IntegerPart == 0)
            { return; }

            InAccumulator._Accumulator -= static_cast<float>(IntegerPart);

            auto RefillAttributeTarget = TUtils_RefillAttributeTarget<TargetHandleType>::Get_StoredEntity(InHandle);

            TUtils_AttributeModifier<TargetAttributeModifierFragmentType>::Add_NotRevocable
            (
                RefillAttributeTarget,
                IntegerPart,
                ECk_AttributeModifier_Operation::Add,
                ECk_AttributeValueChange_SyncPolicy::DoNotSync
            );
        }
        else // AlwaysToZero
        {
            using TargetAttributeFragmentType = typename TargetAttributeModifierFragmentType::AttributeFragmentType;

            auto RefillAttributeTarget = TUtils_RefillAttributeTarget<TargetHandleType>::Get_StoredEntity(InHandle);
            const auto AttributeValue = TUtils_Attribute<TargetAttributeFragmentType>::Get_FinalValue(RefillAttributeTarget);

            if (AttributeValue == 0)
            { return; }

            InAccumulator._Accumulator += FMath::Abs(InFloatAttribute.Get_Final()) * InDeltaT.Get_Seconds();

            const auto IntegerPart = static_cast<int32>(InAccumulator._Accumulator);
            if (IntegerPart == 0)
            { return; }

            InAccumulator._Accumulator -= static_cast<float>(IntegerPart);

            auto ToApply = FMath::Min(IntegerPart, FMath::Abs(AttributeValue));

            TUtils_AttributeModifier<TargetAttributeModifierFragmentType>::Add_NotRevocable
            (
                RefillAttributeTarget,
                AttributeValue > 0 ? -ToApply : ToApply,
                ECk_AttributeModifier_Operation::Add,
                ECk_AttributeValueChange_SyncPolicy::DoNotSync
            );
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, concepts::ValidAttributeModifierFragment T_AttributeModifierFragment>
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
            TEXT("Resetting FinalValue for [{}] AttributeComponent of Attribute Entity [{}] and requesting a new computation from all its Attribute Modifiers."),
            AttributeFragmentType::ComponentTagType,
            InHandle
        );

        InAttribute._Final = InAttribute._Base;

        TUtils_AttributeModifier<AttributeModifierFragmentType>::RecordOfAttributeModifiers_Utils::ForEach_ValidEntry
        (
            InHandle,
            [&](auto InAttributeModifier) -> void
            {
                // This is necessary since all 3 types of attribute components (Min/Max/Current) are stored in the same Record.
                // Since this processor is specialized for one of them, we need to skip over the modifiers that does NOT match it
                // to avoid triggering an ensure.
                if (NOT TUtils_AttributeModifier<AttributeModifierFragmentType>::Has(InAttributeModifier))
                { return; }

                TUtils_AttributeModifier<AttributeModifierFragmentType>::Request_ComputeResult(InAttributeModifier);
            }
        );

        TUtils_Attribute<AttributeFragmentType>::Request_TryClamp(InHandle);
        TUtils_Attribute<AttributeFragmentType>::Request_FireSignals(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    namespace modifier_detail
    {
        template <ECk_AttributeModifier_Operation T_Op>
        constexpr auto Get_RevocableLogLabel() -> const TCHAR*
        {
            if constexpr (T_Op == ECk_AttributeModifier_Operation::Add)
            { return TEXT("Computing REVOCABLE (ADD) AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]"); }
            else if constexpr (T_Op == ECk_AttributeModifier_Operation::Subtract)
            { return TEXT("Computing REVOCABLE (SUBTRACT) AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]"); }
            else if constexpr (T_Op == ECk_AttributeModifier_Operation::Multiply)
            { return TEXT("Computing REVOCABLE (MULTIPLY) AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]"); }
            else if constexpr (T_Op == ECk_AttributeModifier_Operation::Divide)
            { return TEXT("Computing REVOCABLE (DIVIDE) AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]"); }
            else
            { return TEXT(""); } // Override is never revocable
        }

        template <ECk_AttributeModifier_Operation T_Op>
        constexpr auto Get_NotRevocableLogLabel() -> const TCHAR*
        {
            if constexpr (T_Op == ECk_AttributeModifier_Operation::Add)
            { return TEXT("Computing NOT REVOCABLE (ADD) AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]"); }
            else if constexpr (T_Op == ECk_AttributeModifier_Operation::Subtract)
            { return TEXT("Computing NOT REVOCABLE (SUBTRACT) AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]"); }
            else if constexpr (T_Op == ECk_AttributeModifier_Operation::Multiply)
            { return TEXT("Computing NOT REVOCABLE (MULTIPLY) AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]"); }
            else if constexpr (T_Op == ECk_AttributeModifier_Operation::Divide)
            { return TEXT("Computing NOT REVOCABLE (DIVIDE) AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]"); }
            else if constexpr (T_Op == ECk_AttributeModifier_Operation::Override)
            { return TEXT("OVERRIDING AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]"); }
            else
            { return TEXT(""); }
        }

        template <typename T_AttributeDataType, ECk_AttributeModifier_Operation T_Op>
        auto ApplyOperation(T_AttributeDataType InValue, T_AttributeDataType InDelta) -> T_AttributeDataType
        {
            if constexpr (T_Op == ECk_AttributeModifier_Operation::Add)
            { return TAttributeModifierOperators<T_AttributeDataType>::Add(InValue, InDelta); }
            else if constexpr (T_Op == ECk_AttributeModifier_Operation::Subtract)
            { return TAttributeModifierOperators<T_AttributeDataType>::Sub(InValue, InDelta); }
            else if constexpr (T_Op == ECk_AttributeModifier_Operation::Multiply)
            { return TAttributeModifierOperators<T_AttributeDataType>::Mul(InValue, InDelta); }
            else if constexpr (T_Op == ECk_AttributeModifier_Operation::Divide)
            { return TAttributeModifierOperators<T_AttributeDataType>::Div(InValue, InDelta); }
            else // Override - should not be called with this helper for the value application
            { return InDelta; }
        }
    }

    template <typename T_DerivedProcessor, concepts::ValidAttributeModifierFragment T_DerivedAttributeModifier,
              ECk_AttributeModifier_Operation T_Operation,
              ECk_AttributeModifier_Revocability T_Revocability>
    auto
        TProcessor_AttributeModifier_Compute<T_DerivedProcessor, T_DerivedAttributeModifier, T_Operation, T_Revocability>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            std::conditional_t<T_Revocability == ECk_AttributeModifier_Revocability::Revocable,
                const AttributeModifierFragmentType&,
                AttributeModifierFragmentType&> InAttributeModifier) const
        -> void
    {
        const auto& ModifierDelta = InAttributeModifier.Get_ModifierDelta();

        if (ck::Is_NOT_Valid(ModifierDelta))
        { return; }

        auto TargetEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        auto& AttributeComp = TargetEntity.template Get<AttributeFragmentType>();

        if constexpr (T_Revocability == ECk_AttributeModifier_Revocability::Revocable)
        {
            attribute::VeryVerbose
            (
                modifier_detail::Get_RevocableLogLabel<T_Operation>(),
                InHandle,
                AttributeFragmentType::ComponentTagType,
                TargetEntity
            );

            AttributeComp._Final = modifier_detail::ApplyOperation<AttributeDataType, T_Operation>(AttributeComp._Final, *ModifierDelta);
        }
        else // NotRevocable
        {
            attribute::VeryVerbose
            (
                modifier_detail::Get_NotRevocableLogLabel<T_Operation>(),
                InHandle,
                AttributeFragmentType::ComponentTagType,
                TargetEntity
            );

            if constexpr (T_Operation == ECk_AttributeModifier_Operation::Override)
            {
                AttributeComp._Base = *ModifierDelta;
            }
            else
            {
                AttributeComp._Base = modifier_detail::ApplyOperation<AttributeDataType, T_Operation>(AttributeComp._Base, *ModifierDelta);
            }

            // TODO: move this to the Tick() of TProcessor_AttributeModifier_RevocableAdditive_Compute
            // technically, the following is 'correct' but it's confusing as to why we are resetting the Final in this processor
            AttributeComp._Final = AttributeComp._Base;

            InAttributeModifier._ModifierDelta.Reset();
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <concepts::ValidAttributeModifierFragment T_DerivedAttributeModifier>
    TProcessor_AttributeModifier_ComputeAll<T_DerivedAttributeModifier>::
        TProcessor_AttributeModifier_ComputeAll(
            RegistryType InRegistry)
        : Super(InRegistry)
        , _Override_Compute(InRegistry)

        , _NotRevocableAdd_Compute(InRegistry)
        , _NotRevocableSubtract_Compute(InRegistry)
        , _NotRevocableMultiply_Compute(InRegistry)
        , _NotRevocableDivide_Compute(InRegistry)

        , _RevocableAdd_Compute(InRegistry)
        , _RevocableSubtract_Compute(InRegistry)
        , _RevocableMultiply_Compute(InRegistry)
        , _RevocableDivide_Compute(InRegistry)
    {
    }

    template <concepts::ValidAttributeModifierFragment T_DerivedAttributeModifier>
    auto
        TProcessor_AttributeModifier_ComputeAll<T_DerivedAttributeModifier>::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _Override_Compute.Tick(InDeltaT);

        _NotRevocableAdd_Compute.Tick(InDeltaT);
        _NotRevocableSubtract_Compute.Tick(InDeltaT);
        _NotRevocableMultiply_Compute.Tick(InDeltaT);
        _NotRevocableDivide_Compute.Tick(InDeltaT);

        _RevocableAdd_Compute.Tick(InDeltaT);
        _RevocableSubtract_Compute.Tick(InDeltaT);
        _RevocableMultiply_Compute.Tick(InDeltaT);
        _RevocableDivide_Compute.Tick(InDeltaT);

        this->_TransientEntity.template Clear<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, concepts::ValidAttributeModifierFragment T_AttributeModifierFragment>
    auto
        TProcessor_AttributeModifier_EndPlay<T_DerivedProcessor, T_AttributeModifierFragment>::
        ForEachEntity(
            const TimeType&,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier) const
        -> void
    {
        // even though WE as a Modifier are dying, our Owner may not be
        auto TargetEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle, ECk_PendingKill_Policy::IncludePendingKill);

        if (ck::Is_NOT_Valid(TargetEntity))
        { return; }

        auto TargetAsAttributeEntity = ck::StaticCast<AttributeHandleType>(TargetEntity);

        attribute::VeryVerbose
        (
            TEXT("Removing REVOCABLE AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]. "
            "Forcing final value calculation again"),
            InHandle,
            AttributeFragmentType::ComponentTagType,
            TargetAsAttributeEntity
        );

        TUtils_Attribute<AttributeFragmentType>::Request_RecomputeFinalValue(TargetAsAttributeEntity);
        TUtils_Attribute<AttributeFragmentType>::Request_TryReplicateAttribute(TargetAsAttributeEntity);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttribute, typename T_MulticastType>
    TProcessor_Attribute_FireSignals_CurrentMinMax<T_DerivedAttribute, T_MulticastType>::
        TProcessor_Attribute_FireSignals_CurrentMinMax(
            RegistryType InRegistry)
        : _Current(InRegistry)
        , _Min(InRegistry)
        , _Max(InRegistry)
        , _MinClamped(InRegistry)
        , _MaxClamped(InRegistry)
        , _Registry(InRegistry)
    {
    }

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttribute, typename T_MulticastType>
    auto
        TProcessor_Attribute_FireSignals_CurrentMinMax<T_DerivedAttribute, T_MulticastType>::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        _Min.Tick(InDeltaT);
        _Max.Tick(InDeltaT);
        _Current.Tick(InDeltaT);
        _MinClamped.Tick(InDeltaT);
        _MaxClamped.Tick(InDeltaT);
    }

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttribute, typename T_MulticastType>
    auto
        TProcessor_Attribute_FireSignals_CurrentMinMax<T_DerivedAttribute, T_MulticastType>::
        Pump()
        -> void
    {
        _Min.Pump();
        _Max.Pump();
        _Current.Pump();
        _MinClamped.Pump();
        _MaxClamped.Pump();
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttribute, typename T_Attribute_ReplicatedFragment>
    TProcessor_Attribute_Replicate_All<T_DerivedAttribute, T_Attribute_ReplicatedFragment>::
    TProcessor_Attribute_Replicate_All(
        RegistryType InRegistry)
        : _Current(InRegistry)
        , _Min(InRegistry)
        , _Max(InRegistry)
        , _Registry(InRegistry)
    {
    }

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttribute, typename T_Attribute_ReplicatedFragment>
    auto
        TProcessor_Attribute_Replicate_All<T_DerivedAttribute, T_Attribute_ReplicatedFragment>::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        _Min.Tick(InDeltaT);
        _Max.Tick(InDeltaT);
        _Current.Tick(InDeltaT);
    }

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttribute, typename T_Attribute_ReplicatedFragment>
    auto
        TProcessor_Attribute_Replicate_All<T_DerivedAttribute, T_Attribute_ReplicatedFragment>::
        Pump()
        -> void
    {
        _Min.Pump();
        _Max.Pump();
        _Current.Pump();
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <template <ECk_MinMaxCurrent T_Component> typename T_DerivedAttribute>
    TProcessor_Attribute_MinMaxClamp<T_DerivedAttribute>::
        TProcessor_Attribute_MinMaxClamp(
            RegistryType InRegistry)
        : _MinClamp(InRegistry)
        , _MaxClamp(InRegistry)
        , _Registry(InRegistry)
    {
    }

    template <template <ECk_MinMaxCurrent T_Component> typename T_DerivedAttribute>
    auto
        TProcessor_Attribute_MinMaxClamp<T_DerivedAttribute>::
        Tick(
            TimeType InDeltaT)
            -> void
    {
        _MinClamp.Tick(InDeltaT);
        _MaxClamp.Tick(InDeltaT);

        UCk_Utils_EntityLifetime_UE::Get_TransientEntity(_Registry).Clear<FTag_MayRequireClamping>();
    }

    template <template <ECk_MinMaxCurrent T_Component> typename T_DerivedAttribute>
    auto
        TProcessor_Attribute_MinMaxClamp<T_DerivedAttribute>::
        Pump()
        -> void
    {
        _MinClamp.Pump();
        _MaxClamp.Pump();

        UCk_Utils_EntityLifetime_UE::Get_TransientEntity(_Registry).Clear<FTag_MayRequireClamping>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    TProcessor_Attribute_RecomputeAll_CurrentMinMax<T_DerivedAttributeModifier>::
        TProcessor_Attribute_RecomputeAll_CurrentMinMax(
            RegistryType InRegistry)
        : _Current_Previous(InRegistry)
        , _Min_Previous(InRegistry)
        , _Max_Previous(InRegistry)
        , _Current(InRegistry)
        , _Min(InRegistry)
        , _Max(InRegistry)
        , _Registry(InRegistry)
    {
    }

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    auto
        TProcessor_Attribute_RecomputeAll_CurrentMinMax<T_DerivedAttributeModifier>::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        _Current_Previous.Tick(InDeltaT);
        _Min_Previous.Tick(InDeltaT);
        _Max_Previous.Tick(InDeltaT);

        _Max.Tick(InDeltaT);
        _Min.Tick(InDeltaT);
        _Current.Tick(InDeltaT);
    }

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    auto
        TProcessor_Attribute_RecomputeAll_CurrentMinMax<T_DerivedAttributeModifier>::
        Pump()
        -> void
    {
        _Current_Previous.Pump();
        _Min_Previous.Pump();
        _Max_Previous.Pump();

        _Max.Pump();
        _Min.Pump();
        _Current.Pump();
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    TProcessor_AttributeModifier_ComputeAll_CurrentMinMax<T_DerivedAttributeModifier>::
        TProcessor_AttributeModifier_ComputeAll_CurrentMinMax(
            RegistryType InRegistry)
        : _Current(InRegistry)
        , _Min(InRegistry)
        , _Max(InRegistry)
        , _Registry(InRegistry)
    {
    }

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    auto
        TProcessor_AttributeModifier_ComputeAll_CurrentMinMax<T_DerivedAttributeModifier>::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        _Max.Tick(InDeltaT);
        _Min.Tick(InDeltaT);
        _Current.Tick(InDeltaT);
    }

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    auto
        TProcessor_AttributeModifier_ComputeAll_CurrentMinMax<T_DerivedAttributeModifier>::
        Pump()
        -> void
    {
        _Max.Pump();
        _Min.Pump();
        _Current.Pump();
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    TProcessor_AttributeModifier_EndPlayAll_CurrentMinMax<T_DerivedAttributeModifier>::
        TProcessor_AttributeModifier_EndPlayAll_CurrentMinMax(
            RegistryType InRegistry)
        : _Current(InRegistry)
        , _Min(InRegistry)
        , _Max(InRegistry)
        , _Registry(InRegistry)
    {
    }

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    auto
        TProcessor_AttributeModifier_EndPlayAll_CurrentMinMax<T_DerivedAttributeModifier>::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        _Max.Tick(InDeltaT);
        _Min.Tick(InDeltaT);
        _Current.Tick(InDeltaT);
    }

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    auto
        TProcessor_AttributeModifier_EndPlayAll_CurrentMinMax<T_DerivedAttributeModifier>::
        Pump()
        -> void
    {
        _Max.Pump();
        _Min.Pump();
        _Current.Pump();
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    TProcessor_Attribute_Refill<T_DerivedAttributeModifier>::
        TProcessor_Attribute_Refill(
            RegistryType InRegistry)
        : _Refill_Current(InRegistry)
        , _Refill_Min(InRegistry)
        , _Refill_Max(InRegistry)

        , _Refill_AlwaysToZero_Current(InRegistry)
        , _Refill_AlwaysToZero_Min(InRegistry)
        , _Refill_AlwaysToZero_Max(InRegistry)

        , _Registry(InRegistry)
    {
    }

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    auto
        TProcessor_Attribute_Refill<T_DerivedAttributeModifier>::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        _Refill_Max.Tick(InDeltaT);
        _Refill_Min.Tick(InDeltaT);
        _Refill_Current.Tick(InDeltaT);

        _Refill_AlwaysToZero_Max.Tick(InDeltaT);
        _Refill_AlwaysToZero_Min.Tick(InDeltaT);
        _Refill_AlwaysToZero_Current.Tick(InDeltaT);
    }

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    auto
        TProcessor_Attribute_Refill<T_DerivedAttributeModifier>::
        Pump()
        -> void
    {
        _Refill_Max.Pump();
        _Refill_Min.Pump();
        _Refill_Current.Pump();

        _Refill_AlwaysToZero_Max.Pump();
        _Refill_AlwaysToZero_Min.Pump();
        _Refill_AlwaysToZero_Current.Pump();
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <template <ECk_MinMaxCurrent T_Component> class T_TargetAttributeModifier,
              template <ECk_MinMaxCurrent T_Component> class T_FloatAttribute>
    TProcessor_Attribute_AccumulatedRefill<T_TargetAttributeModifier, T_FloatAttribute>::
        TProcessor_Attribute_AccumulatedRefill(
            RegistryType InRegistry)
        : _Refill_Current(InRegistry)
        , _Refill_Min(InRegistry)
        , _Refill_Max(InRegistry)

        , _Refill_AlwaysToZero_Current(InRegistry)
        , _Refill_AlwaysToZero_Min(InRegistry)
        , _Refill_AlwaysToZero_Max(InRegistry)

        , _Registry(InRegistry)
    {
    }

    template <template <ECk_MinMaxCurrent T_Component> class T_TargetAttributeModifier,
              template <ECk_MinMaxCurrent T_Component> class T_FloatAttribute>
    auto
        TProcessor_Attribute_AccumulatedRefill<T_TargetAttributeModifier, T_FloatAttribute>::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        _Refill_Max.Tick(InDeltaT);
        _Refill_Min.Tick(InDeltaT);
        _Refill_Current.Tick(InDeltaT);

        _Refill_AlwaysToZero_Max.Tick(InDeltaT);
        _Refill_AlwaysToZero_Min.Tick(InDeltaT);
        _Refill_AlwaysToZero_Current.Tick(InDeltaT);
    }

    template <template <ECk_MinMaxCurrent T_Component> class T_TargetAttributeModifier,
              template <ECk_MinMaxCurrent T_Component> class T_FloatAttribute>
    auto
        TProcessor_Attribute_AccumulatedRefill<T_TargetAttributeModifier, T_FloatAttribute>::
        Pump()
        -> void
    {
        _Refill_Max.Pump();
        _Refill_Min.Pump();
        _Refill_Current.Pump();

        _Refill_AlwaysToZero_Max.Pump();
        _Refill_AlwaysToZero_Min.Pump();
        _Refill_AlwaysToZero_Current.Pump();
    }
}

// --------------------------------------------------------------------------------------------------------------------
