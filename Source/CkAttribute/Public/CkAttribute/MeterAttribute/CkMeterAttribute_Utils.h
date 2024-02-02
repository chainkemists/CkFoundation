#pragma once

#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment.h"
#include "CkAttribute/MeterAttribute/CkMeterAttribute_Fragment.h"
#include "CkAttribute/MeterAttribute/CkMeterAttribute_Fragment_Data.h"

#include "CkCore/Meter/CkMeter.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkEcsBasics/CkEcsBasics_Utils.h"

#include "CkSignal/CkSignal_Fragment_Data.h"

#include <GameplayTagContainer.h>
#include <InstancedStruct.h>

#include "CkMeterAttribute_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
    struct FMeterAttribute_Tags final : public FGameplayTagNativeAdder
    {
    protected:
        auto AddTags() -> void override;

    private:
        FGameplayTag _MinCapacity;
        FGameplayTag _MaxCapacity;
        FGameplayTag _Current;

        static FMeterAttribute_Tags _Tags;

    public:
        static auto Get_MinCapacity() -> FGameplayTag
        {
            return _Tags._MinCapacity;
        }

        static auto Get_MaxCapacity() -> FGameplayTag
        {
            return _Tags._MaxCapacity;
        }

        static auto Get_Current() -> FGameplayTag
        {
            return _Tags._Current;
        }
    };
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintType, NotBlueprintable)
class CKATTRIBUTE_API UCk_MeterAttribute_ConstructionScript_PDA final : public UCk_Entity_ConstructionScript_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_MeterAttribute_ConstructionScript_PDA);

    auto DoConstruct_Implementation(
        const FCk_Handle& InHandle,
        const FInstancedStruct& InOptionalParams) -> void override;

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess))
    FCk_Fragment_MeterAttribute_ParamsData _Params;

public:
    CK_PROPERTY(_Params);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_MeterAttribute_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_MeterAttribute_UE);

public:
    friend class UCk_Utils_MeterAttributeModifier_UE;

public:
    class RecordOfMeterAttributes_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfMeterAttributes> {};

    class FloatAttribute_Utils : public ck::TUtils_Attribute<ck::FFragment_FloatAttribute> {};
    class RecordOfFloatAttributes_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfFloatAttributes> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="[Ck][MeterAttribute] Add New Attribute")
    static void
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_MeterAttribute_ParamsData& InConstructionScriptData,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="[Ck][MeterAttribute] Add Multiple New Attributes")
    static void
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleMeterAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="[Ck][MeterAttribute] Has Attribute")
    static bool
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="[Ck][MeterAttribute] Has Any Attribute")
    static bool
    Has_Any(
        FCk_Handle InAttributeOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="[Ck][MeterAttribute] Ensure Has Attribute")
    static bool
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

        UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="[Ck][MeterAttribute] Ensure Has Any Attribute")
    static bool
    Ensure_Any(
        FCk_Handle InAttributeOwnerEntity);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="[Ck][MeterAttribute] Get All Attributes",
              meta=(DeprecatedFunction, DeprecationMessage="Use the ForEach variants"))
    static TArray<FGameplayTag>
    Get_All(
        FCk_Handle InAttributeOwnerEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="[Ck][MeterAttribute] For Each",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle>
    ForEach(
        FCk_Handle InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach(
        const FCk_Handle&                  InAttributeOwner,
        const TFunction<void(FCk_Handle)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="[Ck][MeterAttribute] For Each If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle>
    ForEach_If(
        FCk_Handle InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate);
    static auto
    ForEach_If(
        const FCk_Handle&                  InAttributeOwner,
        const TFunction<void(FCk_Handle)>& InFunc,
        const TFunction<bool(FCk_Handle)>& InPredicate) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="[Ck][MeterAttribute] Get Base Value")
    static FCk_Meter
    Get_BaseValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="[Ck][MeterAttribute] Get Bonus Value")
    static FCk_Meter
    Get_BonusValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="[Ck][MeterAttribute] Get Final Value")
    static FCk_Meter
    Get_FinalValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName="[Ck][MeterAttribute] Request Override Base Value")
    static void
    Request_Override(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FCk_Meter InNewBaseValue,
        FCk_Meter_Mask InMask);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName = "[Ck][MeterAttribute] Bind To OnValueChanged")
    static void
    BindTo_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_MeterAttribute_OnValueChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Meter",
              DisplayName = "[Ck][MeterAttribute] Unbind From OnValueChanged")
    static void
    UnbindFrom_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        const FCk_Delegate_MeterAttribute_OnValueChanged& InDelegate);

public:
    static auto
    Get_MinAndCurrentAttributeEntities(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> std::tuple<FCk_Handle, FCk_Handle>;

    static auto
    Get_MaxAndCurrentAttributeEntities(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> std::tuple<FCk_Handle, FCk_Handle>;

    static auto
    Get_MinMaxAndCurrentAttributeEntities(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> std::tuple<FCk_Handle, FCk_Handle, FCk_Handle>;

private:
    static auto
    OnMinCapacityChanged(
        FCk_Handle InFloatAttributeEntity,
        ck::TPayload_Attribute_OnValueChanged<ck::FFragment_FloatAttribute> InValueChanged) -> void;

    static auto
    OnMaxCapacityChanged(
        FCk_Handle InFloatAttributeEntity,
        ck::TPayload_Attribute_OnValueChanged<ck::FFragment_FloatAttribute> InValueChanged) -> void;

    static auto
    OnCurrentValueCanged(
        FCk_Handle InFloatAttributeEntity,
        ck::TPayload_Attribute_OnValueChanged<ck::FFragment_FloatAttribute> InValueChanged) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_MeterAttributeModifier_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_MeterAttributeModifier_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Meter",
              DisplayName="[Ck][MeterAttribute] Add Modifier")
    static void
    Add(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_MeterAttributeModifier_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Meter",
              DisplayName="[Ck][MeterAttribute] Has Modifier")
    static bool
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Meter",
              DisplayName="[Ck][MeterAttribute] Ensure Has Modifier")
    static bool
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Meter",
              DisplayName="[Ck][MeterAttribute] Remove Modifier")
    static void
    Remove(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName);
};

// --------------------------------------------------------------------------------------------------------------------
