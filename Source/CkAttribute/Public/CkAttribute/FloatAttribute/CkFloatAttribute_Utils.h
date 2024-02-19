#pragma once

#include <GameplayTagContainer.h>

#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"
#include "CkEcsBasics/CkEcsBasics_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkFloatAttribute_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_FloatAttribute_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_FloatAttribute_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_FloatAttribute);

public:
    friend class UCk_Utils_FloatAttributeModifier_UE;

private:
    class FloatAttribute_Utils_Min : public ck::TUtils_Attribute<ck::FFragment_FloatAttribute_Min> {};
    class FloatAttribute_Utils_Max : public ck::TUtils_Attribute<ck::FFragment_FloatAttribute_Max> {};
    class FloatAttribute_Utils_Current : public ck::TUtils_Attribute<ck::FFragment_FloatAttribute_Current> {};

    class RecordOfFloatAttributes_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfFloatAttributes> {};

public:
    friend class UCk_Utils_FloatAttributes_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][FloatAttribute] Add New Attribute")
    static FCk_Handle_FloatAttribute
    Add(
        UPARAM(ref) FCk_Handle& InAttributeOwnerEntity,
        const FCk_Fragment_FloatAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Add Multiple New Attributes")
    static TArray<FCk_Handle_FloatAttribute>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InAttributeOwnerEntity,
        const FCk_Fragment_MultipleFloatAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InAttributeOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Has Any Attribute")
    static bool
    Has_Any(
        const FCk_Handle& InAttributeOwnerEntity);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Attribute|Float",
        DisplayName="[Ck][FloatAttribute] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_FloatAttribute
    DoCast(
        FCk_Handle InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Attribute|Float",
        DisplayName="[Ck][FloatAttribute] Handle -> FloatAttribute Handle",
        meta = (CompactNodeTitle = "<AsFloatAttribute>", BlueprintAutocast))
    static FCk_Handle_FloatAttribute
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Try Get Attribute")
    static FCk_Handle_FloatAttribute
    TryGet(
        const FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] For Each",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_FloatAttribute>
    ForEach(
        UPARAM(ref) FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_FloatAttribute)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] For Each If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_FloatAttribute>
    ForEach_If(
        UPARAM(ref) FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate);
    static auto
    ForEach_If(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_FloatAttribute)>& InFunc,
        const TFunction<bool(FCk_Handle_FloatAttribute)>& InPredicate) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Has Component")
    static bool
    Has_Component(
        const FCk_Handle_FloatAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Min);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Get Base Value")
    static float
    Get_BaseValue(
        const FCk_Handle_FloatAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Get Bonus Value")
    static float
    Get_BonusValue(
        const FCk_Handle_FloatAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Get Final Value")
    static float
    Get_FinalValue(
        const FCk_Handle_FloatAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Request Override Base Value")
    static FCk_Handle_FloatAttribute
    Request_Override(
        UPARAM(ref) FCk_Handle_FloatAttribute& InAttribute,
        float InNewBaseValue,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName = "[Ck][FloatAttribute] Bind To OnValueChanged")
    static FCk_Handle_FloatAttribute
    BindTo_OnValueChanged(
        UPARAM(ref) FCk_Handle_FloatAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_FloatAttribute_OnValueChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName = "[Ck][FloatAttribute] Unbind From OnValueChanged")
    static FCk_Handle_FloatAttribute
    UnbindFrom_OnValueChanged(
        UPARAM(ref) FCk_Handle_FloatAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        const FCk_Delegate_FloatAttribute_OnValueChanged& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_FloatAttributeModifier_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_FloatAttributeModifier_UE);

private:
    class FloatAttributeModifier_Utils_Current : public ck::TUtils_AttributeModifier<ck::FFragment_FloatAttributeModifier_Current> {};
    class FloatAttributeModifier_Utils_Min : public ck::TUtils_AttributeModifier<ck::FFragment_FloatAttributeModifier_Min> {};
    class FloatAttributeModifier_Utils_Max : public ck::TUtils_AttributeModifier<ck::FFragment_FloatAttributeModifier_Max> {};

    class RecordOfFloatAttributeModifiers_Utils_Current : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfFloatAttributeModifiers> {};
    class RecordOfFloatAttributeModifiers_Utils_Min : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfFloatAttributeModifiers> {};
    class RecordOfFloatAttributeModifiers_Utils_Max : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfFloatAttributeModifiers> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][FloatAttribute] Add Modifier")
    static FCk_Handle_FloatAttributeModifier
    Add(
        UPARAM(ref) FCk_Handle_FloatAttribute& InAttribute,
        FGameplayTag InModifierName,
        const FCk_Fragment_FloatAttributeModifier_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Float",
              DisplayName="[Ck][FloatAttribute] Try Get Modifier")
    static FCk_Handle_FloatAttributeModifier
    TryGet(
        const FCk_Handle_FloatAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent = ECk_MinMaxCurrent::Current);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Float",
              DisplayName="[Ck][FloatAttribute] Remove Modifier")
    static FCk_Handle_FloatAttribute
    Remove(
        UPARAM(ref) FCk_Handle_FloatAttributeModifier& InAttributeModifierEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Float",
              DisplayName="[Ck][FloatAttribute] For Each Modifier",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_FloatAttributeModifier>
    ForEach(
        UPARAM(ref) FCk_Handle_FloatAttribute& InAttribute,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);
    static auto
    ForEach(
        FCk_Handle_FloatAttribute& InAttribute,
        const TFunction<void(FCk_Handle_FloatAttributeModifier)>& InFunc,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Float",
              DisplayName="[Ck][FloatAttribute] For Each Modifier If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_FloatAttributeModifier>
    ForEach_If(
        UPARAM(ref) FCk_Handle_FloatAttribute& InAttribute,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);
    static auto
    ForEach_If(
        FCk_Handle_FloatAttribute& InAttribute,
        const TFunction<void(FCk_Handle_FloatAttributeModifier)>& InFunc,
        const TFunction<bool(FCk_Handle_FloatAttributeModifier)>& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
