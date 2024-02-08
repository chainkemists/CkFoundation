#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcsBasics/EntityHolder/CkEntityHolder_Fragment.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkSignal/CkSignal_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedAttribute>
    class TUtils_Attribute;

    template <typename T_DerivedAttributeModifier>
    class TUtils_AttributeModifier;

    template <typename T_DerivedProcessor, typename T_DerivedAttribute, typename T_MulticastType>
    class TProcessor_Attribute_FireSignals;

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_Attribute_RecomputeAll;

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_RevokableAdditive_Compute;

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_NotRevokableAdditive_Compute;

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_Additive_Teardown;

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_RevokableMultiplicative_Compute;

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_NotRevokableMultiplicative_Compute;

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_Multiplicative_Teardown;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T>
    struct TAttributeModifierOperators
    {
        static auto Add(T, T) -> T = delete;
        static auto Multiply(T, T) -> T = delete;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_AttributeType>
    struct TFragment_Attribute
    {
    public:
        template <typename>
        friend class TUtils_Attribute;

        template <typename, typename, typename>
        friend class TProcessor_Attribute_FireSignals;

        template <typename, typename>
        friend class TProcessor_Attribute_MinClamp;

        template <typename, typename>
        friend class TProcessor_Attribute_MaxClamp;

        template <typename, typename>
        friend class TProcessor_Attribute_OverrideBaseValue;

        template <typename, typename>
        friend class TProcessor_Attribute_RecomputeAll;

        template <typename, typename>
        friend class TProcessor_AttributeModifier_RevokableAdditive_Compute;

        template <typename, typename>
        friend class TProcessor_AttributeModifier_NotRevokableAdditive_Compute;

        template <typename, typename>
        friend class TProcessor_AttributeModifier_RevokableMultiplicative_Compute;

        template <typename, typename>
        friend class TProcessor_AttributeModifier_NotRevokableMultiplicative_Compute;

    public:
        CK_GENERATED_BODY(TFragment_Attribute<T_AttributeType>);

    public:
        CK_DEFINE_ECS_TAG(Tag_RecomputeFinalValue);
        CK_DEFINE_ECS_TAG(Tag_FireSignals);

        CK_DEFINE_ECS_TAG(FTag_MinValue);
        CK_DEFINE_ECS_TAG(FTag_MaxValue);
        CK_DEFINE_ECS_TAG(FTag_RequiresUpdate);

    public:
        using AttributeDataType = T_AttributeType;
        using ThisType          = TFragment_Attribute<AttributeDataType>;
        using HandleType        = FCk_Handle;

    public:
        TFragment_Attribute() = default;
        explicit TFragment_Attribute(
            AttributeDataType InBase);

    private:
        AttributeDataType _Base;
        AttributeDataType _Final;

        AttributeDataType _Min;
        AttributeDataType _Max;

    public:
        CK_PROPERTY_GET(_Base);
        CK_PROPERTY_GET(_Final);
        CK_PROPERTY(_Min);
        CK_PROPERTY(_Max);

        CK_DEFINE_CONSTRUCTOR(TFragment_Attribute<T_AttributeType>, _Base, _Final);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttribute>
    struct TFragment_Request_AttributeOverride
    {
        CK_GENERATED_BODY(TFragment_Request_AttributeOverride<T_DerivedAttribute>);

    public:
        using AttributeDataType = typename T_DerivedAttribute::AttributeDataType;

    private:
        AttributeDataType _NewBaseValue;

    public:
        CK_PROPERTY_GET(_NewBaseValue);

        CK_DEFINE_CONSTRUCTORS(TFragment_Request_AttributeOverride, _NewBaseValue);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttribute>
    struct TFragment_AttributeModifier
    {
    public:
        template <typename>
        friend class TUtils_AttributeModifier;

    public:
        CK_DEFINE_ECS_TAG(Tag_AdditiveModification);
        CK_DEFINE_ECS_TAG(Tag_MultiplicativeModification);
        CK_DEFINE_ECS_TAG(Tag_IsRevokableModification);
        CK_DEFINE_ECS_TAG(Tag_IsNotRevokableModification);
        CK_DEFINE_ECS_TAG(Tag_ComputeResult);

    public:
        using AttributeFragmentType = T_DerivedAttribute;
        using ThisType              = TFragment_AttributeModifier<AttributeFragmentType>;
        using AttributeDataType     = typename AttributeFragmentType::AttributeDataType;
        using HandleType            = FCk_Handle;

    public:
        TFragment_AttributeModifier() = default;
        explicit TFragment_AttributeModifier(
            AttributeDataType InModifierDelta);

    private:
        AttributeDataType _ModifierDelta;

    public:
        CK_PROPERTY_GET(_ModifierDelta);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_AttributeModifierTarget : public FFragment_EntityHolder
    {
        using FFragment_EntityHolder::FFragment_EntityHolder;
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfAttributeModifiers, FCk_Handle);

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttribute>
    struct TPayload_Attribute_OnValueChanged
    {
        using AttributeFragmentType   = T_DerivedAttribute;
        using ThisType                = TPayload_Attribute_OnValueChanged<AttributeFragmentType>;
        using AttributeDataType       = typename AttributeFragmentType::AttributeDataType;
        using HandleType              = FCk_Handle;

    public:
        TPayload_Attribute_OnValueChanged(
            HandleType        InHandle,
            AttributeDataType InBaseValue,
            AttributeDataType InFinalValue);

    private:
        HandleType _Handle;
        AttributeDataType _BaseValue;
        AttributeDataType _FinalValue;

    public:
        CK_PROPERTY_GET(_Handle);
        CK_PROPERTY_GET(_BaseValue);
        CK_PROPERTY_GET(_FinalValue);
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
    struct TFragment_Signal_UnrealMulticast_OnAttributeValueChanged : public TFragment_Signal_UnrealMulticast
    <
        T_Multicast,
        ECk_Signal_PostFireBehavior::DoNothing,
        FCk_Handle,
        TPayload_Attribute_OnValueChanged<T_DerivedAttribute>
    > {};

    template<typename T_DerivedAttribute, typename T_Multicast>
    struct TFragment_Signal_UnrealMulticast_OnAttributeValueChanged_PostFireUnbind : public TFragment_Signal_UnrealMulticast
    <
        T_Multicast,
        ECk_Signal_PostFireBehavior::Unbind,
        FCk_Handle,
        TPayload_Attribute_OnValueChanged<T_DerivedAttribute>
    > {};

    // --------------------------------------------------------------------------------------------------------------------

    template<typename T_DerivedAttribute, typename T_Multicast>
    class TUtils_Signal_OnAttributeValueChanged : public TUtils_Signal_UnrealMulticast
    <
        TFragment_Signal_OnAttributeValueChanged<T_DerivedAttribute>,
        TFragment_Signal_UnrealMulticast_OnAttributeValueChanged<T_DerivedAttribute, T_Multicast>
    > {};

    template<typename T_DerivedAttribute, typename T_Multicast>
    class TUtils_Signal_OnAttributeValueChanged_PostFireUnbind : public TUtils_Signal_UnrealMulticast
    <
        TFragment_Signal_OnAttributeValueChanged<T_DerivedAttribute>,
        TFragment_Signal_UnrealMulticast_OnAttributeValueChanged_PostFireUnbind<T_DerivedAttribute, T_Multicast>
    > {};

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkAttribute_Fragment.inl.h"