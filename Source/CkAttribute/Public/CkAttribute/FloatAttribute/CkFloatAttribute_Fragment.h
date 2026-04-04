#pragma once

#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"

#include "CkCore/TypeConverter/CkTypeConverter.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkFloatAttribute_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <ECk_MinMaxCurrent T_Component>
    struct CKATTRIBUTE_API TFragment_FloatAttribute : public TFragment_Attribute<FCk_Handle_FloatAttribute, float, T_Component>
    {
        using TFragment_Attribute<FCk_Handle_FloatAttribute, float, T_Component>::TFragment_Attribute;
    };

    using FFragment_FloatAttribute_Current = TFragment_FloatAttribute<ECk_MinMaxCurrent::Current>;
    using FFragment_FloatAttribute_Min = TFragment_FloatAttribute<ECk_MinMaxCurrent::Min>;
    using FFragment_FloatAttribute_Max = TFragment_FloatAttribute<ECk_MinMaxCurrent::Max>;

    // --------------------------------------------------------------------------------------------------------------------

    template <ECk_MinMaxCurrent T_Component>
    struct CKATTRIBUTE_API TFragment_FloatAttributeModifier : public TFragment_AttributeModifier<
        FCk_Handle_FloatAttributeModifier, TFragment_FloatAttribute<T_Component>>
    {
        using TFragment_AttributeModifier<FCk_Handle_FloatAttributeModifier, TFragment_FloatAttribute<T_Component>>::TFragment_AttributeModifier;
    };

    using FFragment_FloatAttributeModifier_Current = TFragment_FloatAttributeModifier<ECk_MinMaxCurrent::Current>;
    using FFragment_FloatAttributeModifier_Min = TFragment_FloatAttributeModifier<ECk_MinMaxCurrent::Min>;
    using FFragment_FloatAttributeModifier_Max = TFragment_FloatAttributeModifier<ECk_MinMaxCurrent::Max>;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfFloatAttributes, FCk_Handle_FloatAttribute);

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TTypeConverter<TPayload_Attribute_OnValueChanged<FFragment_FloatAttribute_Current>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnValueChanged<FFragment_FloatAttribute_Current>& InPayload) const
        {
            return FCk_Payload_FloatAttribute_OnValueChanged
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
    struct TTypeConverter<TPayload_Attribute_OnValueChanged<FFragment_FloatAttribute_Min>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnValueChanged<FFragment_FloatAttribute_Min>& InPayload) const
        {
            return FCk_Payload_FloatAttribute_OnValueChanged
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
    struct TTypeConverter<TPayload_Attribute_OnValueChanged<FFragment_FloatAttribute_Max>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnValueChanged<FFragment_FloatAttribute_Max>& InPayload) const
        {
            return FCk_Payload_FloatAttribute_OnValueChanged
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
    struct TTypeConverter<TPayload_Attribute_OnClamped<FFragment_FloatAttribute_Current>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnClamped<FFragment_FloatAttribute_Current>& InPayload) const
        {
            return FCk_Payload_FloatAttribute_OnClamped
            {
                InPayload.Get_Handle(),
                InPayload.Get_ClampedFinalValue()
            };
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Value Changed
    using UUtils_Signal_OnFloatAttributeValueChanged_Current = TUtils_Signal_OnAttributeValueChanged<
        FFragment_FloatAttribute_Current, FCk_Delegate_FloatAttribute_OnValueChanged>;

    using UUtils_Signal_OnFloatAttributeValueChanged_Current_PostFireUnbind = TUtils_Signal_OnAttributeValueChanged_PostFireUnbind<
        FFragment_FloatAttribute_Current, FCk_Delegate_FloatAttribute_OnValueChanged>;

    using UUtils_Signal_OnFloatAttributeValueChanged_Min = TUtils_Signal_OnAttributeValueChanged<
        FFragment_FloatAttribute_Min, FCk_Delegate_FloatAttribute_OnValueChanged>;

    using UUtils_Signal_OnFloatAttributeValueChanged_Min_PostFireUnbind = TUtils_Signal_OnAttributeValueChanged_PostFireUnbind<
        FFragment_FloatAttribute_Min, FCk_Delegate_FloatAttribute_OnValueChanged>;

    using UUtils_Signal_OnFloatAttributeValueChanged_Max = TUtils_Signal_OnAttributeValueChanged<
        FFragment_FloatAttribute_Max, FCk_Delegate_FloatAttribute_OnValueChanged>;

    using UUtils_Signal_OnFloatAttributeValueChanged_Max_PostFireUnbind = TUtils_Signal_OnAttributeValueChanged_PostFireUnbind<
        FFragment_FloatAttribute_Max, FCk_Delegate_FloatAttribute_OnValueChanged>;

    // Clamped
    using UUtils_Signal_OnFloatAttributeMinClamped = TUtils_Signal_OnAttributeClamped<
        FFragment_FloatAttribute_Current, FFragment_FloatAttribute_Min, FCk_Delegate_FloatAttribute_OnClamped>;

    using UUtils_Signal_OnFloatAttributeMinClamped_PostFireUnbind = TUtils_Signal_OnAttributeClamped_PostFireUnbind<
        FFragment_FloatAttribute_Current, FFragment_FloatAttribute_Min, FCk_Delegate_FloatAttribute_OnClamped>;

    using UUtils_Signal_OnFloatAttributeMaxClamped = TUtils_Signal_OnAttributeClamped<
        FFragment_FloatAttribute_Current, FFragment_FloatAttribute_Max, FCk_Delegate_FloatAttribute_OnClamped>;

    using UUtils_Signal_OnFloatAttributeMaxClamped_PostFireUnbind = TUtils_Signal_OnAttributeClamped_PostFireUnbind<
        FFragment_FloatAttribute_Current, FFragment_FloatAttribute_Max, FCk_Delegate_FloatAttribute_OnClamped>;

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_FloatAttribute_OverrideModifier
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Fragment_FloatAttribute_OverrideModifier);

