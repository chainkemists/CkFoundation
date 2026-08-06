#pragma once

#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/RotatorAttribute/CkRotatorAttribute_Fragment_Data.h"

#include "CkCore/TypeConverter/CkTypeConverter.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkRotatorAttribute_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <ECk_MinMaxCurrent T_Component>
    struct CKATTRIBUTE_API TFragment_RotatorAttribute : public TFragment_Attribute<FCk_Handle_RotatorAttribute, FRotator, T_Component>
    {
        using TFragment_Attribute<FCk_Handle_RotatorAttribute, FRotator, T_Component>::TFragment_Attribute;
    };

    using FFragment_RotatorAttribute_Current = TFragment_RotatorAttribute<ECk_MinMaxCurrent::Current>;
    using FFragment_RotatorAttribute_Min = TFragment_RotatorAttribute<ECk_MinMaxCurrent::Min>;
    using FFragment_RotatorAttribute_Max = TFragment_RotatorAttribute<ECk_MinMaxCurrent::Max>;

    // --------------------------------------------------------------------------------------------------------------------

    template <ECk_MinMaxCurrent T_Component>
    struct CKATTRIBUTE_API TFragment_RotatorAttributeModifier : public TFragment_AttributeModifier<
        FCk_Handle_RotatorAttributeModifier, TFragment_RotatorAttribute<T_Component>>
    {
        using TFragment_AttributeModifier<FCk_Handle_RotatorAttributeModifier, TFragment_RotatorAttribute<T_Component>>::TFragment_AttributeModifier;
    };

    using FFragment_RotatorAttributeModifier_Current = TFragment_RotatorAttributeModifier<ECk_MinMaxCurrent::Current>;
    using FFragment_RotatorAttributeModifier_Min = TFragment_RotatorAttributeModifier<ECk_MinMaxCurrent::Min>;
    using FFragment_RotatorAttributeModifier_Max = TFragment_RotatorAttributeModifier<ECk_MinMaxCurrent::Max>;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_ROUNDTRIP(FFragment_RecordOfRotatorAttributes, FCk_Handle_RotatorAttribute);

    // --------------------------------------------------------------------------------------------------------------------
    // FRotator has no operator* / operator/ / operator< — provide component-wise behavior so the generic
    // attribute compute (Mul/Div) and clamp (Min/Max) templates instantiate for FRotator.

    template <>
    struct TAttributeModifierOperators<FRotator>
    {
        static auto Add(FRotator A, FRotator B) -> FRotator { return A + B; }
        static auto Sub(FRotator A, FRotator B) -> FRotator { return A - B; }
        static auto Mul(FRotator A, FRotator B) -> FRotator
        {
            return FRotator{A.Pitch * B.Pitch, A.Yaw * B.Yaw, A.Roll * B.Roll};
        }
        static auto Div(FRotator A, FRotator B) -> FRotator
        {
            return FRotator{A.Pitch / B.Pitch, A.Yaw / B.Yaw, A.Roll / B.Roll};
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TAttributeMinMax<FRotator>
    {
        static auto Min(FRotator A, FRotator B) -> FRotator
        {
            const auto Pitch = FMath::Min(A.Pitch, B.Pitch);
            const auto Yaw   = FMath::Min(A.Yaw, B.Yaw);
            const auto Roll  = FMath::Min(A.Roll, B.Roll);

            return FRotator{Pitch, Yaw, Roll};
        }

        static auto Max(FRotator A, FRotator B) -> FRotator
        {
            const auto Pitch = FMath::Max(A.Pitch, B.Pitch);
            const auto Yaw   = FMath::Max(A.Yaw, B.Yaw);
            const auto Roll  = FMath::Max(A.Roll, B.Roll);

            return FRotator{Pitch, Yaw, Roll};
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TTypeConverter<TPayload_Attribute_OnValueChanged<FFragment_RotatorAttribute_Current>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnValueChanged<FFragment_RotatorAttribute_Current>& InPayload) const
        {
            return FCk_Payload_RotatorAttribute_OnValueChanged
            {
                InPayload.Get_Handle(),
                InPayload.Get_BaseValue(),
                InPayload.Get_FinalValue(),
                InPayload.Get_BaseValue_Previous(),
                InPayload.Get_FinalValue_Previous()
            };
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TTypeConverter<TPayload_Attribute_OnClamped<FFragment_RotatorAttribute_Current>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnClamped<FFragment_RotatorAttribute_Current>& InPayload) const
        {
            return FCk_Payload_RotatorAttribute_OnClamped
            {
                InPayload.Get_Handle(),
                InPayload.Get_PreClampFinalValue(),
                InPayload.Get_ClampedFinalValue(),
                InPayload.Get_ClampOverflow()
            };
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TTypeConverter<TPayload_Attribute_OnValueChanged<FFragment_RotatorAttribute_Min>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnValueChanged<FFragment_RotatorAttribute_Min>& InPayload) const
        {
            return FCk_Payload_RotatorAttribute_OnValueChanged
            {
                InPayload.Get_Handle(),
                InPayload.Get_BaseValue(),
                InPayload.Get_FinalValue(),
                InPayload.Get_BaseValue_Previous(),
                InPayload.Get_FinalValue_Previous()
            };
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TTypeConverter<TPayload_Attribute_OnValueChanged<FFragment_RotatorAttribute_Max>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnValueChanged<FFragment_RotatorAttribute_Max>& InPayload) const
        {
            return FCk_Payload_RotatorAttribute_OnValueChanged
            {
                InPayload.Get_Handle(),
                InPayload.Get_BaseValue(),
                InPayload.Get_FinalValue(),
                InPayload.Get_BaseValue_Previous(),
                InPayload.Get_FinalValue_Previous()
            };
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Value Changed
    using UUtils_Signal_OnRotatorAttributeValueChanged_Current = TUtils_Signal_OnAttributeValueChanged<
        FFragment_RotatorAttribute_Current, FCk_Delegate_RotatorAttribute_OnValueChanged>;

    using UUtils_Signal_OnRotatorAttributeValueChanged_Current_PostFireUnbind = TUtils_Signal_OnAttributeValueChanged_PostFireUnbind<
        FFragment_RotatorAttribute_Current, FCk_Delegate_RotatorAttribute_OnValueChanged>;

    using UUtils_Signal_OnRotatorAttributeValueChanged_Min = TUtils_Signal_OnAttributeValueChanged<
        FFragment_RotatorAttribute_Min, FCk_Delegate_RotatorAttribute_OnValueChanged>;

    using UUtils_Signal_OnRotatorAttributeValueChanged_Min_PostFireUnbind = TUtils_Signal_OnAttributeValueChanged_PostFireUnbind<
        FFragment_RotatorAttribute_Min, FCk_Delegate_RotatorAttribute_OnValueChanged>;

    using UUtils_Signal_OnRotatorAttributeValueChanged_Max = TUtils_Signal_OnAttributeValueChanged<
        FFragment_RotatorAttribute_Max, FCk_Delegate_RotatorAttribute_OnValueChanged>;

    using UUtils_Signal_OnRotatorAttributeValueChanged_Max_PostFireUnbind = TUtils_Signal_OnAttributeValueChanged_PostFireUnbind<
        FFragment_RotatorAttribute_Max, FCk_Delegate_RotatorAttribute_OnValueChanged>;

    // Clamped
    using UUtils_Signal_OnRotatorAttributeMinClamped = TUtils_Signal_OnAttributeClamped<
        FFragment_RotatorAttribute_Current, FFragment_RotatorAttribute_Min, FCk_Delegate_RotatorAttribute_OnClamped>;

    using UUtils_Signal_OnRotatorAttributeMinClamped_PostFireUnbind = TUtils_Signal_OnAttributeClamped_PostFireUnbind<
        FFragment_RotatorAttribute_Current, FFragment_RotatorAttribute_Min, FCk_Delegate_RotatorAttribute_OnClamped>;

    using UUtils_Signal_OnRotatorAttributeMaxClamped = TUtils_Signal_OnAttributeClamped<
        FFragment_RotatorAttribute_Current, FFragment_RotatorAttribute_Max, FCk_Delegate_RotatorAttribute_OnClamped>;

    using UUtils_Signal_OnRotatorAttributeMaxClamped_PostFireUnbind = TUtils_Signal_OnAttributeClamped_PostFireUnbind<
        FFragment_RotatorAttribute_Current, FFragment_RotatorAttribute_Max, FCk_Delegate_RotatorAttribute_OnClamped>;

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_RotatorAttribute_OverrideModifier
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Fragment_RotatorAttribute_OverrideModifier);

private:
    UPROPERTY()
    FGameplayTag _AttributeName;

    UPROPERTY()
    FGameplayTag _ModifierName;

    UPROPERTY()
    FRotator _NewDelta = FRotator::ZeroRotator;

    UPROPERTY()
    ECk_MinMaxCurrent _Component = ECk_MinMaxCurrent::Current;

public:
    CK_PROPERTY_GET(_AttributeName);
    CK_PROPERTY_GET(_ModifierName);
    CK_PROPERTY_GET(_NewDelta);
    CK_PROPERTY_GET(_Component);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_RotatorAttribute_OverrideModifier, _AttributeName, _ModifierName, _NewDelta, _Component);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_RotatorAttribute_PendingModifier
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Fragment_RotatorAttribute_PendingModifier);

private:
    UPROPERTY()
    FGameplayTag _ModifierName;

    UPROPERTY()
    FCk_RotatorAttributeModifier_Spec _Params;

public:
    CK_PROPERTY_GET(_ModifierName);
    CK_PROPERTY_GET(_Params);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_RotatorAttribute_PendingModifier, _ModifierName, _Params);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_RotatorAttribute_RemovePendingModifier
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Fragment_RotatorAttribute_RemovePendingModifier);

