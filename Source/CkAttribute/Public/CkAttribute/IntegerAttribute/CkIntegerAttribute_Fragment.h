#pragma once

#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Fragment_Data.h"

#include "CkCore/TypeConverter/CkTypeConverter.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkIntegerAttribute_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <ECk_MinMaxCurrent T_Component>
    struct CKATTRIBUTE_API TFragment_IntegerAttribute : public TFragment_Attribute<FCk_Handle_IntegerAttribute, int32, T_Component>
    {
        using TFragment_Attribute<FCk_Handle_IntegerAttribute, int32, T_Component>::TFragment_Attribute;
    };

    using FFragment_IntegerAttribute_Current = TFragment_IntegerAttribute<ECk_MinMaxCurrent::Current>;
    using FFragment_IntegerAttribute_Min = TFragment_IntegerAttribute<ECk_MinMaxCurrent::Min>;
    using FFragment_IntegerAttribute_Max = TFragment_IntegerAttribute<ECk_MinMaxCurrent::Max>;

    // --------------------------------------------------------------------------------------------------------------------

    template <ECk_MinMaxCurrent T_Component>
    struct CKATTRIBUTE_API TFragment_IntegerAttributeModifier : public TFragment_AttributeModifier<
        FCk_Handle_IntegerAttributeModifier, TFragment_IntegerAttribute<T_Component>>
    {
        using TFragment_AttributeModifier<FCk_Handle_IntegerAttributeModifier, TFragment_IntegerAttribute<T_Component>>::TFragment_AttributeModifier;
    };

    using FFragment_IntegerAttributeModifier_Current = TFragment_IntegerAttributeModifier<ECk_MinMaxCurrent::Current>;
    using FFragment_IntegerAttributeModifier_Min = TFragment_IntegerAttributeModifier<ECk_MinMaxCurrent::Min>;
    using FFragment_IntegerAttributeModifier_Max = TFragment_IntegerAttributeModifier<ECk_MinMaxCurrent::Max>;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfIntegerAttributes, FCk_Handle_IntegerAttribute);

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TTypeConverter<TPayload_Attribute_OnValueChanged<FFragment_IntegerAttribute_Current>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnValueChanged<FFragment_IntegerAttribute_Current>& InPayload) const
        {
            return FCk_Payload_IntegerAttribute_OnValueChanged
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
    struct TTypeConverter<TPayload_Attribute_OnValueChanged<FFragment_IntegerAttribute_Min>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnValueChanged<FFragment_IntegerAttribute_Min>& InPayload) const
        {
            return FCk_Payload_IntegerAttribute_OnValueChanged
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
    struct TTypeConverter<TPayload_Attribute_OnValueChanged<FFragment_IntegerAttribute_Max>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnValueChanged<FFragment_IntegerAttribute_Max>& InPayload) const
        {
            return FCk_Payload_IntegerAttribute_OnValueChanged
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
    struct TTypeConverter<TPayload_Attribute_OnClamped<FFragment_IntegerAttribute_Current>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnClamped<FFragment_IntegerAttribute_Current>& InPayload) const
        {
            return FCk_Payload_IntegerAttribute_OnClamped
            {
                InPayload.Get_Handle(),
                InPayload.Get_ClampedFinalValue()
            };
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Value Changed
    using UUtils_Signal_OnIntegerAttributeValueChanged_Current = TUtils_Signal_OnAttributeValueChanged<
        FFragment_IntegerAttribute_Current, FCk_Delegate_IntegerAttribute_OnValueChanged>;

    using UUtils_Signal_OnIntegerAttributeValueChanged_Current_PostFireUnbind = TUtils_Signal_OnAttributeValueChanged_PostFireUnbind<
        FFragment_IntegerAttribute_Current, FCk_Delegate_IntegerAttribute_OnValueChanged>;

    using UUtils_Signal_OnIntegerAttributeValueChanged_Min = TUtils_Signal_OnAttributeValueChanged<
        FFragment_IntegerAttribute_Min, FCk_Delegate_IntegerAttribute_OnValueChanged>;

    using UUtils_Signal_OnIntegerAttributeValueChanged_Min_PostFireUnbind = TUtils_Signal_OnAttributeValueChanged_PostFireUnbind<
        FFragment_IntegerAttribute_Min, FCk_Delegate_IntegerAttribute_OnValueChanged>;

    using UUtils_Signal_OnIntegerAttributeValueChanged_Max = TUtils_Signal_OnAttributeValueChanged<
        FFragment_IntegerAttribute_Max, FCk_Delegate_IntegerAttribute_OnValueChanged>;

    using UUtils_Signal_OnIntegerAttributeValueChanged_Max_PostFireUnbind = TUtils_Signal_OnAttributeValueChanged_PostFireUnbind<
        FFragment_IntegerAttribute_Max, FCk_Delegate_IntegerAttribute_OnValueChanged>;

    // Clamped
    using UUtils_Signal_OnIntegerAttributeMinClamped = TUtils_Signal_OnAttributeClamped<
        FFragment_IntegerAttribute_Current, FFragment_IntegerAttribute_Min, FCk_Delegate_IntegerAttribute_OnClamped>;

    using UUtils_Signal_OnIntegerAttributeMinClamped_PostFireUnbind = TUtils_Signal_OnAttributeClamped_PostFireUnbind<
        FFragment_IntegerAttribute_Current, FFragment_IntegerAttribute_Min, FCk_Delegate_IntegerAttribute_OnClamped>;

    using UUtils_Signal_OnIntegerAttributeMaxClamped = TUtils_Signal_OnAttributeClamped<
        FFragment_IntegerAttribute_Current, FFragment_IntegerAttribute_Max, FCk_Delegate_IntegerAttribute_OnClamped>;

    using UUtils_Signal_OnIntegerAttributeMaxClamped_PostFireUnbind = TUtils_Signal_OnAttributeClamped_PostFireUnbind<
        FFragment_IntegerAttribute_Current, FFragment_IntegerAttribute_Max, FCk_Delegate_IntegerAttribute_OnClamped>;

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_IntegerAttribute_OverrideModifier
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Fragment_IntegerAttribute_OverrideModifier);

