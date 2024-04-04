#pragma once

#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"

#include "CkCore/TypeConverter/CkTypeConverter.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

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

    using UUtils_Signal_OnFloatAttributeValueChanged_Current = TUtils_Signal_OnAttributeValueChanged<
        FFragment_FloatAttribute_Current, FCk_Delegate_FloatAttribute_OnValueChanged_MC>;

    using UUtils_Signal_OnFloatAttributeValueChanged_Current_PostFireUnbind = TUtils_Signal_OnAttributeValueChanged_PostFireUnbind<
        FFragment_FloatAttribute_Current, FCk_Delegate_FloatAttribute_OnValueChanged_MC>;

    using UUtils_Signal_OnFloatAttributeValueChanged_Min = TUtils_Signal_OnAttributeValueChanged<
        FFragment_FloatAttribute_Min, FCk_Delegate_FloatAttribute_OnValueChanged_MC>;

    using UUtils_Signal_OnFloatAttributeValueChanged_Min_PostFireUnbind = TUtils_Signal_OnAttributeValueChanged_PostFireUnbind<
        FFragment_FloatAttribute_Min, FCk_Delegate_FloatAttribute_OnValueChanged_MC>;

    using UUtils_Signal_OnFloatAttributeValueChanged_Max = TUtils_Signal_OnAttributeValueChanged<
        FFragment_FloatAttribute_Max, FCk_Delegate_FloatAttribute_OnValueChanged_MC>;

    using UUtils_Signal_OnFloatAttributeValueChanged_Max_PostFireUnbind = TUtils_Signal_OnAttributeValueChanged_PostFireUnbind<
        FFragment_FloatAttribute_Max, FCk_Delegate_FloatAttribute_OnValueChanged_MC>;


    // --------------------------------------------------------------------------------------------------------------------

    class FProcessor_FloatAttribute_Replicate;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_FloatAttribute_PendingModifier
{
    GENERATED_BODY()

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

UCLASS(Blueprintable)
class CKATTRIBUTE_API UCk_Fragment_FloatAttribute_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Fragment_FloatAttribute_Rep);
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_FloatAttribute_Rep);

public:
    friend class ck::FProcessor_FloatAttribute_Replicate;

public:
    auto
    Broadcast_AddModifier(
        FGameplayTag InModifierName,
        const FCk_Fragment_FloatAttributeModifier_ParamsData& InParams) -> void;

    auto
    Broadcast_RemoveModifier(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName,
        ECk_MinMaxCurrent InAttributeComponent) -> void;

    // TODO: 'permanent' modifiers

private:
    auto
    PostLink() -> void override;;

private:
    auto
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const -> void override;

private:
    UFUNCTION()
    void
    OnRep_PendingModifiers();

    UPROPERTY(ReplicatedUsing = OnRep_PendingModifiers)
    TArray<FCk_Fragment_FloatAttribute_PendingModifier> _PendingAddModifiers;
    int32 _NextPendingAddModifier = 0;

    UPROPERTY(ReplicatedUsing = OnRep_PendingModifiers)
    TArray<FCk_Fragment_FloatAttribute_RemovePendingModifier> _PendingRemoveModifiers;
    int32 _NextPendingRemoveModifier = 0;
};

// --------------------------------------------------------------------------------------------------------------------