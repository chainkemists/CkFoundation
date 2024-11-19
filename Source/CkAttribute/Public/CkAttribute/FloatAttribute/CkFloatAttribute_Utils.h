#pragma once

#include <GameplayTagContainer.h>

#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"
#include "CkEcsExt/CkEcsExt_Utils.h"
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
    using FloatAttribute_Utils_Min = ck::TUtils_Attribute<ck::FFragment_FloatAttribute_Min>;
    using FloatAttribute_Utils_Max = ck::TUtils_Attribute<ck::FFragment_FloatAttribute_Max>;
    using FloatAttribute_Utils_Current = ck::TUtils_Attribute<ck::FFragment_FloatAttribute_Current>;

    using RecordOfFloatAttributes_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfFloatAttributes>;

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
        UPARAM(ref) FCk_Handle& InHandle,
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
        UPARAM(meta = (Categories = "FloatAttribute")) FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Try Get Entity With Attribute In Ownership Chain")
    static FCk_Handle
    TryGet_Entity_WithAttribute_InOwnershipChain(
        const FCk_Handle& InHandle,
        UPARAM(meta = (Categories = "FloatAttribute")) FGameplayTag InAttributeName);

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
              DisplayName="[Ck][FloatAttribute] Has Refill Attribute")
    static bool
    Has_RefillAttribute(
        const FCk_Handle_FloatAttribute& InAttribute);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Float",
              DisplayName="[Ck][FloatAttribute] Try Get Refill Attribute")
    static FCk_Handle_FloatAttributeRefill
    TryGet_RefillAttribute(
        const FCk_Handle_FloatAttribute& InAttribute);

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
class CKATTRIBUTE_API UCk_Utils_FloatAttributeRefill_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_FloatAttributeRefill_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_FloatAttributeRefill);

public:
    friend class UCk_Utils_FloatAttribute_UE;

private:
    static auto
    Add(
        FCk_Handle_FloatAttribute& InAttributeRefillEntity,
        ECk_Attribute_RefillState InStartingState) -> FCk_Handle_FloatAttributeRefill;

    static auto
    Has(
        const FCk_Handle& InAttributeOwnerEntity) -> bool;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeRefill|Float",
              DisplayName="[Ck][FloatAttribute] Get Fill Rate")
    static float
    Get_FillRate(
        const FCk_Handle_FloatAttributeRefill& InAttributeRefill);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeRefill|Float",
              DisplayName="[Ck][FloatAttribute] Get Refill State")
    static ECk_Attribute_RefillState
    Get_RefillState(
        const FCk_Handle_FloatAttributeRefill& InAttributeRefill);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeRefill|Float",
              DisplayName="[Ck][FloatAttribute] Request Pause Refill")
    static FCk_Handle_FloatAttributeRefill
    Request_Pause(
        UPARAM(ref) FCk_Handle_FloatAttributeRefill& InAttributeRefill);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeRefill|Float",
              DisplayName="[Ck][FloatAttribute] Request Resume Refill")
    static FCk_Handle_FloatAttributeRefill
    Request_Resume(
        UPARAM(ref) FCk_Handle_FloatAttributeRefill& InAttributeRefill);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Utils_FloatAttributeModifier_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_FloatAttributeModifier_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_FloatAttributeModifier);

private:
    using FloatAttributeModifier_Utils_Current = ck::TUtils_AttributeModifier<ck::FFragment_FloatAttributeModifier_Current>;
    using FloatAttributeModifier_Utils_Min     = ck::TUtils_AttributeModifier<ck::FFragment_FloatAttributeModifier_Min>;
    using FloatAttributeModifier_Utils_Max     = ck::TUtils_AttributeModifier<ck::FFragment_FloatAttributeModifier_Max>;

    using RecordOfFloatAttributeModifiers_Utils_Current = FloatAttributeModifier_Utils_Current::RecordOfAttributeModifiers_Utils;
    using RecordOfFloatAttributeModifiers_Utils_Min     = FloatAttributeModifier_Utils_Min::RecordOfAttributeModifiers_Utils;
    using RecordOfFloatAttributeModifiers_Utils_Max     = FloatAttributeModifier_Utils_Max::RecordOfAttributeModifiers_Utils;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][FloatAttribute] Add Modifier (Revocable)")
    static FCk_Handle_FloatAttributeModifier
    Add_Revocable(
        UPARAM(ref) FCk_Handle_FloatAttribute& InAttribute,
        FGameplayTag InModifierName,
        UPARAM(meta = (InvalidEnumValues="Override")) ECk_AttributeModifier_Operation InModifierOperation,
        const FCk_Fragment_FloatAttributeModifier_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][FloatAttribute] Add Modifier (Not Revocable)")
    static void
    Add_NotRevocable(
        UPARAM(ref) FCk_Handle_FloatAttribute& InAttribute,
        ECk_AttributeModifier_Operation InModifierOperation,
        const FCk_Fragment_FloatAttributeModifier_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Float",
              DisplayName="[Ck][FloatAttribute] Override Modifier")
    static FCk_Handle_FloatAttributeModifier
    Override(
        UPARAM(ref) FCk_Handle_FloatAttributeModifier& InAttributeModifierEntity,
        float InNewDelta);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Float",
              DisplayName="[Ck][FloatAttribute] Get Modifier Delta")
    static float
    Get_Delta(
        const FCk_Handle_FloatAttributeModifier& InAttributeModifierEntity,
        ECk_MinMaxCurrent InComponent = ECk_MinMaxCurrent::Current);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Float",
              DisplayName="[Ck][FloatAttribute] Try Get Modifier")
    static FCk_Handle_FloatAttributeModifier
    TryGet(
        const FCk_Handle_FloatAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent = ECk_MinMaxCurrent::Current);
    static FCk_Handle_FloatAttributeModifier
    TryGet_If(
        const FCk_Handle_FloatAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent,
        const TFunction<bool(FCk_Handle_FloatAttributeModifier)>& InPredicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Float",
              DisplayName="[Ck][FloatAttribute] Remove Modifier")
    static FCk_Handle_FloatAttribute
    Remove(
        UPARAM(ref) FCk_Handle_FloatAttributeModifier& InAttributeModifierEntity);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InModifierEntity);

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
