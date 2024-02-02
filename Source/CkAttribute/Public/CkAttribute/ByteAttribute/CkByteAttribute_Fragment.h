#pragma once

#include "CkAttribute/CkAttribute_Fragment.h"
#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/ByteAttribute/CkByteAttribute_Fragment_Data.h"

#include "CkCore/TypeConverter/CkTypeConverter.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkByteAttribute_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKATTRIBUTE_API FFragment_ByteAttribute : public TFragment_Attribute<uint8>
    {
        using TFragment_Attribute::TFragment_Attribute;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKATTRIBUTE_API FFragment_ByteAttributeModifier : public TFragment_AttributeModifier<FFragment_ByteAttribute>
    {
        using TFragment_AttributeModifier::TFragment_AttributeModifier;
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfByteAttributes, FCk_Handle);

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TAttributeModifierOperators<uint8>
    {
        static auto Add(uint8 InA, uint8 InB) -> uint8
        {
            return InA + InB;
        };

        static auto Multiply(uint8 InA, uint8 InB) -> uint8
        {
            return InA * InB;
        };
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TTypeConverter<TPayload_Attribute_OnValueChanged<FFragment_ByteAttribute>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnValueChanged<FFragment_ByteAttribute>& InPayload) const
        {
            return FCk_Payload_ByteAttribute_OnValueChanged
            {
                InPayload.Get_Handle(),
                InPayload.Get_BaseValue(),
                InPayload.Get_FinalValue()
            };
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    using UUtils_Signal_OnByteAttributeValueChanged = TUtils_Signal_OnAttributeValueChanged<
        FFragment_ByteAttribute, FCk_Delegate_ByteAttribute_OnValueChanged_MC>;

    using UUtils_Signal_OnByteAttributeValueChanged_PostFireUnbind = TUtils_Signal_OnAttributeValueChanged_PostFireUnbind<
        FFragment_ByteAttribute, FCk_Delegate_ByteAttribute_OnValueChanged_MC>;

    // --------------------------------------------------------------------------------------------------------------------

    class FProcessor_ByteAttribute_Replicate;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_ByteAttribute_PendingModifier
{
    GENERATED_BODY()

private:
    UPROPERTY()
    FGameplayTag _ModifierName;

    UPROPERTY()
    FCk_Fragment_ByteAttributeModifier_ParamsData _Params;

public:
    CK_PROPERTY_GET(_ModifierName);
    CK_PROPERTY_GET(_Params);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_ByteAttribute_PendingModifier, _ModifierName, _Params);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_ByteAttribute_RemovePendingModifier
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

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_ByteAttribute_RemovePendingModifier, _AttributeName, _ModifierName);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable)
class CKATTRIBUTE_API UCk_Fragment_ByteAttribute_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Fragment_ByteAttribute_Rep);
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_ByteAttribute_Rep);

public:
    friend class ck::FProcessor_ByteAttribute_Replicate;

public:
    UFUNCTION(Server, Reliable)
    void
    Broadcast_AddModifier(
        FGameplayTag InModifierName,
        const FCk_Fragment_ByteAttributeModifier_ParamsData& InParams);

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
    TArray<FCk_Fragment_ByteAttribute_PendingModifier> _PendingAddModifiers;
    int32 _NextPendingAddModifier = 0;

    UPROPERTY(ReplicatedUsing = OnRep_PendingModifiers)
    TArray<FCk_Fragment_ByteAttribute_RemovePendingModifier> _PendingRemoveModifiers;
    int32 _NextPendingRemoveModifier = 0;
};

// --------------------------------------------------------------------------------------------------------------------