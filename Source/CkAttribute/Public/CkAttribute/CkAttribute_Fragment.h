#pragma once

#include "CkAttribute/CkAttribute_Concepts.h"
#include "CkAttribute/CkAttribute_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"
#include <GameplayTagContainer.h>

// --------------------------------------------------------------------------------------------------------------------

enum class ECk_AttributeModifier_Operation : uint8;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FAttributeModifier_ReplicationTags final : public FGameplayTagNativeAdder
    {
    protected:
        auto AddTags() -> void override;

    private:
        FGameplayTag _Base;
        FGameplayTag _Final;

        static FAttributeModifier_ReplicationTags _Tags;

    public:
        static auto Get_BaseTag() -> FGameplayTag;
        static auto Get_FinalTag() -> FGameplayTag;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FAttributeModifier_Tags final : public FGameplayTagNativeAdder
    {
    protected:
        auto AddTags() -> void override;

    private:
        FGameplayTag _Override;
        FGameplayTag _Refill;

        static FAttributeModifier_Tags _Tags;

    public:
        static auto Get_Override() -> FGameplayTag;
        static auto Get_Refill() -> FGameplayTag;
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_ReplicatedAttribute);
    CK_DEFINE_ECS_TAG(FTag_IsRefillAttribute);
    CK_DEFINE_ECS_TAG(FTag_RefillBehaviorAlwaysToZero);
    CK_DEFINE_ECS_TAG(FTag_IsRefillRunning);
    CK_DEFINE_ECS_TAG(FTag_MayRequireClamping);

    struct FFragment_RefillAccumulator
    {
        float _Accumulator = 0.0f;
    };

    template <concepts::ValidAttributeFragment T_DerivedAttribute>
    class TUtils_Attribute;

    template <concepts::ValidAttributeModifierFragment T_DerivedAttributeModifier>
    class TUtils_AttributeModifier;
}

enum class ECk_AttributeModifier_Operation : uint8;
enum class ECk_AttributeModifier_Revocability : uint8;
enum class ECk_AttributeClamp_Direction : uint8;
enum class ECk_Attribute_Refill_Policy : uint8;

namespace ck::detail
{
    template <typename T_DerivedProcessor, concepts::ValidAttributeFragment T_DerivedAttribute, typename T_MulticastType>
    class TProcessor_Attribute_FireSignals_ValueChanged;

    template <typename, concepts::ValidAttributeFragment, concepts::ValidAttributeFragment, typename, ECk_AttributeClamp_Direction>
    class TProcessor_Attribute_FireSignals_Clamped;

    template <typename, concepts::ValidAttributeFragment, concepts::ValidAttributeFragment, ECk_AttributeClamp_Direction>
    class TProcessor_Attribute_Clamp;

    template <typename, concepts::ValidAttributeFragment, concepts::ValidAttributeFragment>
    class TProcessor_Attribute_OverrideBaseValue;

    template <typename T_DerivedProcessor, concepts::ValidAttributeModifierFragment T_DerivedAttributeModifier>
    class TProcessor_Attribute_RecomputeAll;

    template <typename, concepts::ValidAttributeModifierFragment, ECk_AttributeModifier_Operation, ECk_AttributeModifier_Revocability>
    class TProcessor_AttributeModifier_Compute;

    template <typename T_DerivedProcessor, concepts::ValidAttributeModifierFragment T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_EndPlay;

    template <typename, concepts::ValidAttributeModifierFragment, ECk_Attribute_Refill_Policy>
    class TProcessor_Attribute_Refill_Impl;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    template <typename T>
    struct TAttributeMinMax
    {
        static auto Min(T A, T B) -> T { return (A < B) ? A : B; }
        static auto Max(T A, T B) -> T { return (B < A) ? A : B; }
    };

