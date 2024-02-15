#pragma once

#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Fragment_Data.h"

#include "CkCore/TypeConverter/CkTypeConverter.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkVectorAttribute_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <ECk_MinMaxCurrent T_Component>
    struct CKATTRIBUTE_API TFragment_VectorAttribute : public TFragment_Attribute<FCk_Handle_VectorAttribute, FVector, T_Component>
    {
        using TFragment_Attribute<FCk_Handle_VectorAttribute, FVector, T_Component>::TFragment_Attribute;
    };

    using FFragment_VectorAttribute_Current = TFragment_VectorAttribute<ECk_MinMaxCurrent::Current>;
    using FFragment_VectorAttribute_Min = TFragment_VectorAttribute<ECk_MinMaxCurrent::Min>;
    using FFragment_VectorAttribute_Max = TFragment_VectorAttribute<ECk_MinMaxCurrent::Max>;

    // --------------------------------------------------------------------------------------------------------------------

    template <ECk_MinMaxCurrent T_Component>
    struct CKATTRIBUTE_API TFragment_VectorAttributeModifier : public TFragment_AttributeModifier<
        FCk_Handle_VectorAttributeModifier, TFragment_VectorAttribute<T_Component>>
    {
        using TFragment_AttributeModifier<FCk_Handle_VectorAttributeModifier, TFragment_VectorAttribute<T_Component>>::TFragment_AttributeModifier;
    };

    using FFragment_VectorAttributeModifier_Current = TFragment_VectorAttributeModifier<ECk_MinMaxCurrent::Current>;
    using FFragment_VectorAttributeModifier_Min = TFragment_VectorAttributeModifier<ECk_MinMaxCurrent::Min>;
    using FFragment_VectorAttributeModifier_Max = TFragment_VectorAttributeModifier<ECk_MinMaxCurrent::Max>;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfVectorAttributes, FCk_Handle_VectorAttribute);
    using FFragment_RecordOfVectorAttributeModifiers = TFragment_RecordOfAttributeModifiers<FCk_Handle_VectorAttributeModifier>;

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TAttributeMinMax<FVector>
    {
        static auto Min(FVector A, FVector B) -> FVector
        {
            const auto X = FMath::Min(A.X, B.X);
            const auto Y = FMath::Min(A.Y, B.Y);
            const auto Z = FMath::Min(A.Z, B.Z);

            return FVector{X, Y, Z};
        }

        static auto Max(FVector A, FVector B) -> FVector
        {
            const auto X = FMath::Max(A.X, B.X);
            const auto Y = FMath::Max(A.Y, B.Y);
            const auto Z = FMath::Max(A.Z, B.Z);

            return FVector{X, Y, Z};
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TTypeConverter<TPayload_Attribute_OnValueChanged<FFragment_VectorAttribute_Current>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnValueChanged<FFragment_VectorAttribute_Current>& InPayload) const
        {
            return FCk_Payload_VectorAttribute_OnValueChanged
            {
                InPayload.Get_Handle(),
                InPayload.Get_BaseValue(),
                InPayload.Get_FinalValue()
            };
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TTypeConverter<TPayload_Attribute_OnValueChanged<FFragment_VectorAttribute_Min>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnValueChanged<FFragment_VectorAttribute_Min>& InPayload) const
        {
            return FCk_Payload_VectorAttribute_OnValueChanged
            {
                InPayload.Get_Handle(),
                InPayload.Get_BaseValue(),
                InPayload.Get_FinalValue()
            };
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TTypeConverter<TPayload_Attribute_OnValueChanged<FFragment_VectorAttribute_Max>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnValueChanged<FFragment_VectorAttribute_Max>& InPayload) const
        {
            return FCk_Payload_VectorAttribute_OnValueChanged
            {
                InPayload.Get_Handle(),
                InPayload.Get_BaseValue(),
                InPayload.Get_FinalValue()
            };
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    using UUtils_Signal_OnVectorAttributeValueChanged_Current = TUtils_Signal_OnAttributeValueChanged<
        FFragment_VectorAttribute_Current, FCk_Delegate_VectorAttribute_OnValueChanged_MC>;

    using UUtils_Signal_OnVectorAttributeValueChanged_Current_PostFireUnbind = TUtils_Signal_OnAttributeValueChanged_PostFireUnbind<
        FFragment_VectorAttribute_Current, FCk_Delegate_VectorAttribute_OnValueChanged_MC>;

    using UUtils_Signal_OnVectorAttributeValueChanged_Min = TUtils_Signal_OnAttributeValueChanged<
        FFragment_VectorAttribute_Min, FCk_Delegate_VectorAttribute_OnValueChanged_MC>;

    using UUtils_Signal_OnVectorAttributeValueChanged_Min_PostFireUnbind = TUtils_Signal_OnAttributeValueChanged_PostFireUnbind<
        FFragment_VectorAttribute_Min, FCk_Delegate_VectorAttribute_OnValueChanged_MC>;

    using UUtils_Signal_OnVectorAttributeValueChanged_Max = TUtils_Signal_OnAttributeValueChanged<
        FFragment_VectorAttribute_Max, FCk_Delegate_VectorAttribute_OnValueChanged_MC>;

    using UUtils_Signal_OnVectorAttributeValueChanged_Max_PostFireUnbind = TUtils_Signal_OnAttributeValueChanged_PostFireUnbind<
        FFragment_VectorAttribute_Max, FCk_Delegate_VectorAttribute_OnValueChanged_MC>;


    // --------------------------------------------------------------------------------------------------------------------

    class FProcessor_VectorAttribute_Replicate;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_VectorAttribute_PendingModifier
{
    GENERATED_BODY()

private:
    UPROPERTY()
    FGameplayTag _ModifierName;

    UPROPERTY()
    FCk_Fragment_VectorAttributeModifier_ParamsData _Params;

public:
    CK_PROPERTY_GET(_ModifierName);
    CK_PROPERTY_GET(_Params);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_VectorAttribute_PendingModifier, _ModifierName, _Params);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_VectorAttribute_RemovePendingModifier
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

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_VectorAttribute_RemovePendingModifier, _AttributeName, _ModifierName);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable)
class CKATTRIBUTE_API UCk_Fragment_VectorAttribute_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Fragment_VectorAttribute_Rep);
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_VectorAttribute_Rep);

public:
    friend class ck::FProcessor_VectorAttribute_Replicate;

public:
    UFUNCTION(Server, Reliable)
    void
    Broadcast_AddModifier(
        FGameplayTag InModifierName,
        const FCk_Fragment_VectorAttributeModifier_ParamsData& InParams);

    UFUNCTION(Server, Reliable)
    void
    Broadcast_RemoveModifier(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName);

    // TODO: 'permanent' modifiers

private:
    auto PostLink() -> void override;;

private:
    auto
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const -> void override;

private:
    UFUNCTION()
    void
    OnRep_PendingModifiers();

    UPROPERTY(ReplicatedUsing = OnRep_PendingModifiers)
    TArray<FCk_Fragment_VectorAttribute_PendingModifier> _PendingAddModifiers;
    int32 _NextPendingAddModifier = 0;

    UPROPERTY(ReplicatedUsing = OnRep_PendingModifiers)
    TArray<FCk_Fragment_VectorAttribute_RemovePendingModifier> _PendingRemoveModifiers;
    int32 _NextPendingRemoveModifier = 0;
};

// --------------------------------------------------------------------------------------------------------------------