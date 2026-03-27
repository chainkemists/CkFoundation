#pragma once

#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include <type_traits>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::detail
{
    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, concepts::ValidAttributeFragment T_DerivedAttribute>
    class TProcessor_Attribute_StorePreviousValue : public ck_exp::TProcessor<
            TProcessor_Attribute_StorePreviousValue<T_DerivedProcessor, T_DerivedAttribute>,
            typename T_DerivedAttribute::HandleType,
            T_DerivedAttribute,
            typename T_DerivedAttribute::FTag_RecomputeFinalValue,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttribute::FTag_RecomputeFinalValue;

    public:
        using AttributeFragmentType = T_DerivedAttribute;
        using AttributeDataType     = typename AttributeFragmentType::AttributeDataType;
        using HandleType            = typename AttributeFragmentType::HandleType;
        using ThisType              = TProcessor_Attribute_StorePreviousValue<T_DerivedProcessor, T_DerivedAttribute>;
        using Super                 = ck_exp::TProcessor<ThisType, HandleType, AttributeFragmentType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType              = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto
        ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType& InAttribute) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, concepts::ValidAttributeFragment T_DerivedAttribute, typename T_MulticastType>
    class TProcessor_Attribute_FireSignals_ValueChanged : public ck_exp::TProcessor<
            TProcessor_Attribute_FireSignals_ValueChanged<T_DerivedProcessor, T_DerivedAttribute, T_MulticastType>,
            typename T_DerivedAttribute::HandleType,
            T_DerivedAttribute,
            TFragment_Attribute_PreviousValues<T_DerivedAttribute>,
            typename T_DerivedAttribute::FTag_FireSignals_ValueChanged,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttribute::FTag_FireSignals_ValueChanged;

    public:
        using AttributeFragmentType         = T_DerivedAttribute;
        using AttributeFragmentPreviousType = TFragment_Attribute_PreviousValues<T_DerivedAttribute>;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using HandleType                    = typename AttributeFragmentType::HandleType;
        using ThisType                      = TProcessor_Attribute_FireSignals_ValueChanged<T_DerivedProcessor, AttributeFragmentType, T_MulticastType>;
        using Super                         = ck_exp::TProcessor<ThisType, HandleType, AttributeFragmentType, AttributeFragmentPreviousType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType                      = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType& InAttribute,
            AttributeFragmentPreviousType& InAttributePrevious) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, concepts::ValidAttributeFragment T_DerivedAttributeCurrent, concepts::ValidAttributeFragment T_DerivedAttributeBound,
              typename T_MulticastType, ECk_AttributeClamp_Direction T_Direction>
    class TProcessor_Attribute_FireSignals_Clamped : public ck_exp::TProcessor<
            TProcessor_Attribute_FireSignals_Clamped<T_DerivedProcessor, T_DerivedAttributeCurrent, T_DerivedAttributeBound, T_MulticastType, T_Direction>,
            typename T_DerivedAttributeCurrent::HandleType,
            T_DerivedAttributeCurrent,
            T_DerivedAttributeBound,
            std::conditional_t<T_Direction == ECk_AttributeClamp_Direction::Min,
                typename T_DerivedAttributeCurrent::FTag_FireSignals_MinClamped,
                typename T_DerivedAttributeCurrent::FTag_FireSignals_MaxClamped>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = std::conditional_t<T_Direction == ECk_AttributeClamp_Direction::Min,
            typename T_DerivedAttributeCurrent::FTag_FireSignals_MinClamped,
            typename T_DerivedAttributeCurrent::FTag_FireSignals_MaxClamped>;

    public:
        using AttributeFragmentType_Current         = T_DerivedAttributeCurrent;
        using AttributeFragmentType_Bound           = T_DerivedAttributeBound;
        using AttributeFragmentPreviousType_Current = TFragment_Attribute_PreviousValues<T_DerivedAttributeCurrent>;
        using AttributeFragmentPreviousType_Bound   = TFragment_Attribute_PreviousValues<T_DerivedAttributeBound>;
        using AttributeDataType                     = typename AttributeFragmentType_Current::AttributeDataType;
        using HandleType                            = typename AttributeFragmentType_Current::HandleType;
        using ThisType                              = TProcessor_Attribute_FireSignals_Clamped<
                                                        T_DerivedProcessor,
                                                        T_DerivedAttributeCurrent,
                                                        T_DerivedAttributeBound,
                                                        T_MulticastType,
                                                        T_Direction>;
        using Super                                 = ck_exp::TProcessor<
                                                        ThisType,
                                                        HandleType,
                                                        AttributeFragmentType_Current,
                                                        AttributeFragmentType_Bound,
                                                        MarkedDirtyBy,
                                                        CK_IGNORE_PENDING_KILL>;
        using TimeType                              = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType_Current& InAttribute_Current,
            AttributeFragmentType_Bound& InAttribute_Bound) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    template <typename T_D, typename T_Current, typename T_Min, typename T_Multi>
    using TProcessor_Attribute_FireSignals_MinClamped =
        TProcessor_Attribute_FireSignals_Clamped<T_D, T_Current, T_Min, T_Multi, ECk_AttributeClamp_Direction::Min>;

    template <typename T_D, typename T_Current, typename T_Max, typename T_Multi>
    using TProcessor_Attribute_FireSignals_MaxClamped =
        TProcessor_Attribute_FireSignals_Clamped<T_D, T_Current, T_Max, T_Multi, ECk_AttributeClamp_Direction::Max>;


    template <typename T_DerivedProcessor, concepts::ValidAttributeFragment T_DerivedAttributeCurrent, concepts::ValidAttributeFragment T_DerivedAttributeBound,
              ECk_AttributeClamp_Direction T_Direction>
    class TProcessor_Attribute_Clamp : public ck_exp::TProcessor<
            TProcessor_Attribute_Clamp<T_DerivedProcessor, T_DerivedAttributeCurrent, T_DerivedAttributeBound, T_Direction>,
            typename T_DerivedAttributeCurrent::HandleType,
            T_DerivedAttributeCurrent,
            T_DerivedAttributeBound,
            FTag_MayRequireClamping,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = T_DerivedAttributeBound;

    public:
        using AttributeFragmentType_Current = T_DerivedAttributeCurrent;
        using AttributeFragmentType_Bound   = T_DerivedAttributeBound;
        using AttributeDataType             = typename AttributeFragmentType_Current::AttributeDataType;
        using HandleType                    = typename AttributeFragmentType_Current::HandleType;
        using ThisType                      = TProcessor_Attribute_Clamp<T_DerivedProcessor, AttributeFragmentType_Current, AttributeFragmentType_Bound, T_Direction>;
        using Super                         = ck_exp::TProcessor<ThisType, HandleType, AttributeFragmentType_Current, MarkedDirtyBy, FTag_MayRequireClamping, CK_IGNORE_PENDING_KILL>;
        using TimeType                      = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType_Current& InAttributeCurrent,
            const AttributeFragmentType_Bound& InAttributeBound) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    template <typename T_D, typename T_Current, typename T_Min>
    using TProcessor_Attribute_MinClamp =
        TProcessor_Attribute_Clamp<T_D, T_Current, T_Min, ECk_AttributeClamp_Direction::Min>;

    template <typename T_D, typename T_Current, typename T_Max>
    using TProcessor_Attribute_MaxClamp =
        TProcessor_Attribute_Clamp<T_D, T_Current, T_Max, ECk_AttributeClamp_Direction::Max>;

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, concepts::ValidAttributeFragment T_DerivedAttribute, typename T_RepDataStruct>
    class TProcessor_Attribute_Replicate : public ck_exp::TProcessor<
            TProcessor_Attribute_Replicate<T_DerivedProcessor, T_DerivedAttribute, T_RepDataStruct>,
            typename T_DerivedAttribute::HandleType,
            T_DerivedAttribute,
            typename T_DerivedAttribute::FTag_MayRequireReplication,
            ck::TExclude<typename T_DerivedAttribute::FTag_RecomputeFinalValue>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttribute::FTag_MayRequireReplication;

    public:
        using AttributeFragmentType = T_DerivedAttribute;
        using HandleType            = typename AttributeFragmentType::HandleType;
        using ThisType              = TProcessor_Attribute_Replicate<T_DerivedProcessor, T_DerivedAttribute, T_RepDataStruct>;
        using Super                 = ck_exp::TProcessor<ThisType, HandleType, T_DerivedAttribute, MarkedDirtyBy, ck::TExclude<typename T_DerivedAttribute::FTag_RecomputeFinalValue>, CK_IGNORE_PENDING_KILL>;
        using TimeType              = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const T_DerivedAttribute& InAttribute) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, concepts::ValidAttributeModifierFragment T_DerivedAttributeModifier,
              ECk_Attribute_Refill_Policy T_RefillMode>
    class TProcessor_Attribute_Refill_Impl : public ck_exp::TProcessor<
            TProcessor_Attribute_Refill_Impl<T_DerivedProcessor, T_DerivedAttributeModifier, T_RefillMode>,
            typename T_DerivedAttributeModifier::AttributeFragmentType::HandleType,
            typename T_DerivedAttributeModifier::AttributeFragmentType,
            FTag_IsRefillAttribute,
            std::conditional_t<T_RefillMode == ECk_Attribute_Refill_Policy::Variable,
                TExclude<FTag_RefillBehaviorAlwaysToZero>,
                FTag_RefillBehaviorAlwaysToZero>,
            FTag_IsRefillRunning,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename FTag_IsRefillRunning;

    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using HandleType                    = typename AttributeFragmentType::HandleType;
        using ThisType                      = TProcessor_Attribute_Refill_Impl<T_DerivedProcessor, T_DerivedAttributeModifier, T_RefillMode>;

    private:
        using RefillModeFilter = std::conditional_t<T_RefillMode == ECk_Attribute_Refill_Policy::Variable,
            TExclude<FTag_RefillBehaviorAlwaysToZero>,
            FTag_RefillBehaviorAlwaysToZero>;

    public:
        using Super = ck_exp::TProcessor<ThisType, HandleType, AttributeFragmentType, FTag_IsRefillAttribute, RefillModeFilter, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType& InAttribute) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    template <typename T_D, typename T_M>
    using TProcessor_Attribute_Refill =
        TProcessor_Attribute_Refill_Impl<T_D, T_M, ECk_Attribute_Refill_Policy::Variable>;

    template <typename T_D, typename T_M>
    using TProcessor_Attribute_Refill_AlwaysToZero =
        TProcessor_Attribute_Refill_Impl<T_D, T_M, ECk_Attribute_Refill_Policy::AlwaysReturnToZero>;


    template <typename T_DerivedProcessor, concepts::ValidAttributeModifierFragment T_TargetAttributeModifier, concepts::ValidAttributeFragment T_FloatAttribute,
              ECk_Attribute_Refill_Policy T_RefillMode>
    class TProcessor_Attribute_AccumulatedRefill_Impl : public ck_exp::TProcessor<
            TProcessor_Attribute_AccumulatedRefill_Impl<T_DerivedProcessor, T_TargetAttributeModifier, T_FloatAttribute, T_RefillMode>,
            typename T_FloatAttribute::HandleType,
            T_FloatAttribute,
            FFragment_RefillAccumulator,
            FTag_IsRefillAttribute,
            std::conditional_t<T_RefillMode == ECk_Attribute_Refill_Policy::Variable,
                TExclude<FTag_RefillBehaviorAlwaysToZero>,
                FTag_RefillBehaviorAlwaysToZero>,
            FTag_IsRefillRunning,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename FTag_IsRefillRunning;

    public:
        using TargetAttributeModifierFragmentType = T_TargetAttributeModifier;
        using TargetHandleType                     = typename T_TargetAttributeModifier::AttributeFragmentType::HandleType;
        using FloatAttributeFragmentType           = T_FloatAttribute;
        using HandleType                           = typename T_FloatAttribute::HandleType;
        using ThisType                             = TProcessor_Attribute_AccumulatedRefill_Impl<T_DerivedProcessor, T_TargetAttributeModifier, T_FloatAttribute, T_RefillMode>;

    private:
        using RefillModeFilter = std::conditional_t<T_RefillMode == ECk_Attribute_Refill_Policy::Variable,
            TExclude<FTag_RefillBehaviorAlwaysToZero>,
            FTag_RefillBehaviorAlwaysToZero>;

    public:
        using Super    = ck_exp::TProcessor<ThisType, HandleType, FloatAttributeFragmentType, FFragment_RefillAccumulator, FTag_IsRefillAttribute, RefillModeFilter, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            FloatAttributeFragmentType& InFloatAttribute,
            FFragment_RefillAccumulator& InAccumulator) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    template <typename T_D, typename T_Target, typename T_Float>
    using TProcessor_Attribute_AccumulatedRefill =
        TProcessor_Attribute_AccumulatedRefill_Impl<T_D, T_Target, T_Float, ECk_Attribute_Refill_Policy::Variable>;

    template <typename T_D, typename T_Target, typename T_Float>
    using TProcessor_Attribute_AccumulatedRefill_AlwaysToZero =
        TProcessor_Attribute_AccumulatedRefill_Impl<T_D, T_Target, T_Float, ECk_Attribute_Refill_Policy::AlwaysReturnToZero>;

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, concepts::ValidAttributeModifierFragment T_DerivedAttributeModifier>
    class TProcessor_Attribute_RecomputeAll : public ck_exp::TProcessor<
            TProcessor_Attribute_RecomputeAll<T_DerivedProcessor, T_DerivedAttributeModifier>,
            typename T_DerivedAttributeModifier::AttributeFragmentType::HandleType,
            typename T_DerivedAttributeModifier::AttributeFragmentType,
            typename T_DerivedAttributeModifier::AttributeFragmentType::FTag_RecomputeFinalValue,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::AttributeFragmentType::FTag_RecomputeFinalValue;

    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using HandleType                    = typename AttributeFragmentType::HandleType;
        using ThisType                      = TProcessor_Attribute_RecomputeAll<T_DerivedProcessor, AttributeModifierFragmentType>;
        using Super                         = ck_exp::TProcessor<ThisType, HandleType, AttributeFragmentType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType                      = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            AttributeFragmentType& InAttribute) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    // --------------------------------------------------------------------------------------------------------------------

    namespace modifier_detail
    {
        template <typename T_Modifier, ECk_AttributeModifier_Operation T_Op>
        struct TOperationTag;

        template <typename T_Modifier>
        struct TOperationTag<T_Modifier, ECk_AttributeModifier_Operation::Add>
        { using Type = typename T_Modifier::FTag_ModifyAdd; };

        template <typename T_Modifier>
        struct TOperationTag<T_Modifier, ECk_AttributeModifier_Operation::Subtract>
        { using Type = typename T_Modifier::FTag_ModifySubtract; };

        template <typename T_Modifier>
        struct TOperationTag<T_Modifier, ECk_AttributeModifier_Operation::Multiply>
        { using Type = typename T_Modifier::FTag_ModifyMultiply; };

        template <typename T_Modifier>
        struct TOperationTag<T_Modifier, ECk_AttributeModifier_Operation::Divide>
        { using Type = typename T_Modifier::FTag_ModifyDivide; };

        template <typename T_Modifier>
        struct TOperationTag<T_Modifier, ECk_AttributeModifier_Operation::Override>
        { using Type = typename T_Modifier::FTag_ModifyOverride; };

        template <typename T_Modifier, ECk_AttributeModifier_Revocability T_Rev>
        struct TRevocabilityTag;

        template <typename T_Modifier>
        struct TRevocabilityTag<T_Modifier, ECk_AttributeModifier_Revocability::Revocable>
        { using Type = typename T_Modifier::FTag_IsRevocableModification; };

        template <typename T_Modifier>
        struct TRevocabilityTag<T_Modifier, ECk_AttributeModifier_Revocability::NotRevocable>
        { using Type = typename T_Modifier::FTag_IsNotRevocableModification; };
    }

    template <typename T_DerivedProcessor, concepts::ValidAttributeModifierFragment T_DerivedAttributeModifier,
              ECk_AttributeModifier_Operation T_Operation,
              ECk_AttributeModifier_Revocability T_Revocability>
    class TProcessor_AttributeModifier_Compute : public ck_exp::TProcessor<
            TProcessor_AttributeModifier_Compute<T_DerivedProcessor, T_DerivedAttributeModifier, T_Operation, T_Revocability>,
            typename T_DerivedAttributeModifier::HandleType,
            T_DerivedAttributeModifier,
            typename modifier_detail::TOperationTag<T_DerivedAttributeModifier, T_Operation>::Type,
            typename modifier_detail::TRevocabilityTag<T_DerivedAttributeModifier, T_Revocability>::Type,
            typename T_DerivedAttributeModifier::FTag_ComputeResult,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::FTag_ComputeResult;

    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using ModificationType              = typename modifier_detail::TOperationTag<T_DerivedAttributeModifier, T_Operation>::Type;
        using RevocabilityType              = typename modifier_detail::TRevocabilityTag<T_DerivedAttributeModifier, T_Revocability>::Type;
        using HandleType                    = typename AttributeModifierFragmentType::HandleType;
        using ThisType                      = TProcessor_AttributeModifier_Compute<T_DerivedProcessor, T_DerivedAttributeModifier, T_Operation, T_Revocability>;
        using Super                         = ck_exp::TProcessor<ThisType, HandleType, AttributeModifierFragmentType, ModificationType, RevocabilityType, MarkedDirtyBy, CK_IGNORE_PENDING_KILL>;
        using TimeType                      = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            std::conditional_t<T_Revocability == ECk_AttributeModifier_Revocability::Revocable,
                const AttributeModifierFragmentType&,
                AttributeModifierFragmentType&> InAttributeModifier) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };

    template <typename T_D, typename T_M>
    using TProcessor_AttributeModifier_RevocableAdd_Compute =
        TProcessor_AttributeModifier_Compute<T_D, T_M, ECk_AttributeModifier_Operation::Add, ECk_AttributeModifier_Revocability::Revocable>;

    template <typename T_D, typename T_M>
    using TProcessor_AttributeModifier_RevocableSubtract_Compute =
        TProcessor_AttributeModifier_Compute<T_D, T_M, ECk_AttributeModifier_Operation::Subtract, ECk_AttributeModifier_Revocability::Revocable>;

    template <typename T_D, typename T_M>
    using TProcessor_AttributeModifier_RevocableMultiply_Compute =
        TProcessor_AttributeModifier_Compute<T_D, T_M, ECk_AttributeModifier_Operation::Multiply, ECk_AttributeModifier_Revocability::Revocable>;

    template <typename T_D, typename T_M>
    using TProcessor_AttributeModifier_RevocableDivide_Compute =
        TProcessor_AttributeModifier_Compute<T_D, T_M, ECk_AttributeModifier_Operation::Divide, ECk_AttributeModifier_Revocability::Revocable>;

    template <typename T_D, typename T_M>
    using TProcessor_AttributeModifier_NotRevocableAdd_Compute =
        TProcessor_AttributeModifier_Compute<T_D, T_M, ECk_AttributeModifier_Operation::Add, ECk_AttributeModifier_Revocability::NotRevocable>;

    template <typename T_D, typename T_M>
    using TProcessor_AttributeModifier_NotRevocableSubtract_Compute =
        TProcessor_AttributeModifier_Compute<T_D, T_M, ECk_AttributeModifier_Operation::Subtract, ECk_AttributeModifier_Revocability::NotRevocable>;

    template <typename T_D, typename T_M>
    using TProcessor_AttributeModifier_NotRevocableMultiply_Compute =
        TProcessor_AttributeModifier_Compute<T_D, T_M, ECk_AttributeModifier_Operation::Multiply, ECk_AttributeModifier_Revocability::NotRevocable>;

    template <typename T_D, typename T_M>
    using TProcessor_AttributeModifier_NotRevocableDivide_Compute =
        TProcessor_AttributeModifier_Compute<T_D, T_M, ECk_AttributeModifier_Operation::Divide, ECk_AttributeModifier_Revocability::NotRevocable>;

    template <typename T_D, typename T_M>
    using TProcessor_AttributeModifier_Override_Compute =
        TProcessor_AttributeModifier_Compute<T_D, T_M, ECk_AttributeModifier_Operation::Override, ECk_AttributeModifier_Revocability::NotRevocable>;

    // --------------------------------------------------------------------------------------------------------------------

    template <concepts::ValidAttributeModifierFragment T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_ComputeAll : public TProcessorBase<TProcessor_AttributeModifier_ComputeAll<T_DerivedAttributeModifier>>
    {
    public:
        using Super        = TProcessorBase<TProcessor_AttributeModifier_ComputeAll<T_DerivedAttributeModifier>>;
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;
        using MarkedDirtyBy = typename T_DerivedAttributeModifier::FTag_ComputeResult;

    public:
        explicit
        TProcessor_AttributeModifier_ComputeAll(RegistryType InRegistry);

    public:
        auto DoTick(
            TimeType InDeltaT) -> void;

    private:
        TProcessor_AttributeModifier_Override_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _Override_Compute;

        TProcessor_AttributeModifier_NotRevocableAdd_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _NotRevocableAdd_Compute;
        TProcessor_AttributeModifier_NotRevocableSubtract_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _NotRevocableSubtract_Compute;
        TProcessor_AttributeModifier_NotRevocableMultiply_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _NotRevocableMultiply_Compute;
        TProcessor_AttributeModifier_NotRevocableDivide_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _NotRevocableDivide_Compute;

        TProcessor_AttributeModifier_RevocableAdd_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _RevocableAdd_Compute;
        TProcessor_AttributeModifier_RevocableSubtract_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _RevocableSubtract_Compute;
        TProcessor_AttributeModifier_RevocableMultiply_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _RevocableMultiply_Compute;
        TProcessor_AttributeModifier_RevocableDivide_Compute<
            TProcessor_AttributeModifier_ComputeAll, T_DerivedAttributeModifier> _RevocableDivide_Compute;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, concepts::ValidAttributeModifierFragment T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_EndPlay : public ck_exp::TProcessor<
            TProcessor_AttributeModifier_EndPlay<T_DerivedProcessor, T_DerivedAttributeModifier>,
            typename T_DerivedAttributeModifier::HandleType,
            T_DerivedAttributeModifier,
            typename T_DerivedAttributeModifier::FTag_IsRevocableModification,
            CK_IF_END_PLAY>
    {
    public:
        using AttributeModifierFragmentType = T_DerivedAttributeModifier;
        using AttributeFragmentType         = typename AttributeModifierFragmentType::AttributeFragmentType;
        using AttributeDataType             = typename AttributeFragmentType::AttributeDataType;
        using IsRevocableModificationType   = typename AttributeModifierFragmentType::FTag_IsRevocableModification;
        using HandleType                    = typename AttributeModifierFragmentType::HandleType;
        using AttributeHandleType           = typename AttributeModifierFragmentType::AttributeHandleType;
        using ThisType                      = TProcessor_AttributeModifier_EndPlay<T_DerivedProcessor, T_DerivedAttributeModifier>;
        using Super                         = ck_exp::TProcessor<ThisType, HandleType, AttributeModifierFragmentType, IsRevocableModificationType, CK_IF_END_PLAY>;
        using TimeType                      = typename Super::TimeType;

    public:
        CK_USING_BASE_CONSTRUCTORS(Super);

    public:
        auto ForEachEntity(
            const TimeType& InDeltaT,
            HandleType InHandle,
            const AttributeModifierFragmentType& InAttributeModifier) const -> void;

    public:
        CK_ENABLE_SFINAE_THIS(T_DerivedProcessor);
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // the second argument is the templated derived fragment from which we can deduce all Fragment types i.e. Current/Min/Max
    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttribute, typename T_MulticastType>
    class TProcessor_Attribute_FireSignals_CurrentMinMax
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;

        template <ECk_MinMaxCurrent T_Component>
        using TValueChangedProcessorType = detail::TProcessor_Attribute_FireSignals_ValueChanged<
            TProcessor_Attribute_FireSignals_CurrentMinMax,
            T_DerivedAttribute<T_Component>,
            T_MulticastType>;

        using TMinClampedProcessorType = detail::TProcessor_Attribute_FireSignals_MinClamped<
            TProcessor_Attribute_FireSignals_CurrentMinMax,
            T_DerivedAttribute<ECk_MinMaxCurrent::Current>,
            T_DerivedAttribute<ECk_MinMaxCurrent::Min>,
            T_MulticastType>;

        using TMaxClampedProcessorType = detail::TProcessor_Attribute_FireSignals_MaxClamped<
            TProcessor_Attribute_FireSignals_CurrentMinMax,
            T_DerivedAttribute<ECk_MinMaxCurrent::Current>,
            T_DerivedAttribute<ECk_MinMaxCurrent::Max>,
            T_MulticastType>;

    public:
        explicit
        TProcessor_Attribute_FireSignals_CurrentMinMax(RegistryType InRegistry);

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    private:
        TValueChangedProcessorType<ECk_MinMaxCurrent::Current> _Current;
        TValueChangedProcessorType<ECk_MinMaxCurrent::Min> _Min;
        TValueChangedProcessorType<ECk_MinMaxCurrent::Max> _Max;
        TMinClampedProcessorType _MinClamped;
        TMaxClampedProcessorType _MaxClamped;

    private:
        RegistryType _Registry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttribute, typename T_RepDataStruct>
    class TProcessor_Attribute_Replicate_All
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;

        template <ECk_MinMaxCurrent T_Component>
        using TInternalProcessorType = detail::TProcessor_Attribute_Replicate<
            TProcessor_Attribute_Replicate_All, T_DerivedAttribute<T_Component>, T_RepDataStruct>;

    public:
        explicit
        TProcessor_Attribute_Replicate_All(RegistryType InRegistry);

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    private:
        TInternalProcessorType<ECk_MinMaxCurrent::Current> _Current;
        TInternalProcessorType<ECk_MinMaxCurrent::Min> _Min;
        TInternalProcessorType<ECk_MinMaxCurrent::Max> _Max;

    private:
        RegistryType _Registry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // the second argument is the templated derived fragment from which we can deduce all Fragment types i.e. Current/Min/Max
    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttribute>
    class TProcessor_Attribute_MinMaxClamp
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;

    public:
        explicit
        TProcessor_Attribute_MinMaxClamp(RegistryType InRegistry);

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    private:
        detail::TProcessor_Attribute_MinClamp<TProcessor_Attribute_MinMaxClamp,
            T_DerivedAttribute<ECk_MinMaxCurrent::Current>, T_DerivedAttribute<ECk_MinMaxCurrent::Min>> _MinClamp;

        detail::TProcessor_Attribute_MaxClamp<TProcessor_Attribute_MinMaxClamp,
            T_DerivedAttribute<ECk_MinMaxCurrent::Current>, T_DerivedAttribute<ECk_MinMaxCurrent::Max>> _MaxClamp;

    private:
        RegistryType _Registry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // the second argument is the templated derived fragment from which we can deduce all Fragment types i.e. Current/Min/Max
    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    class TProcessor_Attribute_RecomputeAll_CurrentMinMax
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;

        template <ECk_MinMaxCurrent T_Component>
        using TInternalProcessorType = detail::TProcessor_Attribute_RecomputeAll<
            TProcessor_Attribute_RecomputeAll_CurrentMinMax, T_DerivedAttributeModifier<T_Component>>;

        template <ECk_MinMaxCurrent T_Component>
        using TStorePreviousValueProcessorType = detail::TProcessor_Attribute_StorePreviousValue<
            TProcessor_Attribute_RecomputeAll_CurrentMinMax, typename T_DerivedAttributeModifier<T_Component>::AttributeFragmentType>;

    public:
        explicit
        TProcessor_Attribute_RecomputeAll_CurrentMinMax(RegistryType InRegistry);

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    private:
        TStorePreviousValueProcessorType<ECk_MinMaxCurrent::Current> _Current_Previous;
        TStorePreviousValueProcessorType<ECk_MinMaxCurrent::Min> _Min_Previous;
        TStorePreviousValueProcessorType<ECk_MinMaxCurrent::Max> _Max_Previous;

        TInternalProcessorType<ECk_MinMaxCurrent::Current> _Current;
        TInternalProcessorType<ECk_MinMaxCurrent::Min> _Min;
        TInternalProcessorType<ECk_MinMaxCurrent::Max> _Max;

    private:
        RegistryType _Registry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // the second argument is the templated derived fragment from which we can deduce all Fragment types i.e. Current/Min/Max
    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_ComputeAll_CurrentMinMax
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;

        template <ECk_MinMaxCurrent T_Component>
        using TInternalProcessorType = detail::TProcessor_AttributeModifier_ComputeAll<T_DerivedAttributeModifier<T_Component>>;

    public:
        explicit
        TProcessor_AttributeModifier_ComputeAll_CurrentMinMax(RegistryType InRegistry);

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    private:
        TInternalProcessorType<ECk_MinMaxCurrent::Current> _Current;
        TInternalProcessorType<ECk_MinMaxCurrent::Min> _Min;
        TInternalProcessorType<ECk_MinMaxCurrent::Max> _Max;

    private:
        RegistryType _Registry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // the second argument is the templated derived fragment from which we can deduce all Fragment types i.e. Current/Min/Max
    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_EndPlayAll_CurrentMinMax
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;

        template <ECk_MinMaxCurrent T_Component>
        using TInternalProcessorType = detail::TProcessor_AttributeModifier_EndPlay<
            TProcessor_AttributeModifier_EndPlayAll_CurrentMinMax, T_DerivedAttributeModifier<T_Component>>;

    public:
        explicit
        TProcessor_AttributeModifier_EndPlayAll_CurrentMinMax(RegistryType InRegistry);

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    private:
        TInternalProcessorType<ECk_MinMaxCurrent::Current> _Current;
        TInternalProcessorType<ECk_MinMaxCurrent::Min> _Min;
        TInternalProcessorType<ECk_MinMaxCurrent::Max> _Max;

    private:
        RegistryType _Registry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <template <ECk_MinMaxCurrent T_Component> class T_DerivedAttributeModifier>
    class TProcessor_Attribute_Refill
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;

        template <ECk_MinMaxCurrent T_Component>
        using TInternalProcessorType = detail::TProcessor_Attribute_Refill<
            TProcessor_Attribute_Refill, T_DerivedAttributeModifier<T_Component>>;

        template <ECk_MinMaxCurrent T_Component>
        using TInternalProcessorType_AlwaysToZero = detail::TProcessor_Attribute_Refill_AlwaysToZero<
            TProcessor_Attribute_Refill, T_DerivedAttributeModifier<T_Component>>;

    public:
        explicit
        TProcessor_Attribute_Refill(
            RegistryType InRegistry);

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    private:
        TInternalProcessorType<ECk_MinMaxCurrent::Current> _Refill_Current;
        TInternalProcessorType<ECk_MinMaxCurrent::Min> _Refill_Min;
        TInternalProcessorType<ECk_MinMaxCurrent::Max> _Refill_Max;

        TInternalProcessorType_AlwaysToZero<ECk_MinMaxCurrent::Current> _Refill_AlwaysToZero_Current;
        TInternalProcessorType_AlwaysToZero<ECk_MinMaxCurrent::Min> _Refill_AlwaysToZero_Min;
        TInternalProcessorType_AlwaysToZero<ECk_MinMaxCurrent::Max> _Refill_AlwaysToZero_Max;

    private:
        RegistryType _Registry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <template <ECk_MinMaxCurrent T_Component> class T_TargetAttributeModifier,
              template <ECk_MinMaxCurrent T_Component> class T_FloatAttribute>
    class TProcessor_Attribute_AccumulatedRefill
    {
    public:
        using TimeType     = FCk_Time;
        using RegistryType = FCk_Registry;

        template <ECk_MinMaxCurrent T_Component>
        using TInternalProcessorType = detail::TProcessor_Attribute_AccumulatedRefill<
            TProcessor_Attribute_AccumulatedRefill, T_TargetAttributeModifier<T_Component>, T_FloatAttribute<T_Component>>;

        template <ECk_MinMaxCurrent T_Component>
        using TInternalProcessorType_AlwaysToZero = detail::TProcessor_Attribute_AccumulatedRefill_AlwaysToZero<
            TProcessor_Attribute_AccumulatedRefill, T_TargetAttributeModifier<T_Component>, T_FloatAttribute<T_Component>>;

    public:
        explicit
        TProcessor_Attribute_AccumulatedRefill(
            RegistryType InRegistry);

    public:
        auto Tick(
            TimeType InDeltaT) -> void;

    private:
        TInternalProcessorType<ECk_MinMaxCurrent::Current> _Refill_Current;
        TInternalProcessorType<ECk_MinMaxCurrent::Min> _Refill_Min;
        TInternalProcessorType<ECk_MinMaxCurrent::Max> _Refill_Max;

        TInternalProcessorType_AlwaysToZero<ECk_MinMaxCurrent::Current> _Refill_AlwaysToZero_Current;
        TInternalProcessorType_AlwaysToZero<ECk_MinMaxCurrent::Min> _Refill_AlwaysToZero_Min;
        TInternalProcessorType_AlwaysToZero<ECk_MinMaxCurrent::Max> _Refill_AlwaysToZero_Max;

    private:
        RegistryType _Registry;
    };
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkAttribute_Processor.inl.h"