private:
    UPROPERTY()
    FGameplayTag _AttributeName;

    UPROPERTY()
    FGameplayTag _ModifierName;

    UPROPERTY()
    float _NewDelta = 0.0f;

    UPROPERTY()
    ECk_MinMaxCurrent _Component = ECk_MinMaxCurrent::Current;

public:
    CK_PROPERTY_GET(_AttributeName);
    CK_PROPERTY_GET(_ModifierName);
    CK_PROPERTY_GET(_NewDelta);
    CK_PROPERTY_GET(_Component);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_FloatAttribute_OverrideModifier, _AttributeName, _ModifierName, _NewDelta, _Component);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_FloatAttribute_PendingModifier
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Fragment_FloatAttribute_PendingModifier);

private:
    UPROPERTY()
    FGameplayTag _ModifierName;

    UPROPERTY()
    FCk_Fragment_FloatAttributeModifier_ParamsData _Params;

public:
    CK_PROPERTY_GET(_ModifierName);
    CK_PROPERTY_GET(_Params);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_FloatAttribute_PendingModifier, _ModifierName, _Params);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_FloatAttribute_RemovePendingModifier
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Fragment_FloatAttribute_RemovePendingModifier);

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

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_FloatAttribute_RemovePendingModifier, _AttributeName, _ModifierName, _Component);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_FloatAttribute_BaseFinal
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Fragment_FloatAttribute_BaseFinal);

private:
    UPROPERTY()
    FGameplayTag _AttributeName;

    UPROPERTY()
    float _Base = 0.0f;

    UPROPERTY()
    float _Final = 0.0f;

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

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_FloatAttribute_BaseFinal, _AttributeName, _Base, _Final, _Component);
};

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_Fragment_FloatAttribute_BaseFinal, [](const FCk_Fragment_FloatAttribute_BaseFinal& InObj)
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
struct CKATTRIBUTE_API FCk_RepData_FloatAttributes
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_FloatAttributes);

    UPROPERTY()
    TArray<FCk_Fragment_FloatAttribute_BaseFinal> Attributes;
};

namespace ck
{
    using FFragment_ContainerRef_FloatAttributes = TFragment_ContainerEntryRef<FCk_RepData_FloatAttributes>;
    using FFragment_FloatAttribute_PendingReplicationEntries = TFragment_PendingReplicationEntries<FCk_Fragment_FloatAttribute_BaseFinal>;
}

// --------------------------------------------------------------------------------------------------------------------