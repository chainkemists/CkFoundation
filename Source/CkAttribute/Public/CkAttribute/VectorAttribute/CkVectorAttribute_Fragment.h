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
    struct CKATTRIBUTE_API FFragment_VectorAttribute : public TFragment_Attribute<FVector>
    {
        using TFragment_Attribute::TFragment_Attribute;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKATTRIBUTE_API FFragment_VectorAttributeModifier : public TFragment_AttributeModifier<FFragment_VectorAttribute>
    {
        using TFragment_AttributeModifier::TFragment_AttributeModifier;
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfVectorAttributes, FCk_Handle_VectorAttribute);

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TAttributeModifierOperators<FVector>
    {
        static auto Add(FVector InA, FVector InB) -> FVector
        {
            return InA + InB;
        };

        static auto Multiply(FVector InA, FVector InB) -> FVector
        {
            return InA * InB;
        };
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TTypeConverter<TPayload_Attribute_OnValueChanged<FFragment_VectorAttribute>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnValueChanged<FFragment_VectorAttribute>& InPayload) const
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

    using UUtils_Signal_OnVectorAttributeValueChanged = TUtils_Signal_OnAttributeValueChanged<
        FFragment_VectorAttribute, FCk_Delegate_VectorAttribute_OnValueChanged_MC>;

    using UUtils_Signal_OnVectorAttributeValueChanged_PostFireUnbind = TUtils_Signal_OnAttributeValueChanged_PostFireUnbind<
        FFragment_VectorAttribute, FCk_Delegate_VectorAttribute_OnValueChanged_MC>;

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

public:
    CK_PROPERTY_GET(_AttributeName);
    CK_PROPERTY_GET(_ModifierName);

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

private:
    UPROPERTY(ReplicatedUsing = OnRep_PendingModifiers)
    TArray<FCk_Fragment_VectorAttribute_PendingModifier> _PendingAddModifiers;
    int32 _NextPendingAddModifier = 0;

    UPROPERTY(ReplicatedUsing = OnRep_PendingModifiers)
    TArray<FCk_Fragment_VectorAttribute_RemovePendingModifier> _PendingRemoveModifiers;
    int32 _NextPendingRemoveModifier = 0;
};

// --------------------------------------------------------------------------------------------------------------------