private:
    UPROPERTY()
    FGameplayTag _AttributeName;

    UPROPERTY()
    FGameplayTag _ModifierName;

    UPROPERTY()
    ECk_MinMaxCurrent _Component = ECk_MinMaxCurrent::Current;

public:
    CK_PROPERTY_GET(_AttributeName);
    CK_PROPERTY_GET(_ModifierName);
    CK_PROPERTY_GET(_Component);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_RotatorAttribute_RemovePendingModifier, _AttributeName, _ModifierName, _Component);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_RotatorAttribute_BaseFinal
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Fragment_RotatorAttribute_BaseFinal);

private:
    UPROPERTY()
    FGameplayTag _AttributeName;

    UPROPERTY()
    FRotator _Base = FRotator::ZeroRotator;

    UPROPERTY()
    FRotator _Final = FRotator::ZeroRotator;

    UPROPERTY()
    ECk_MinMaxCurrent _Component = ECk_MinMaxCurrent::Current;

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

public:
    CK_PROPERTY_GET(_AttributeName);
    CK_PROPERTY_GET(_Base);
    CK_PROPERTY_GET(_Final);
    CK_PROPERTY_GET(_Component);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_RotatorAttribute_BaseFinal, _AttributeName, _Base, _Final, _Component);
};

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_Fragment_RotatorAttribute_BaseFinal, [](const FCk_Fragment_RotatorAttribute_BaseFinal& InObj)
{
    return ck::Format
    (
        TEXT("{} [B{}|F{}][{}]"),
        InObj.Get_AttributeName(),
        InObj.Get_Base(),
        InObj.Get_Final(),
        InObj.Get_Component()
    );
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKATTRIBUTE_API FCk_RepData_RotatorAttributes
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_RotatorAttributes);

    UPROPERTY()
    TArray<FCk_Fragment_RotatorAttribute_BaseFinal> Attributes;
};

namespace ck
{
    using FFragment_ContainerRef_RotatorAttributes = TFragment_ContainerEntryRef<FCk_RepData_RotatorAttributes>;
}

// --------------------------------------------------------------------------------------------------------------------
