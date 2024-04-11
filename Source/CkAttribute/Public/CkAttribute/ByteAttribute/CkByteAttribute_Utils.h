#pragma once

#include <GameplayTagContainer.h>

#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/ByteAttribute/CkByteAttribute_Fragment.h"
#include "CkAttribute/ByteAttribute/CkByteAttribute_Fragment_Data.h"
#include "CkEcsBasics/CkEcsBasics_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkByteAttribute_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_ByteAttribute_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ByteAttribute_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ByteAttribute);

public:
    friend class UCk_Utils_ByteAttributeModifier_UE;

private:
    using ByteAttribute_Utils_Min = ck::TUtils_Attribute<ck::FFragment_ByteAttribute_Min>;
    using ByteAttribute_Utils_Max = ck::TUtils_Attribute<ck::FFragment_ByteAttribute_Max>;
    using ByteAttribute_Utils_Current = ck::TUtils_Attribute<ck::FFragment_ByteAttribute_Current>;

    using RecordOfByteAttributes_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfByteAttributes>;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][ByteAttribute] Add New Attribute")
    static FCk_Handle_ByteAttribute
    Add(
        UPARAM(ref) FCk_Handle& InAttributeOwnerEntity,
        const FCk_Fragment_ByteAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Add Multiple New Attributes")
    static TArray<FCk_Handle_ByteAttribute>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InAttributeOwnerEntity,
        const FCk_Fragment_MultipleByteAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InAttributeOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Has Any Attribute")
    static bool
    Has_Any(
        const FCk_Handle& InAttributeOwnerEntity);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Attribute|Byte",
        DisplayName="[Ck][ByteAttribute] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ByteAttribute
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Attribute|Byte",
        DisplayName="[Ck][ByteAttribute] Handle -> ByteAttribute Handle",
        meta = (CompactNodeTitle = "<AsByteAttribute>", BlueprintAutocast))
    static FCk_Handle_ByteAttribute
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Try Get Attribute")
    static FCk_Handle_ByteAttribute
    TryGet(
        const FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] For Each",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_ByteAttribute>
    ForEach(
        UPARAM(ref) FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_ByteAttribute)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] For Each If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_ByteAttribute>
    ForEach_If(
        UPARAM(ref) FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate);
    static auto
    ForEach_If(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_ByteAttribute)>& InFunc,
        const TFunction<bool(FCk_Handle_ByteAttribute)>& InPredicate) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Has Component")
    static bool
    Has_Component(
        const FCk_Handle_ByteAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Min);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Get Base Value")
    static uint8
    Get_BaseValue(
        const FCk_Handle_ByteAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Get Bonus Value")
    static uint8
    Get_BonusValue(
        const FCk_Handle_ByteAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Get Final Value")
    static uint8
    Get_FinalValue(
        const FCk_Handle_ByteAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName="[Ck][ByteAttribute] Request Override Base Value")
    static FCk_Handle_ByteAttribute
    Request_Override(
        UPARAM(ref) FCk_Handle_ByteAttribute& InAttribute,
        uint8 InNewBaseValue,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName = "[Ck][ByteAttribute] Bind To OnValueChanged")
    static FCk_Handle_ByteAttribute
    BindTo_OnValueChanged(
        UPARAM(ref) FCk_Handle_ByteAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ByteAttribute_OnValueChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Byte",
              DisplayName = "[Ck][ByteAttribute] Unbind From OnValueChanged")
    static FCk_Handle_ByteAttribute
    UnbindFrom_OnValueChanged(
        UPARAM(ref) FCk_Handle_ByteAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        const FCk_Delegate_ByteAttribute_OnValueChanged& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_ByteAttributeModifier_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ByteAttributeModifier_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ByteAttributeModifier);

private:
    using ByteAttributeModifier_Utils_Current = ck::TUtils_AttributeModifier<ck::FFragment_ByteAttributeModifier_Current>;
    using ByteAttributeModifier_Utils_Min     = ck::TUtils_AttributeModifier<ck::FFragment_ByteAttributeModifier_Min>;
    using ByteAttributeModifier_Utils_Max     = ck::TUtils_AttributeModifier<ck::FFragment_ByteAttributeModifier_Max>;

    using RecordOfByteAttributeModifiers_Utils_Current = ByteAttributeModifier_Utils_Current::RecordOfAttributeModifiers_Utils;
    using RecordOfByteAttributeModifiers_Utils_Min     = ByteAttributeModifier_Utils_Min::RecordOfAttributeModifiers_Utils;
    using RecordOfByteAttributeModifiers_Utils_Max     = ByteAttributeModifier_Utils_Max::RecordOfAttributeModifiers_Utils;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][ByteAttribute] Add Modifier")
    static FCk_Handle_ByteAttributeModifier
    Add(
        UPARAM(ref) FCk_Handle_ByteAttribute& InAttribute,
        FGameplayTag InModifierName,
        const FCk_Fragment_ByteAttributeModifier_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Byte",
              DisplayName="[Ck][ByteAttribute] Try Get Modifier")
    static FCk_Handle_ByteAttributeModifier
    TryGet(
        const FCk_Handle_ByteAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent = ECk_MinMaxCurrent::Current);
    static FCk_Handle_ByteAttributeModifier
    TryGet_If(
        const FCk_Handle_ByteAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent,
        const TFunction<bool(FCk_Handle_ByteAttributeModifier)>& InPredicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Byte",
              DisplayName="[Ck][ByteAttribute] Remove Modifier")
    static FCk_Handle_ByteAttribute
    Remove(
        UPARAM(ref) FCk_Handle_ByteAttributeModifier& InAttributeModifierEntity);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InModifierEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Byte",
              DisplayName="[Ck][ByteAttribute] For Each Modifier",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_ByteAttributeModifier>
    ForEach(
        UPARAM(ref) FCk_Handle_ByteAttribute& InAttribute,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);
    static auto
    ForEach(
        FCk_Handle_ByteAttribute& InAttribute,
        const TFunction<void(FCk_Handle_ByteAttributeModifier)>& InFunc,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Byte",
              DisplayName="[Ck][ByteAttribute] For Each Modifier If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_ByteAttributeModifier>
    ForEach_If(
        UPARAM(ref) FCk_Handle_ByteAttribute& InAttribute,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);
    static auto
    ForEach_If(
        FCk_Handle_ByteAttribute& InAttribute,
        const TFunction<void(FCk_Handle_ByteAttributeModifier)>& InFunc,
        const TFunction<bool(FCk_Handle_ByteAttributeModifier)>& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