    // --------------------------------------------------------------------------------------------------------------------
    //
    // Direction-aware clamp helpers. They encapsulate the counterintuitive inversion between
    // clamp direction and the Min/Max primitive being called:
    //   - Min direction enforces a LOWER bound, so we return the LARGER of (value, bound).
    //   - Max direction enforces an UPPER bound, so we return the SMALLER of (value, bound).
    //
    // These are free templates that delegate to TAttributeMinMax<T>::Min/Max, so every existing
    // specialization (including the component-wise FVector one) works automatically without any
    // direction-specific specialization boilerplate.
    //
    template <ECk_AttributeClamp_Direction T_Direction, typename T>
    auto Attribute_Clamp(T InValue, T InBound) -> T
    {
        if constexpr (T_Direction == ECk_AttributeClamp_Direction::Min)
        { return TAttributeMinMax<T>::Max(InValue, InBound); }
        else
        { return TAttributeMinMax<T>::Min(InValue, InBound); }
    }

    // Direction-aware invariant check used to assert that a value never exceeded its bound.
    //
    // Uses the clamp result as an oracle: for a value already on the correct side of the bound,
    // clamping leaves it unchanged. This sidesteps ill-defined lexicographic comparisons on
    // composite types (e.g. FVector::operator<) and uniformly works with any T whose
    // TAttributeMinMax specialization behaves correctly — including component-wise FVector.
    //
    template <ECk_AttributeClamp_Direction T_Direction, typename T>
    auto Attribute_IsWithinBounds(T InValue, T InBound) -> bool
    {
        return Attribute_Clamp<T_Direction>(InValue, InBound) == InValue;
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T>
    struct TAttributeModifierOperators
    {
        static auto Add(T A, T B) -> T { return A + B; }
        static auto Sub(T A, T B) -> T { return A - B; }
        static auto Mul(T A, T B) -> T { return A * B; }
        static auto Div(T A, T B) -> T { return A / B; }
    };

    // ---- Templated entity holders for attribute refill relationships ----

    template <concepts::ValidAttributeRefillHandleType T_RefillHandleType>
    struct TFragment_RefillAttribute : public TFragment_EntityHolder<T_RefillHandleType>
    {
        using TFragment_EntityHolder<T_RefillHandleType>::TFragment_EntityHolder;
    };

    template <concepts::ValidAttributeHandleType T_AttributeHandleType>
    struct TFragment_RefillAttributeTarget : public TFragment_EntityHolder<T_AttributeHandleType>
    {
        using TFragment_EntityHolder<T_AttributeHandleType>::TFragment_EntityHolder;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <concepts::ValidAttributeHandleType T_HandleType, typename T_AttributeType, ECk_MinMaxCurrent T_ComponentTag>
    struct TFragment_Attribute
    {
    public:
        template <concepts::ValidAttributeFragment>
        friend class TUtils_Attribute;

        template <typename, concepts::ValidAttributeFragment, typename>
        friend class detail::TProcessor_Attribute_FireSignals_ValueChanged;

        template <typename, concepts::ValidAttributeFragment, concepts::ValidAttributeFragment, typename, ECk_AttributeClamp_Direction>
        friend class detail::TProcessor_Attribute_FireSignals_Clamped;

        template <typename, concepts::ValidAttributeFragment, concepts::ValidAttributeFragment, ECk_AttributeClamp_Direction>
        friend class detail::TProcessor_Attribute_Clamp;

        template <typename, concepts::ValidAttributeFragment, concepts::ValidAttributeFragment>
        friend class detail::TProcessor_Attribute_OverrideBaseValue;

        template <typename, concepts::ValidAttributeModifierFragment>
        friend class detail::TProcessor_Attribute_RecomputeAll;

        template <typename, concepts::ValidAttributeModifierFragment, ECk_AttributeModifier_Operation, ECk_AttributeModifier_Revocability>
        friend class detail::TProcessor_AttributeModifier_Compute;

        template <typename, concepts::ValidAttributeModifierFragment, ECk_Attribute_Refill_Policy>
        friend class detail::TProcessor_Attribute_Refill_Impl;

    public:
        CK_GENERATED_BODY(TFragment_Attribute<T_HandleType COMMA T_AttributeType COMMA T_ComponentTag>);

    public:
        CK_DEFINE_ECS_TAG(FTag_StorePreviousValue);
        CK_DEFINE_ECS_TAG(FTag_RecomputeFinalValue);
        CK_DEFINE_ECS_TAG(FTag_FireSignals_ValueChanged);
        CK_DEFINE_ECS_TAG(FTag_FireSignals_MinClamped);
        CK_DEFINE_ECS_TAG(FTag_FireSignals_MaxClamped);
        CK_DEFINE_ECS_TAG(FTag_MayRequireReplication);

    public:
        using AttributeDataType = T_AttributeType;
        using HandleType        = T_HandleType;

        static constexpr auto ComponentTagType  = T_ComponentTag;

    public:
        TFragment_Attribute() = default;
        explicit TFragment_Attribute(
            AttributeDataType InBase);

    private:
        AttributeDataType _Base;
        AttributeDataType _Final;

    public:
        CK_PROPERTY_GET(_Base);
        CK_PROPERTY_GET(_Final);

        CK_DEFINE_CONSTRUCTOR(TFragment_Attribute, _Base, _Final);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // we store the previous values of the Attribute in another Fragment for the following reasons:
    // - avoid bloating up the Attribute with previous values
    // - only store previous values for Attributes that need it (ones that are modified)
    template <typename T_AttributeType>
    struct TFragment_Attribute_PreviousValues
    {
    public:
        CK_GENERATED_BODY(TFragment_Attribute_PreviousValues<T_AttributeType>);

    public:
        using AttributeType = T_AttributeType;
        using AttributeDataType = typename T_AttributeType::AttributeDataType;
        using HandleType = typename T_AttributeType::HandleType;

    private:
        AttributeDataType _Base;
        AttributeDataType _Final;

    public:
        CK_PROPERTY(_Base);
        CK_PROPERTY(_Final);

        CK_DEFINE_CONSTRUCTORS(TFragment_Attribute_PreviousValues, _Base, _Final);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <concepts::ValidAttributeHandleType T_HandleType, typename T_AttributeType>
    using TFragment_Attribute_Current = TFragment_Attribute<T_HandleType, T_AttributeType, ECk_MinMaxCurrent::Current>;

    template <concepts::ValidAttributeHandleType T_HandleType, typename T_AttributeType>
    using TFragment_Attribute_Min = TFragment_Attribute<T_HandleType, T_AttributeType, ECk_MinMaxCurrent::Min>;

    template <concepts::ValidAttributeHandleType T_HandleType, typename T_AttributeType>
    using TFragment_Attribute_Max = TFragment_Attribute<T_HandleType, T_AttributeType, ECk_MinMaxCurrent::Max>;

    // --------------------------------------------------------------------------------------------------------------------

    template <concepts::ValidAttributeModifierHandleType T_HandleType, typename T_DerivedAttribute>
    struct TFragment_AttributeModifier
    {
    public:
        template <concepts::ValidAttributeModifierFragment>
        friend class TUtils_AttributeModifier;

        template <typename, concepts::ValidAttributeModifierFragment, ECk_AttributeModifier_Operation, ECk_AttributeModifier_Revocability>
        friend class detail::TProcessor_AttributeModifier_Compute;

    public:
        CK_DEFINE_ECS_TAG(FTag_ModifyAdd);
        CK_DEFINE_ECS_TAG(FTag_ModifySubtract);
        CK_DEFINE_ECS_TAG(FTag_ModifyMultiply);
        CK_DEFINE_ECS_TAG(FTag_ModifyDivide);
        CK_DEFINE_ECS_TAG(FTag_ModifyOverride);
        CK_DEFINE_ECS_TAG(FTag_IsRevocableModification);
        CK_DEFINE_ECS_TAG(FTag_IsNotRevocableModification);
        CK_DEFINE_ECS_TAG(FTag_ComputeResult);

    public:
        using AttributeFragmentType = T_DerivedAttribute;
        using ThisType              = TFragment_AttributeModifier<T_HandleType, AttributeFragmentType>;
        using AttributeDataType     = typename AttributeFragmentType::AttributeDataType;
        using AttributeHandleType   = typename AttributeFragmentType::HandleType;
        using HandleType            = T_HandleType;

    public:
        TFragment_AttributeModifier();
        explicit
        TFragment_AttributeModifier(
            AttributeDataType InModifierDelta,
            ECk_AttributeModifier_Operation InOperation);

    private:
        TOptional<AttributeDataType> _ModifierDelta;
        ECk_AttributeModifier_Operation _Operation;

    public:
        CK_PROPERTY_GET(_ModifierDelta);
        CK_PROPERTY_GET(_Operation);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_Handle>
    struct TFragment_RecordOfAttributeModifiers : public TFragment_RecordOfEntities<T_Handle>
    {
        using TFragment_RecordOfEntities<T_Handle>::TFragment_RecordOfEntities;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <concepts::ValidAttributeFragment T_DerivedAttribute>
    struct TPayload_Attribute_OnValueChanged
    {
        using AttributeFragmentType   = T_DerivedAttribute;
        using ThisType                = TPayload_Attribute_OnValueChanged<AttributeFragmentType>;
        using AttributeDataType       = typename AttributeFragmentType::AttributeDataType;
        using HandleType              = typename AttributeFragmentType::HandleType;

    public:
        CK_GENERATED_BODY(TPayload_Attribute_OnValueChanged<T_DerivedAttribute>);

    private:
        HandleType _Handle;
        AttributeDataType _BaseValue;
        AttributeDataType _FinalValue;

        AttributeDataType _BaseValue_Previous;
        AttributeDataType _FinalValue_Previous;

    public:
        CK_PROPERTY_GET(_Handle);
        CK_PROPERTY_GET(_BaseValue);
        CK_PROPERTY_GET(_FinalValue);

        CK_PROPERTY_GET(_BaseValue_Previous);
        CK_PROPERTY_GET(_FinalValue_Previous);

        CK_DEFINE_CONSTRUCTORS(TPayload_Attribute_OnValueChanged<T_DerivedAttribute>,
            _Handle, _BaseValue, _FinalValue, _BaseValue_Previous, _FinalValue_Previous);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <concepts::ValidAttributeFragment T_DerivedAttribute_Current>
    struct TPayload_Attribute_OnClamped
    {
        using AttributeFragmentType_Current   = T_DerivedAttribute_Current;
        using ThisType                        = TPayload_Attribute_OnClamped<T_DerivedAttribute_Current>;
        using AttributeDataType               = typename AttributeFragmentType_Current::AttributeDataType;
        using HandleType                      = typename AttributeFragmentType_Current::HandleType;

    public:
        CK_GENERATED_BODY(TPayload_Attribute_OnClamped<T_DerivedAttribute_Current>);

    private:
        HandleType _Handle;
        AttributeDataType _ClampedFinalValue;

    public:
        CK_PROPERTY_GET(_Handle);
        CK_PROPERTY_GET(_ClampedFinalValue);

        CK_DEFINE_CONSTRUCTORS(TPayload_Attribute_OnClamped<T_DerivedAttribute_Current>,
            _Handle, _ClampedFinalValue);
    };

    // --------------------------------------------------------------------------------------------------------------------
    // Signals

    template<typename T_DerivedAttribute>
    struct TFragment_Signal_OnAttributeValueChanged : public TFragment_Signal
    <
        FCk_Handle,
        TPayload_Attribute_OnValueChanged<T_DerivedAttribute>
    > {};

    // --------------------------------------------------------------------------------------------------------------------

    template<typename T_DerivedAttribute, typename T_Multicast>
    struct TFragment_Signal_Delegate_OnAttributeValueChanged : public TFragment_Signal_Delegate
    <
        T_Multicast,
        ECk_Signal_PostFireBehavior::DoNothing,
        FCk_Handle,
        TPayload_Attribute_OnValueChanged<T_DerivedAttribute>
    > {};

    template<typename T_DerivedAttribute, typename T_Multicast>
    struct TFragment_Signal_Delegate_OnAttributeValueChanged_PostFireUnbind : public TFragment_Signal_Delegate
    <
        T_Multicast,
        ECk_Signal_PostFireBehavior::Unbind,
        FCk_Handle,
        TPayload_Attribute_OnValueChanged<T_DerivedAttribute>
    > {};

    // --------------------------------------------------------------------------------------------------------------------

    template<typename T_DerivedAttribute, typename T_Multicast>
    class TUtils_Signal_OnAttributeValueChanged : public TUtils_Signal_Delegate
    <
        TFragment_Signal_OnAttributeValueChanged<T_DerivedAttribute>,
        TFragment_Signal_Delegate_OnAttributeValueChanged<T_DerivedAttribute, T_Multicast>
    > {};

    template<typename T_DerivedAttribute, typename T_Multicast>
    class TUtils_Signal_OnAttributeValueChanged_PostFireUnbind : public TUtils_Signal_Delegate
    <
        TFragment_Signal_OnAttributeValueChanged<T_DerivedAttribute>,
        TFragment_Signal_Delegate_OnAttributeValueChanged_PostFireUnbind<T_DerivedAttribute, T_Multicast>
    > {};

    // --------------------------------------------------------------------------------------------------------------------

    template<typename T_DerivedAttribute_Current, typename T_DerivedAttribute_Bounds>
    struct TFragment_Signal_OnAttributeClamped : public TFragment_Signal
    <
        FCk_Handle,
        TPayload_Attribute_OnClamped<T_DerivedAttribute_Current>
    > {};

    // --------------------------------------------------------------------------------------------------------------------

    template<typename T_DerivedAttribute_Current, typename T_DerivedAttribute_Bounds, typename T_Multicast>
    struct TFragment_Signal_Delegate_OnAttributeClamped : public TFragment_Signal_Delegate
    <
        T_Multicast,
        ECk_Signal_PostFireBehavior::DoNothing,
        FCk_Handle,
        TPayload_Attribute_OnClamped<T_DerivedAttribute_Current>
    > {};

    template<typename T_DerivedAttribute_Current, typename T_DerivedAttribute_Bounds, typename T_Multicast>
    struct TFragment_Signal_Delegate_OnAttributeClamped_PostFireUnbind : public TFragment_Signal_Delegate
    <
        T_Multicast,
        ECk_Signal_PostFireBehavior::Unbind,
        FCk_Handle,
        TPayload_Attribute_OnClamped<T_DerivedAttribute_Current>
    > {};

    // --------------------------------------------------------------------------------------------------------------------

    template<typename T_DerivedAttribute_Current, typename T_DerivedAttribute_Bounds, typename T_Multicast>
    class TUtils_Signal_OnAttributeClamped : public TUtils_Signal_Delegate
    <
        TFragment_Signal_OnAttributeClamped<T_DerivedAttribute_Current, T_DerivedAttribute_Bounds>,
        TFragment_Signal_Delegate_OnAttributeClamped<T_DerivedAttribute_Current, T_DerivedAttribute_Bounds, T_Multicast>
    > {};

    template<typename T_DerivedAttribute_Current, typename T_DerivedAttribute_Bounds, typename T_Multicast>
    class TUtils_Signal_OnAttributeClamped_PostFireUnbind : public TUtils_Signal_Delegate
    <
        TFragment_Signal_OnAttributeClamped<T_DerivedAttribute_Current, T_DerivedAttribute_Bounds>,
        TFragment_Signal_Delegate_OnAttributeClamped_PostFireUnbind<T_DerivedAttribute_Current, T_DerivedAttribute_Bounds, T_Multicast>
    > {};
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    template <concepts::ValidAttributeModifierFragment T_DerivedAttributeModifier, typename... T_Tags>
    struct MatchesAttributeModifierWithOperation
    {
        auto operator()(const typename T_DerivedAttributeModifier::HandleType& InModifier) const -> bool
        {
            if (NOT InModifier.template Has<T_DerivedAttributeModifier>())
            { return {}; }

            if (NOT InModifier.template Has_All<T_Tags...>())
            { return {}; }

            return InModifier.template Get<T_DerivedAttributeModifier>().Get_Operation() == _Operation;
        }

    private:
        ECk_AttributeModifier_Operation _Operation = ECk_AttributeModifier_Operation::Add;

    public:
        CK_DEFINE_CONSTRUCTORS(MatchesAttributeModifierWithOperation, _Operation);
    };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_BaseFinalEntry>
    struct TFragment_PendingReplicationEntries
    {
        CK_GENERATED_BODY(TFragment_PendingReplicationEntries<T_BaseFinalEntry>);

        TArray<T_BaseFinalEntry> _PendingEntries;
    };
}

// --------------------------------------------------------------------------------------------------------------------

// ReSharper disable once CppUnusedIncludeDirective
#include "CkAttribute_Fragment.inl.h"