private:
    UPROPERTY()
    FGameplayTag _AttributeName;

    UPROPERTY()
    FGameplayTag _ModifierName;

    UPROPERTY()
    int32 _NewDelta = 0.0f;

    UPROPERTY()
    ECk_MinMaxCurrent _Component = ECk_MinMaxCurrent::Current;

public:
    CK_PROPERTY_GET(_AttributeName);
    CK_PROPERTY_GET(_ModifierName);
    CK_PROPERTY_GET(_NewDelta);
    CK_PROPERTY_GET(_Component);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_IntegerAttribute_OverrideModifier, _AttributeName, _ModifierName, _NewDelta, _Component);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_IntegerAttribute_PendingModifier
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Fragment_IntegerAttribute_PendingModifier);

private:
    UPROPERTY()
    FGameplayTag _ModifierName;

    UPROPERTY()
    FCk_Fragment_IntegerAttributeModifier_ParamsData _Params;

public:
    CK_PROPERTY_GET(_ModifierName);
    CK_PROPERTY_GET(_Params);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_IntegerAttribute_PendingModifier, _ModifierName, _Params);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_IntegerAttribute_RemovePendingModifier
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Fragment_IntegerAttribute_RemovePendingModifier);

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

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_IntegerAttribute_RemovePendingModifier, _AttributeName, _ModifierName, _Component);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_IntegerAttribute_BaseFinal
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Fragment_IntegerAttribute_BaseFinal);

private:
    UPROPERTY()
    FGameplayTag _AttributeName;

    UPROPERTY()
    int32 _Base = 0.0f;

    UPROPERTY()
    int32 _Final = 0.0f;

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

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_IntegerAttribute_BaseFinal, _AttributeName, _Base, _Final, _Component);
};

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_Fragment_IntegerAttribute_BaseFinal, [](const FCk_Fragment_IntegerAttribute_BaseFinal& InObj)
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
struct CKATTRIBUTE_API FCk_RepData_IntegerAttributes
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_RepData_IntegerAttributes);

    UPROPERTY()
    TArray<FCk_Fragment_IntegerAttribute_BaseFinal> Attributes;
};

namespace ck
{
    using FFragment_ContainerRef_IntegerAttributes = TFragment_ContainerEntryRef<FCk_RepData_IntegerAttributes>;
    using FFragment_IntegerAttribute_PendingReplicationEntries = TFragment_PendingReplicationEntries<FCk_Fragment_IntegerAttribute_BaseFinal>;
}

// --------------------------------------------------------------------------------------------------------------------