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
                InPayload.Get_FinalValue(),
                InPayload.Get_BaseValue_Previous(),
                InPayload.Get_FinalValue_Previous()
            };
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <>
    struct TTypeConverter<TPayload_Attribute_OnClamped<FFragment_VectorAttribute_Current>, TypeConverterPolicy::TypeToUnreal>
    {
        auto operator()(const TPayload_Attribute_OnClamped<FFragment_VectorAttribute_Current>& InPayload) const
        {
            return FCk_Payload_VectorAttribute_OnClamped
            {
                InPayload.Get_Handle(),
                InPayload.Get_ClampedFinalValue()
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
                InPayload.Get_FinalValue(),
                InPayload.Get_BaseValue_Previous(),
                InPayload.Get_FinalValue_Previous()
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
                InPayload.Get_FinalValue(),
                InPayload.Get_BaseValue_Previous(),
                InPayload.Get_FinalValue_Previous()
            };
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Value Changed
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

    // Clamped
    using UUtils_Signal_OnVectorAttributeMinClamped = TUtils_Signal_OnAttributeClamped<
        FFragment_VectorAttribute_Current, FFragment_VectorAttribute_Min, FCk_Delegate_VectorAttribute_OnClamped_MC>;

    using UUtils_Signal_OnVectorAttributeMinClamped_PostFireUnbind = TUtils_Signal_OnAttributeClamped_PostFireUnbind<
        FFragment_VectorAttribute_Current, FFragment_VectorAttribute_Min, FCk_Delegate_VectorAttribute_OnClamped_MC>;

    using UUtils_Signal_OnVectorAttributeMaxClamped = TUtils_Signal_OnAttributeClamped<
        FFragment_VectorAttribute_Current, FFragment_VectorAttribute_Max, FCk_Delegate_VectorAttribute_OnClamped_MC>;

    using UUtils_Signal_OnVectorAttributeMaxClamped_PostFireUnbind = TUtils_Signal_OnAttributeClamped_PostFireUnbind<
        FFragment_VectorAttribute_Current, FFragment_VectorAttribute_Max, FCk_Delegate_VectorAttribute_OnClamped_MC>;

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_VectorAttribute_OverrideModifier
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Fragment_VectorAttribute_OverrideModifier);

private:
    UPROPERTY()
    FGameplayTag _AttributeName;

    UPROPERTY()
    FGameplayTag _ModifierName;

    UPROPERTY()
    FVector _NewDelta = FVector::ZeroVector;

    UPROPERTY()
    ECk_MinMaxCurrent _Component = ECk_MinMaxCurrent::Current;

public:
    CK_PROPERTY_GET(_AttributeName);
    CK_PROPERTY_GET(_ModifierName);
    CK_PROPERTY_GET(_NewDelta);
    CK_PROPERTY_GET(_Component);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_VectorAttribute_OverrideModifier, _AttributeName, _ModifierName, _NewDelta, _Component);
};

// --------------------------------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_VectorAttribute_PendingModifier
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Fragment_VectorAttribute_PendingModifier);

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

    CK_GENERATED_BODY(FCk_Fragment_VectorAttribute_RemovePendingModifier);

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

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_VectorAttribute_RemovePendingModifier, _AttributeName, _ModifierName, _Component);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Fragment_VectorAttribute_BaseFinal
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Fragment_VectorAttribute_BaseFinal);

private:
    UPROPERTY()
    FGameplayTag _AttributeName;

    UPROPERTY()
    FVector _Base = FVector::ZeroVector;

    UPROPERTY()
    FVector _Final = FVector::ZeroVector;

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

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_VectorAttribute_BaseFinal, _AttributeName, _Base, _Final, _Component);
};

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_Fragment_VectorAttribute_BaseFinal, [](const FCk_Fragment_VectorAttribute_BaseFinal& InObj)
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

UCLASS(Blueprintable)
class CKATTRIBUTE_API UCk_Fragment_VectorAttribute_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Fragment_VectorAttribute_Rep);
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_VectorAttribute_Rep);

public:
    auto
    Broadcast_AddOrUpdate(
        FGameplayTag InAttributeName,
        const FVector& InBase,
        const FVector& InFinal,
        ECk_MinMaxCurrent InComponent) -> void;

private:
    auto
    PostLink() -> void override;;

private:
    auto
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const -> void override;

public:
    auto
    Request_TryUpdateReplicatedAttributes() -> void;

private:
    UFUNCTION()
    void
    OnRep_Updated();

private:
    UPROPERTY(ReplicatedUsing = OnRep_Updated);
    TArray<FCk_Fragment_VectorAttribute_BaseFinal> _AttributesToReplicate;
    TArray<FCk_Fragment_VectorAttribute_BaseFinal> _AttributesToReplicate_Previous;
};

// --------------------------------------------------------------------------------------------------------------------