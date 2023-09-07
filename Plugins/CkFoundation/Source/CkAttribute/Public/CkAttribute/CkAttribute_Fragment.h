#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcsBasics/EntityHolder/CkEntityHolder_Fragment.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedAttribute>
    class TUtils_Attribute;

    template <typename T_DerivedAttributeModifier>
    class TUtils_AttributeModifier;

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_Attribute_RecomputeAll;

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_Additive_Compute;

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_Additive_Teardown;

    template <typename T_DerivedProcessor, typename T_DerivedAttributeModifier>
    class TProcessor_AttributeModifier_Multiplicative_Compute;

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

        template <typename, typename>
        friend class TProcessor_Attribute_RecomputeAll;

        template <typename, typename>
        friend class TProcessor_AttributeModifier_Additive_Compute;

        template <typename, typename>
        friend class TProcessor_AttributeModifier_Multiplicative_Compute;

    public:
        struct Tag_RecomputeFinalValue {};
        struct Tag_DispatchDelegates {};

    public:
        using ValueType         = T_AttributeType;
        using AttributeDataType = ValueType;
        using ThisType          = TFragment_Attribute<AttributeDataType>;
        using HandleType        = FCk_Handle;

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
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttribute>
    struct TFragment_AttributeModifier
    {
    public:
        template <typename>
        friend class TUtils_AttributeModifier;

        template <typename, typename>
        friend class TProcessor_AttributeModifier_Additive_Compute;

        template <typename, typename>
        friend class TProcessor_AttributeModifier_Multiplicative_Compute;

    public:
        struct Tag_AdditiveModification{};
        struct Tag_MultiplicativeModification{};
        struct Tag_ComputeResult{};

    public:
        using ValueType             = T_DerivedAttribute;
        using AttributeFragmentType = ValueType;
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

    struct FFragment_RecordOfAttributeModifiers : public FFragment_RecordOfEntities {};

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedAttribute>
    struct TPayload_Attribute_OnFinalValueChanged
    {
        using ValueType               = T_DerivedAttribute;
        using AttributeFragmentType   = ValueType;
        using ThisType                = TPayload_Attribute_OnFinalValueChanged<AttributeFragmentType>;
        using AttributeDataType       = typename AttributeFragmentType::AttributeDataType;
        using HandleType              = FCk_Handle;

    public:
        TPayload_Attribute_OnFinalValueChanged(
            HandleType         InHandle,
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
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkAttribute_Fragment.inl.h"