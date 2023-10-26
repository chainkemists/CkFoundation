#pragma once
#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedProcessor, typename T_DerivedAttribute, typename T_MulticastType>
    class TProcessor_Attribute_FireSignals : public TProcessor<
            TProcessor_Attribute_FireSignals<T_DerivedProcessor, T_DerivedAttribute, T_MulticastType>,
            T_DerivedAttribute,
            typename T_DerivedAttribute::Tag_FireSignals>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttribute::Tag_FireSignals;

    public:
        using AttributeFragmentType = T_DerivedAttribute;
        using AttributeDataType     = typename AttributeFragmentType::AttributeDataType;
        using ThisType              = TProcessor_Attribute_FireSignals<T_DerivedProcessor, AttributeFragmentType, T_MulticastType>;
        using Super                 = TProcessor<ThisType, AttributeFragmentType, MarkedDirtyBy>;
        using HandleType            = typename Super::HandleType;
        using TimeType              = typename Super::TimeType;

    public:
        using Super::Super;

    public:
        auto Tick(TimeType InDeltaT) -> void;
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType& InAttribute) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_Attribute_RecomputeAll : public TProcessor<
            TProcessor_Attribute_RecomputeAll<T_DerivedProcessor, T_DerivedAttributeModifier>,
            typename T_DerivedAttributeModifier::AttributeFragmentType,
            typename T_DerivedAttributeModifier::AttributeFragmentType::Tag_RecomputeFinalValue>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::AttributeFragmentType::Tag_RecomputeFinalValue;

    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using ThisType                      = TProcessor_Attribute_RecomputeAll<T_DerivedProcessor, AttributeModifierFragmentType>;
        using Super                         = TProcessor<ThisType, AttributeFragmentType, MarkedDirtyBy>;
        using HandleType                    = typename Super::HandleType;
        using TimeType                      = typename Super::TimeType;

    public:
        using Super::Super;

    public:
        auto Tick(TimeType InDeltaT) -> void;
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType& InAttribute) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_RevokableAdditive_Compute : public TProcessor<
        TProcessor_AttributeModifier_RevokableAdditive_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
        T_DerivedAttributeModifier,
        FFragment_AttributeModifierTarget,
        typename T_DerivedAttributeModifier::Tag_AdditiveModification,
        typename T_DerivedAttributeModifier::Tag_IsRevokableModification,
        typename T_DerivedAttributeModifier::Tag_ComputeResult>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::Tag_ComputeResult;

    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using AttributeModifierTargetType   = FFragment_AttributeModifierTarget;
        using ModificationType              = typename AttributeModifierFragmentType::Tag_AdditiveModification;
        using IsRevokableModificationType   = typename AttributeModifierFragmentType::Tag_IsRevokableModification;
        using ThisType                      = TProcessor_AttributeModifier_RevokableAdditive_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                         = TProcessor<ThisType, AttributeModifierFragmentType, AttributeModifierTargetType, ModificationType, IsRevokableModificationType, MarkedDirtyBy>;
        using HandleType                    = typename Super::HandleType;
        using TimeType                      = typename Super::TimeType;

    public:
        using Super::Super;

    public:
        auto Tick(TimeType InDeltaT) -> void;
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier,
            const AttributeModifierTargetType& InAttributeTarget) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_NotRevokableAdditive_Compute : public TProcessor<
        TProcessor_AttributeModifier_NotRevokableAdditive_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
        T_DerivedAttributeModifier,
        FFragment_AttributeModifierTarget,
        typename T_DerivedAttributeModifier::Tag_AdditiveModification,
        typename T_DerivedAttributeModifier::Tag_IsNotRevokableModification,
        typename T_DerivedAttributeModifier::Tag_ComputeResult>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::Tag_ComputeResult;

    public:
        using AttributeModifierFragmentType  = T_DerivedAttributeModifier;
        using AttributeFragmentType          = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType              = typename AttributeFragmentType::AttributeDataType;
        using AttributeModifierTargetType    = FFragment_AttributeModifierTarget;
        using ModificationType               = typename AttributeModifierFragmentType::Tag_AdditiveModification;
        using IsNotRevokableModificationType = typename AttributeModifierFragmentType::Tag_IsNotRevokableModification;
        using ThisType                       = TProcessor_AttributeModifier_NotRevokableAdditive_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                          = TProcessor<ThisType, AttributeModifierFragmentType, AttributeModifierTargetType, ModificationType, IsNotRevokableModificationType, MarkedDirtyBy>;
        using HandleType                     = typename Super::HandleType;
        using TimeType                       = typename Super::TimeType;

    public:
        using Super::Super;

    public:
        auto Tick(TimeType InDeltaT) -> void;
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier,
            const AttributeModifierTargetType& InAttributeTarget) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_Additive_Teardown : public TProcessor<
        TProcessor_AttributeModifier_Additive_Teardown<T_DerivedProcessor, T_DerivedAttributeModifier>,
        T_DerivedAttributeModifier,
        FFragment_AttributeModifierTarget,
        typename T_DerivedAttributeModifier::Tag_AdditiveModification,
        typename T_DerivedAttributeModifier::Tag_IsRevokableModification,
        FTag_PendingDestroyEntity>
    {
    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using AttributeModifierTargetType   = FFragment_AttributeModifierTarget;
        using ModificationType              = typename AttributeModifierFragmentType::Tag_AdditiveModification;
        using IsRevokableModificationType   = typename AttributeModifierFragmentType::Tag_IsRevokableModification;
        using ThisType                      = TProcessor_AttributeModifier_Additive_Teardown<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                         = TProcessor<ThisType, AttributeModifierFragmentType, AttributeModifierTargetType, ModificationType, IsRevokableModificationType, FTag_PendingDestroyEntity>;
        using HandleType                    = typename Super::HandleType;
        using TimeType                      = typename Super::TimeType;

    public:
        using Super::Super;

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier,
            const AttributeModifierTargetType& InAttributeTarget) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_RevokableMultiplicative_Compute : public TProcessor<
        TProcessor_AttributeModifier_RevokableMultiplicative_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
        T_DerivedAttributeModifier,
        FFragment_AttributeModifierTarget,
        typename T_DerivedAttributeModifier::Tag_MultiplicativeModification,
        typename T_DerivedAttributeModifier::Tag_IsRevokableModification,
        typename T_DerivedAttributeModifier::Tag_ComputeResult>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::Tag_ComputeResult;

    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using AttributeModifierTargetType   = FFragment_AttributeModifierTarget;
        using ModificationType              = typename AttributeModifierFragmentType::Tag_MultiplicativeModification;
        using IsRevokableModificationType   = typename AttributeModifierFragmentType::Tag_IsRevokableModification;
        using ThisType                      = TProcessor_AttributeModifier_RevokableMultiplicative_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                         = TProcessor<ThisType, AttributeModifierFragmentType, AttributeModifierTargetType, ModificationType, IsRevokableModificationType, MarkedDirtyBy>;
        using HandleType                    = typename Super::HandleType;
        using TimeType                      = typename Super::TimeType;

    public:
        using Super::Super;

    public:
        auto Tick(TimeType InDeltaT) -> void;
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier,
            const AttributeModifierTargetType& InAttributeTarget) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

        // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_NotRevokableMultiplicative_Compute : public TProcessor<
        TProcessor_AttributeModifier_NotRevokableMultiplicative_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>,
        T_DerivedAttributeModifier,
        FFragment_AttributeModifierTarget,
        typename T_DerivedAttributeModifier::Tag_MultiplicativeModification,
        typename T_DerivedAttributeModifier::Tag_IsNotRevokableModification,
        typename T_DerivedAttributeModifier::Tag_ComputeResult>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::Tag_ComputeResult;

    public:
        using AttributeModifierFragmentType  = T_DerivedAttributeModifier;
        using AttributeFragmentType          = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType              = typename AttributeFragmentType::AttributeDataType;
        using AttributeModifierTargetType    = FFragment_AttributeModifierTarget;
        using ModificationType               = typename AttributeModifierFragmentType::Tag_MultiplicativeModification;
        using IsNotRevokableModificationType = typename AttributeModifierFragmentType::Tag_IsNotRevokableModification;
        using ThisType                       = TProcessor_AttributeModifier_NotRevokableMultiplicative_Compute<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                          = TProcessor<ThisType, AttributeModifierFragmentType, AttributeModifierTargetType, ModificationType, IsNotRevokableModificationType, MarkedDirtyBy>;
        using HandleType                     = typename Super::HandleType;
        using TimeType                       = typename Super::TimeType;

    public:
        using Super::Super;

    public:
        auto Tick(TimeType InDeltaT) -> void;
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier,
            const AttributeModifierTargetType& InAttributeTarget) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_Multiplicative_Teardown : public TProcessor<
        TProcessor_AttributeModifier_Multiplicative_Teardown<T_DerivedProcessor, T_DerivedAttributeModifier>,
        T_DerivedAttributeModifier,
        FFragment_AttributeModifierTarget,
        typename T_DerivedAttributeModifier::Tag_MultiplicativeModification,
        typename T_DerivedAttributeModifier::Tag_IsRevokableModification,
        FTag_PendingDestroyEntity>
    {
    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeModifierTargetType   = FFragment_AttributeModifierTarget;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using ModificationType              = typename AttributeModifierFragmentType::Tag_MultiplicativeModification;
        using IsRevokableModificationType   = typename AttributeModifierFragmentType::Tag_IsRevokableModification;
        using ThisType                      = TProcessor_AttributeModifier_Multiplicative_Teardown<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                         = TProcessor<ThisType, AttributeModifierFragmentType, AttributeModifierTargetType, ModificationType, IsRevokableModificationType, FTag_PendingDestroyEntity>;
        using HandleType                    = typename Super::HandleType;
        using TimeType                      = typename Super::TimeType;

    public:
        using Super::Super;

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier,
            const AttributeModifierTargetType&   InAttributeTarget) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkAttribute_Processor.inl.h"