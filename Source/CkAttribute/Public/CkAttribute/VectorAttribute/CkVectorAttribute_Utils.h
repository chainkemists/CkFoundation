#pragma once

#include <GameplayTagContainer.h>

#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Fragment.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Fragment_Data.h"
#include "CkEcsBasics/CkEcsBasics_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkVectorAttribute_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_VectorAttribute_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VectorAttribute_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_VectorAttribute);

public:
    friend class UCk_Utils_VectorAttributeModifier_UE;

private:
    class VectorAttribute_Utils_Min : public ck::TUtils_Attribute<ck::FFragment_VectorAttribute_Min> {};
    class VectorAttribute_Utils_Max : public ck::TUtils_Attribute<ck::FFragment_VectorAttribute_Max> {};
    class VectorAttribute_Utils_Current : public ck::TUtils_Attribute<ck::FFragment_VectorAttribute_Current> {};

    class RecordOfVectorAttributes_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfVectorAttributes> {};

public:
    friend class UCk_Utils_VectorAttributes_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][VectorAttribute] Add New Attribute")
    static FCk_Handle_VectorAttribute
    Add(
        UPARAM(ref) FCk_Handle& InAttributeOwnerEntity,
        const FCk_Fragment_VectorAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] Add Multiple New Attributes")
    static TArray<FCk_Handle_VectorAttribute>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InAttributeOwnerEntity,
        const FCk_Fragment_MultipleVectorAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] Has Any Attribute")
    static bool
    Has(
        const FCk_Handle& InAttributeOwnerEntity);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Attribute|Vector",
        DisplayName="[Ck][VectorAttribute] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_VectorAttribute
    DoCast(
        FCk_Handle InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Attribute|Vector",
        DisplayName="[Ck][VectorAttribute] Handle -> VectorAttribute Handle",
        meta = (CompactNodeTitle = "<AsVectorAttribute>", BlueprintAutocast))
    static FCk_Handle_VectorAttribute
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] Try Get Attribute")
    static FCk_Handle_VectorAttribute
    TryGet(
        const FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag InAttributeName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] For Each",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_VectorAttribute>
    ForEach(
        UPARAM(ref) FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_VectorAttribute)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] For Each If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_VectorAttribute>
    ForEach_If(
        UPARAM(ref) FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate);
    static auto
    ForEach_If(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_VectorAttribute)>& InFunc,
        const TFunction<bool(FCk_Handle_VectorAttribute)>& InPredicate) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] Has Component")
    static bool
    Has_Component(
        const FCk_Handle_VectorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Min);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] Get Base Value")
    static FVector
    Get_BaseValue(
        const FCk_Handle_VectorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] Get Bonus Value")
    static FVector
    Get_BonusValue(
        const FCk_Handle_VectorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] Get Final Value")
    static FVector
    Get_FinalValue(
        const FCk_Handle_VectorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName="[Ck][VectorAttribute] Request Override Base Value")
    static FCk_Handle_VectorAttribute
    Request_Override(
        UPARAM(ref) FCk_Handle_VectorAttribute& InAttribute,
        FVector InNewBaseValue,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName = "[Ck][VectorAttribute] Bind To OnValueChanged")
    static FCk_Handle_VectorAttribute
    BindTo_OnValueChanged(
        UPARAM(ref) FCk_Handle_VectorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VectorAttribute_OnValueChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Vector",
              DisplayName = "[Ck][VectorAttribute] Unbind From OnValueChanged")
    static FCk_Handle_VectorAttribute
    UnbindFrom_OnValueChanged(
        UPARAM(ref) FCk_Handle_VectorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        const FCk_Delegate_VectorAttribute_OnValueChanged& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_VectorAttributeModifier_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VectorAttributeModifier_UE);

private:
    class VectorAttributeModifier_Utils_Current : public ck::TUtils_AttributeModifier<ck::FFragment_VectorAttributeModifier_Current> {};
    class VectorAttributeModifier_Utils_Min : public ck::TUtils_AttributeModifier<ck::FFragment_VectorAttributeModifier_Min> {};
    class VectorAttributeModifier_Utils_Max : public ck::TUtils_AttributeModifier<ck::FFragment_VectorAttributeModifier_Max> {};

    class RecordOfVectorAttributeModifiers_Utils_Current : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfVectorAttributeModifiers> {};
    class RecordOfVectorAttributeModifiers_Utils_Min : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfVectorAttributeModifiers> {};
    class RecordOfVectorAttributeModifiers_Utils_Max : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfVectorAttributeModifiers> {};

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][VectorAttribute] Add Modifier")
    static FCk_Handle_VectorAttributeModifier
    Add(
        UPARAM(ref) FCk_Handle_VectorAttribute& InAttribute,
        FGameplayTag InModifierName,
        const FCk_Fragment_VectorAttributeModifier_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Vector",
              DisplayName="[Ck][VectorAttribute] Try Get Modifier")
    static FCk_Handle_VectorAttributeModifier
    TryGet(
        const FCk_Handle_VectorAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent = ECk_MinMaxCurrent::Current);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Vector",
              DisplayName="[Ck][VectorAttribute] Remove Modifier")
    static FCk_Handle_VectorAttribute
    Remove(
        UPARAM(ref) FCk_Handle_VectorAttributeModifier& InAttributeModifierEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Vector",
              DisplayName="[Ck][VectorAttribute] For Each Modifier",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_VectorAttributeModifier>
    ForEach(
        UPARAM(ref) FCk_Handle_VectorAttribute& InAttribute,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);
    static auto
    ForEach(
        FCk_Handle_VectorAttribute& InAttribute,
        const TFunction<void(FCk_Handle_VectorAttributeModifier)>& InFunc,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Vector",
              DisplayName="[Ck][VectorAttribute] For Each Modifier If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_VectorAttributeModifier>
    ForEach_If(
        UPARAM(ref) FCk_Handle_VectorAttribute& InAttribute,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);
    static auto
    ForEach_If(
        FCk_Handle_VectorAttribute& InAttribute,
        const TFunction<void(FCk_Handle_VectorAttributeModifier)>& InFunc,
        const TFunction<bool(FCk_Handle_VectorAttributeModifier)>& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
