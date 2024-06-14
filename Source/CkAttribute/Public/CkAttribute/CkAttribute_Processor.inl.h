#pragma once

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/CkAttribute_Utils.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Payload/CkPayload.h"

#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::detail
{
    template <typename T_DerivedProcessor, typename T_DerivedAttribute, typename T_DerivedAttributeModifier>
    auto
        TProcessor_Attribute_StorePreviousValue<T_DerivedProcessor, T_DerivedAttribute, T_DerivedAttributeModifier>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType& InAttribute) const
        -> void
    {
        using AttributePreviousType = ck::TFragment_Attribute_PreviousValues<AttributeFragmentType>;

        auto& PreviousValue = InHandle.template AddOrGet<AttributePreviousType>();
        PreviousValue = AttributePreviousType{InAttribute.Get_Base(), InAttribute.Get_Final()};
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttribute, typename T_MulticastType>
    auto
        TProcessor_Attribute_FireSignals<T_DerivedProcessor, T_DerivedAttribute, T_MulticastType>::
        ForEachEntity(
            const TimeType&,
            HandleType InHandle,
            AttributeFragmentType& InAttribute,
            AttributeFragmentPreviousType& InAttributePrevious) const
        -> void
    {
        InHandle.template Remove<MarkedDirtyBy>();

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
                    InAttribute._Base,
                    InAttribute._Final,

                    InAttributePrevious.Get_Base(),
                    InAttributePrevious.Get_Final()
                }
            )
        );
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeCurrent, typename T_DerivedAttributeMin>
    auto
        TProcessor_Attribute_MinClamp<T_DerivedProcessor, T_DerivedAttributeCurrent, T_DerivedAttributeMin>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType_Current& InAttributeCurrent,
            const AttributeFragmentType_Min& InAttributeMin) const
        -> void
    {
        const auto BaseValue = InAttributeCurrent._Base;
        const auto FinalValue = InAttributeCurrent._Final;

        if (InHandle.template Has<ck::FTag_ReplicatedAttribute>() && UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(InHandle))
        {
            using AttributePreviousType = ck::TFragment_Attribute_PreviousValues<T_DerivedAttributeCurrent>;
            auto& PreviousValue = InHandle.template AddOrGet<AttributePreviousType>();

            if (PreviousValue.Get_Base() != BaseValue || PreviousValue.Get_Final() != FinalValue)
            { TUtils_Attribute<T_DerivedAttributeCurrent>::Request_FireSignals(InHandle); }

            PreviousValue = AttributePreviousType{BaseValue, FinalValue};

            return;
        }

        const auto FinalValue_Min = InAttributeMin._Final;

        InAttributeCurrent._Base = TAttributeMinMax<AttributeDataType>::Max(BaseValue, FinalValue_Min);
        InAttributeCurrent._Final = TAttributeMinMax<AttributeDataType>::Max(FinalValue, FinalValue_Min);

        if (InAttributeCurrent._Final != FinalValue || InAttributeCurrent._Base != BaseValue)
        {
            using AttributePreviousType = ck::TFragment_Attribute_PreviousValues<T_DerivedAttributeCurrent>;
            auto& PreviousValue = InHandle.template AddOrGet<AttributePreviousType>();

            PreviousValue = AttributePreviousType{BaseValue, FinalValue};

            TUtils_Attribute<T_DerivedAttributeCurrent>::Request_FireSignals(InHandle);
            TUtils_Attribute<T_DerivedAttributeCurrent>::Request_TryReplicateAttribute(InHandle);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeCurrent, typename T_DerivedAttributeMax>
    auto
        TProcessor_Attribute_MaxClamp<T_DerivedProcessor, T_DerivedAttributeCurrent, T_DerivedAttributeMax>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType_Current& InAttributeCurrent,
            const AttributeFragmentType_Max& InAttributeMax) const
        -> void
    {
        const auto BaseValue = InAttributeCurrent._Base;
        const auto FinalValue = InAttributeCurrent._Final;

        if (InHandle.template Has<ck::FTag_ReplicatedAttribute>() && UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(InHandle))
        {
            using AttributePreviousType = ck::TFragment_Attribute_PreviousValues<T_DerivedAttributeCurrent>;
            auto& PreviousValue = InHandle.template AddOrGet<AttributePreviousType>();

            if (PreviousValue.Get_Base() != BaseValue || PreviousValue.Get_Final() != FinalValue)
            { TUtils_Attribute<T_DerivedAttributeCurrent>::Request_FireSignals(InHandle); }

            PreviousValue = AttributePreviousType{BaseValue, FinalValue};

            return;
        }

        const auto FinalValue_Max = InAttributeMax._Final;

        InAttributeCurrent._Base = TAttributeMinMax<AttributeDataType>::Min(BaseValue, FinalValue_Max);
        InAttributeCurrent._Final = TAttributeMinMax<AttributeDataType>::Min(FinalValue, FinalValue_Max);

        if (InAttributeCurrent._Final != FinalValue || InAttributeCurrent._Base != BaseValue)
        {
            using AttributePreviousType = ck::TFragment_Attribute_PreviousValues<T_DerivedAttributeCurrent>;
            auto& PreviousValue = InHandle.template AddOrGet<AttributePreviousType>();

            PreviousValue = AttributePreviousType{BaseValue, FinalValue};

            TUtils_Attribute<T_DerivedAttributeCurrent>::Request_FireSignals(InHandle);
            TUtils_Attribute<T_DerivedAttributeCurrent>::Request_TryReplicateAttribute(InHandle);
        }
    }

    template <typename T_DerivedProcessor, typename T_DerivedAttribute, typename T_DerivedAttribute_ReplicatedFragment>
    auto
        TProcessor_Attribute_Replicate<T_DerivedProcessor, T_DerivedAttribute, T_DerivedAttribute_ReplicatedFragment>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const T_DerivedAttribute& InAttribute) const
            -> void
    {
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<T_DerivedAttribute_ReplicatedFragment>(
            LifetimeOwner, [&](T_DerivedAttribute_ReplicatedFragment* InRepComp)
        {
            const auto& AttributeName = UCk_Utils_GameplayLabel_UE::Get_Label(InHandle);
            const auto& BaseValue = InAttribute.Get_Base();
            const auto& FinalValue = InAttribute.Get_Final();
            const auto ComponentType = T_DerivedAttribute::ComponentTagType;

            InRepComp->Broadcast_AddOrUpdate(AttributeName, BaseValue, FinalValue, ComponentType);
        });

        InHandle.template Remove<MarkedDirtyBy>();
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

        TUtils_Attribute<AttributeFragmentType>::Request_FireSignals(InHandle);
        TUtils_Attribute<AttributeFragmentType>::Request_TryReplicateAttribute(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_AttributeModifierFragment>
    auto
        TProcessor_AttributeModifier_RevocableAdd_Compute<T_DerivedProcessor ,T_AttributeModifierFragment>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier) const
        -> void
    {
        auto TargetEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        auto& AttributeComp = TargetEntity.template Get<AttributeFragmentType>();

        attribute::VeryVerbose
        (
            TEXT("Computing REVOCABLE (ADD) AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]"),
            InHandle,
            AttributeFragmentType::ComponentTagType,
            TargetEntity
        );

        AttributeComp._Final = TAttributeModifierOperators<AttributeDataType>::Add(AttributeComp._Final, InAttributeModifier.Get_ModifierDelta());
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_AttributeModifierFragment>
    auto
        TProcessor_AttributeModifier_RevocableSubtract_Compute<T_DerivedProcessor ,T_AttributeModifierFragment>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier) const
        -> void
    {
        auto TargetEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        auto& AttributeComp = TargetEntity.template Get<AttributeFragmentType>();

        attribute::VeryVerbose
        (
            TEXT("Computing REVOCABLE (SUBTRACT) AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]"),
            InHandle,
            AttributeFragmentType::ComponentTagType,
            TargetEntity
        );

        AttributeComp._Final = TAttributeModifierOperators<AttributeDataType>::Sub(AttributeComp._Final, InAttributeModifier.Get_ModifierDelta());
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    auto
        TProcessor_AttributeModifier_NotRevocableAdd_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier) const -> void
    {
        auto TargetEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        auto& AttributeComp = TargetEntity.template Get<AttributeFragmentType>();

        attribute::VeryVerbose
        (
            TEXT("Computing NOT REVOCABLE (ADD) AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]"),
            InHandle,
            AttributeFragmentType::ComponentTagType,
            TargetEntity
        );

        AttributeComp._Base = TAttributeModifierOperators<AttributeDataType>::Add(AttributeComp._Base, InAttributeModifier.Get_ModifierDelta());

        // TODO: move this to the Tick() of TProcessor_AttributeModifier_RevocableAdditive_Compute
        // technically, the following is 'correct' but it's confusing as to why we are resetting the Final in this processor
        AttributeComp._Final = AttributeComp._Base;

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    auto
        TProcessor_AttributeModifier_NotRevocableSubtract_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier) const -> void
    {
        auto TargetEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        auto& AttributeComp = TargetEntity.template Get<AttributeFragmentType>();

        attribute::VeryVerbose
        (
            TEXT("Computing NOT REVOCABLE (SUBTRACT) AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]"),
            InHandle,
            AttributeFragmentType::ComponentTagType,
            TargetEntity
        );

        AttributeComp._Base = TAttributeModifierOperators<AttributeDataType>::Sub(AttributeComp._Base, InAttributeModifier.Get_ModifierDelta());

        // TODO: move this to the Tick() of TProcessor_AttributeModifier_RevocableAdditive_Compute
        // technically, the following is 'correct' but it's confusing as to why we are resetting the Final in this processor
        AttributeComp._Final = AttributeComp._Base;

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_AttributeModifierFragment>
    auto
        TProcessor_AttributeModifier_RevocableMultiply_Compute<T_DerivedProcessor, T_AttributeModifierFragment>::
        ForEachEntity(
            const TimeType&,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier) const
        -> void
    {
        auto TargetEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        auto& AttributeComp = TargetEntity.template Get<AttributeFragmentType>();

        attribute::VeryVerbose
        (
            TEXT("Computing REVOCABLE (MULTIPLY) AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]"),
            InHandle,
            AttributeFragmentType::ComponentTagType,
            TargetEntity
        );

        AttributeComp._Final = TAttributeModifierOperators<AttributeDataType>::Mul(AttributeComp._Final, InAttributeModifier.Get_ModifierDelta());
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    auto
        TProcessor_AttributeModifier_RevocableDivide_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier) const
            -> void
    {
        auto TargetEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        auto& AttributeComp = TargetEntity.template Get<AttributeFragmentType>();

        attribute::VeryVerbose
        (
            TEXT("Computing REVOCABLE (DIVIDE) AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]"),
            InHandle,
            AttributeFragmentType::ComponentTagType,
            TargetEntity
        );

        AttributeComp._Final = TAttributeModifierOperators<AttributeDataType>::Div(AttributeComp._Final, InAttributeModifier.Get_ModifierDelta());
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_AttributeModifierFragment>
    auto
        TProcessor_AttributeModifier_NotRevocableMultiply_Compute<T_DerivedProcessor, T_AttributeModifierFragment>::
        ForEachEntity(
            const TimeType&,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier) const
        -> void
    {
        auto TargetEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        auto& AttributeComp = TargetEntity.template Get<AttributeFragmentType>();

        attribute::VeryVerbose
        (
            TEXT("Computing NOT REVOCABLE (MULTIPLY) AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]"),
            InHandle,
            AttributeFragmentType::ComponentTagType,
            TargetEntity
        );

        AttributeComp._Base = TAttributeModifierOperators<AttributeDataType>::Mul(AttributeComp._Base, InAttributeModifier.Get_ModifierDelta());
        AttributeComp._Final = AttributeComp._Base;

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    auto
        TProcessor_AttributeModifier_NotRevocableDivide_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>::
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier) const
        -> void
    {
        auto TargetEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        auto& AttributeComp = TargetEntity.template Get<AttributeFragmentType>();

        attribute::VeryVerbose
        (
            TEXT("Computing NOT REVOCABLE (DIVIDE) AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]"),
            InHandle,
            AttributeFragmentType::ComponentTagType,
            TargetEntity
        );

        AttributeComp._Base = TAttributeModifierOperators<AttributeDataType>::Div(AttributeComp._Base, InAttributeModifier.Get_ModifierDelta());
        AttributeComp._Final = AttributeComp._Base;

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttributeModifier>
    TProcessor_AttributeModifier_ComputeAll<T_DerivedAttributeModifier>::
        TProcessor_AttributeModifier_ComputeAll(
            RegistryType InRegistry)
        : Super(InRegistry)
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

    template <typename T_DerivedAttributeModifier>
    auto
        TProcessor_AttributeModifier_ComputeAll<T_DerivedAttributeModifier>::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
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

    template <typename T_DerivedProcessor, typename T_AttributeModifierFragment>
    auto
        TProcessor_AttributeModifier_Teardown<T_DerivedProcessor, T_AttributeModifierFragment>::
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

        attribute::VeryVerbose
        (
            TEXT("Removing REVOCABLE AttributeModifier Entity [{}] targeting [{}] AttributeComponent of Attribute Entity [{}]. "
            "Forcing final value calculation again"),
            InHandle,
            AttributeFragmentType::ComponentTagType,
            TargetEntity
        );

        TUtils_Attribute<AttributeFragmentType>::Request_RecomputeFinalValue(TargetEntity);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttribute, typename
        T_MulticastType>
    TProcessor_Attribute_FireSignals_CurrentMinMax<T_DerivedAttribute, T_MulticastType>::
        TProcessor_Attribute_FireSignals_CurrentMinMax(
            RegistryType InRegistry)
        : _Current(InRegistry)
        , _Min(InRegistry)
        , _Max(InRegistry)
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
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttribute, typename
        T_Attribute_ReplicatedFragment>
    TProcessor_Attribute_Replicate_All<T_DerivedAttribute, T_Attribute_ReplicatedFragment>::
    TProcessor_Attribute_Replicate_All(
        RegistryType InRegistry)
        : _Current(InRegistry)
        , _Min(InRegistry)
        , _Max(InRegistry)
    {
    }

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttribute, typename
        T_Attribute_ReplicatedFragment>
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

    // --------------------------------------------------------------------------------------------------------------------

    template <template <ECk_MinMaxCurrent T_Component> typename T_DerivedAttribute>
    TProcessor_Attribute_MinMaxClamp<T_DerivedAttribute>::
        TProcessor_Attribute_MinMaxClamp(
            RegistryType InRegistry)
        : _MinClamp(InRegistry)
        , _MaxClamp(InRegistry)
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

        _Current.Tick(InDeltaT);
        _Min.Tick(InDeltaT);
        _Max.Tick(InDeltaT);
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    TProcessor_AttributeModifier_ComputeAll_CurrentMinMax<T_DerivedAttributeModifier>::
        TProcessor_AttributeModifier_ComputeAll_CurrentMinMax(
            RegistryType InRegistry)
        : _Current(InRegistry)
        , _Min(InRegistry)
        , _Max(InRegistry)
    {
    }

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    auto
        TProcessor_AttributeModifier_ComputeAll_CurrentMinMax<T_DerivedAttributeModifier>::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        _Current.Tick(InDeltaT);
        _Min.Tick(InDeltaT);
        _Max.Tick(InDeltaT);
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    TProcessor_AttributeModifier_TeardownAll_CurrentMinMax<T_DerivedAttributeModifier>::
        TProcessor_AttributeModifier_TeardownAll_CurrentMinMax(
            RegistryType InRegistry)
        : _Current(InRegistry)
        , _Min(InRegistry)
        , _Max(InRegistry)
    {
    }

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    auto
        TProcessor_AttributeModifier_TeardownAll_CurrentMinMax<T_DerivedAttributeModifier>::
        Tick(
            TimeType InDeltaT)
            -> void
    {
        _Current.Tick(InDeltaT);
        _Min.Tick(InDeltaT);
        _Max.Tick(InDeltaT);
    }
}

// --------------------------------------------------------------------------------------------------------------------
