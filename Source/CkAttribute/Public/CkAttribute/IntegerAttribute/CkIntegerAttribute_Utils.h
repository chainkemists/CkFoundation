#pragma once

#include <GameplayTagContainer.h>

#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Fragment.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Fragment_Data.h"
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkIntegerAttribute_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_IntegerAttribute"))
class CKATTRIBUTE_API UCk_Utils_IntegerAttribute_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IntegerAttribute_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_IntegerAttribute);

public:
    friend class UCk_Utils_IntegerAttributeModifier_UE;

private:
    using IntegerAttribute_Utils_Min = ck::TUtils_Attribute<ck::FFragment_IntegerAttribute_Min>;
    using IntegerAttribute_Utils_Max = ck::TUtils_Attribute<ck::FFragment_IntegerAttribute_Max>;
    using IntegerAttribute_Utils_Current = ck::TUtils_Attribute<ck::FFragment_IntegerAttribute_Current>;

    using RecordOfIntegerAttributes_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfIntegerAttributes>;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][IntegerAttribute] Add New Attribute")
    static FCk_Handle_IntegerAttribute
    Add(
        UPARAM(ref) FCk_Handle& InAttributeOwnerEntity,
        const FCk_Fragment_IntegerAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName="[Ck][IntegerAttribute] Add Multiple New Attributes")
    static TArray<FCk_Handle_IntegerAttribute>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InAttributeOwnerEntity,
        const FCk_Fragment_MultipleIntegerAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InAttributeOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName="[Ck][IntegerAttribute] Has Any Attribute")
    static bool
    Has_Any(
        const FCk_Handle& InAttributeOwnerEntity);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Attribute|Integer",
        DisplayName="[Ck][IntegerAttribute] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_IntegerAttribute
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Attribute|Integer",
        DisplayName="[Ck][IntegerAttribute] Handle -> IntegerAttribute Handle",
        meta = (CompactNodeTitle = "<AsIntegerAttribute>", BlueprintAutocast))
    static FCk_Handle_IntegerAttribute
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid IntegerAttribute Handle",
        Category = "Ck|Utils|IntegerAttribute",
        meta = (CompactNodeTitle = "INVALID_IntegerAttributeHandle", Keywords = "make"))
    static FCk_Handle_IntegerAttribute
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName="[Ck][IntegerAttribute] Try Get Attribute")
    static FCk_Handle_IntegerAttribute
    TryGet(
        const FCk_Handle& InAttributeOwnerEntity,
        UPARAM(meta = (Categories = "IntegerAttribute")) FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName="[Ck][IntegerAttribute] Try Get Entity With Attribute In Ownership Chain")
    static FCk_Handle
    TryGet_Entity_WithAttribute_InOwnershipChain(
        const FCk_Handle& InHandle,
        UPARAM(meta = (Categories = "IntegerAttribute")) FGameplayTag InAttributeName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName="[Ck][IntegerAttribute] For Each",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_IntegerAttribute>
    ForEach(
        UPARAM(ref) FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_IntegerAttribute)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName="[Ck][IntegerAttribute] For Each If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_IntegerAttribute>
    ForEach_If(
        UPARAM(ref) FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate);
    static auto
    ForEach_If(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_IntegerAttribute)>& InFunc,
        const TFunction<bool(FCk_Handle_IntegerAttribute)>& InPredicate) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName="[Ck][IntegerAttribute] For Each (By Name)",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_IntegerAttribute>
    ForEach_ByName(
        UPARAM(ref) FCk_Handle& InAttributeOwner,
        UPARAM(meta = (Categories = "IntegerAttribute")) FGameplayTag InAttributeName,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_ByName(
        FCk_Handle& InAttributeOwner,
        FGameplayTag InAttributeName,
        const TFunction<void(FCk_Handle_IntegerAttribute)>& InFunc) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName="[Ck][IntegerAttribute] Has Refill Attribute")
    static bool
    Has_RefillAttribute(
        const FCk_Handle_IntegerAttribute& InAttribute);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName="[Ck][IntegerAttribute] Try Get Refill Attribute")
    static FCk_Handle_IntegerAttributeRefill
    TryGet_RefillAttribute(
        const FCk_Handle_IntegerAttribute& InAttribute);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName="[Ck][IntegerAttribute] Has Component")
    static bool
    Has_Component(
        const FCk_Handle_IntegerAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Min);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName="[Ck][IntegerAttribute] Get Base Value")
    static int32
    Get_BaseValue(
        const FCk_Handle_IntegerAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName="[Ck][IntegerAttribute] Get Bonus Value")
    static int32
    Get_BonusValue(
        const FCk_Handle_IntegerAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName="[Ck][IntegerAttribute] Get Final Value")
    static int32
    Get_FinalValue(
        const FCk_Handle_IntegerAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName="[Ck][IntegerAttribute] Request Override Base Value")
    static FCk_Handle_IntegerAttribute
    Request_Override(
        UPARAM(ref) FCk_Handle_IntegerAttribute& InAttribute,
        int32 InNewBaseValue,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName = "[Ck][IntegerAttribute] Bind To OnValueChanged")
    static FCk_Handle_IntegerAttribute
    BindTo_OnValueChanged(
        UPARAM(ref) FCk_Handle_IntegerAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        const FCk_Delegate_IntegerAttribute_OnValueChanged& InDelegate,
        ECk_Signal_BindingPolicy InBehavior = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName = "[Ck][IntegerAttribute] Unbind From OnValueChanged")
    static FCk_Handle_IntegerAttribute
    UnbindFrom_OnValueChanged(
        UPARAM(ref) FCk_Handle_IntegerAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        const FCk_Delegate_IntegerAttribute_OnValueChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName = "[Ck][IntegerAttribute] Bind To OnMinClamped",
              meta = (Keywords = "Depleted"))
    static FCk_Handle_IntegerAttribute
    BindTo_OnMinClamped(
        UPARAM(ref) FCk_Handle_IntegerAttribute& InAttribute,
        const FCk_Delegate_IntegerAttribute_OnClamped& InDelegate,
        ECk_Signal_BindingPolicy InBehavior = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName = "[Ck][IntegerAttribute] Unbind From OnMinClamped",
              meta = (Keywords = "Depleted"))
    static FCk_Handle_IntegerAttribute
    UnbindFrom_OnMinClamped(
        UPARAM(ref) FCk_Handle_IntegerAttribute& InAttribute,
        const FCk_Delegate_IntegerAttribute_OnClamped& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName = "[Ck][IntegerAttribute] Bind To OnMaxClamped",
              meta = (Keywords = "Filled"))
    static FCk_Handle_IntegerAttribute
    BindTo_OnMaxClamped(
        UPARAM(ref) FCk_Handle_IntegerAttribute& InAttribute,
        const FCk_Delegate_IntegerAttribute_OnClamped& InDelegate,
        ECk_Signal_BindingPolicy InBehavior = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Integer",
              DisplayName = "[Ck][IntegerAttribute] Unbind From OnMaxClamped",
              meta = (Keywords = "Filled"))
    static FCk_Handle_IntegerAttribute
    UnbindFrom_OnMaxClamped(
        UPARAM(ref) FCk_Handle_IntegerAttribute& InAttribute,
        const FCk_Delegate_IntegerAttribute_OnClamped& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_IntegerAttribute FCk_Handle_IntegerAttributeRefill"))
class CKATTRIBUTE_API UCk_Utils_IntegerAttributeRefill_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IntegerAttributeRefill_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_IntegerAttributeRefill);

public:
    friend class UCk_Utils_IntegerAttribute_UE;

private:
    static auto
    Add(
        FCk_Handle_FloatAttribute& InAttributeRefillEntity,
        ECk_Attribute_RefillState InStartingState) -> FCk_Handle_IntegerAttributeRefill;

    static auto
    Has(
        const FCk_Handle& InAttributeOwnerEntity) -> bool;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeRefill|Integer",
              DisplayName="[Ck][IntegerAttribute] Get Fill Rate")
    static float
    Get_FillRate(
        const FCk_Handle_IntegerAttributeRefill& InAttributeRefill);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeRefill|Integer",
              DisplayName="[Ck][IntegerAttribute] Get Refill State")
    static ECk_Attribute_RefillState
    Get_RefillState(
        const FCk_Handle_IntegerAttributeRefill& InAttributeRefill);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeRefill|Integer",
              DisplayName="[Ck][IntegerAttribute] Request Pause Refill")
    static FCk_Handle_IntegerAttributeRefill
    Request_Pause(
        UPARAM(ref) FCk_Handle_IntegerAttributeRefill& InAttributeRefill);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeRefill|Integer",
              DisplayName="[Ck][IntegerAttribute] Request Resume Refill")
    static FCk_Handle_IntegerAttributeRefill
    Request_Resume(
        UPARAM(ref) FCk_Handle_IntegerAttributeRefill& InAttributeRefill);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_IntegerAttributeModifier FCk_Handle_IntegerAttribute"))
class CKATTRIBUTE_API UCk_Utils_IntegerAttributeModifier_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IntegerAttributeModifier_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_IntegerAttributeModifier);

private:
    using IntegerAttributeModifier_Utils_Current = ck::TUtils_AttributeModifier<ck::FFragment_IntegerAttributeModifier_Current>;
    using IntegerAttributeModifier_Utils_Min     = ck::TUtils_AttributeModifier<ck::FFragment_IntegerAttributeModifier_Min>;
    using IntegerAttributeModifier_Utils_Max     = ck::TUtils_AttributeModifier<ck::FFragment_IntegerAttributeModifier_Max>;

    using RecordOfIntegerAttributeModifiers_Utils_Current = IntegerAttributeModifier_Utils_Current::RecordOfAttributeModifiers_Utils;
    using RecordOfIntegerAttributeModifiers_Utils_Min     = IntegerAttributeModifier_Utils_Min::RecordOfAttributeModifiers_Utils;
    using RecordOfIntegerAttributeModifiers_Utils_Max     = IntegerAttributeModifier_Utils_Max::RecordOfAttributeModifiers_Utils;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][IntegerAttribute] Add Modifier (Revocable)")
    static FCk_Handle_IntegerAttributeModifier
    Add_Revocable(
        UPARAM(ref) FCk_Handle_IntegerAttribute& InAttribute,
        FGameplayTag InModifierName,
        UPARAM(meta = (InvalidEnumValues="Override")) ECk_AttributeModifier_Operation InModifierOperation,
        const FCk_Fragment_IntegerAttributeModifier_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][IntegerAttribute] Add Modifier (Not Revocable)")
    static void
    Add_NotRevocable(
        UPARAM(ref) FCk_Handle_IntegerAttribute& InAttribute,
        ECk_AttributeModifier_Operation InModifierOperation,
        const FCk_Fragment_IntegerAttributeModifier_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Integer",
              DisplayName="[Ck][IntegerAttribute] Override Modifier")
    static FCk_Handle_IntegerAttributeModifier
    Override(
        UPARAM(ref) FCk_Handle_IntegerAttributeModifier& InAttributeModifierEntity,
        int32 InNewDelta);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Integer",
              DisplayName="[Ck][IntegerAttribute] Get Modifier Delta")
    static int32
    Get_Delta(
        const FCk_Handle_IntegerAttributeModifier& InAttributeModifierEntity,
        ECk_MinMaxCurrent InComponent = ECk_MinMaxCurrent::Current);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Integer",
              DisplayName="[Ck][IntegerAttribute] Try Get Modifier")
    static FCk_Handle_IntegerAttributeModifier
    TryGet(
        const FCk_Handle_IntegerAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent = ECk_MinMaxCurrent::Current);
    static FCk_Handle_IntegerAttributeModifier
    TryGet_If(
        const FCk_Handle_IntegerAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent,
        const TFunction<bool(FCk_Handle_IntegerAttributeModifier)>& InPredicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Integer",
              DisplayName="[Ck][IntegerAttribute] Remove Modifier")
    static FCk_Handle_IntegerAttribute
    Remove(
        UPARAM(ref) FCk_Handle_IntegerAttributeModifier& InAttributeModifierEntity);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InModifierEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Integer",
              DisplayName="[Ck][IntegerAttribute] For Each Modifier",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_IntegerAttributeModifier>
    ForEach(
        UPARAM(ref) FCk_Handle_IntegerAttribute& InAttribute,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);
    static auto
    ForEach(
        FCk_Handle_IntegerAttribute& InAttribute,
        const TFunction<void(FCk_Handle_IntegerAttributeModifier)>& InFunc,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Integer",
              DisplayName="[Ck][IntegerAttribute] For Each Modifier If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_IntegerAttributeModifier>
    ForEach_If(
        UPARAM(ref) FCk_Handle_IntegerAttribute& InAttribute,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);
    static auto
    ForEach_If(
        FCk_Handle_IntegerAttribute& InAttribute,
        const TFunction<void(FCk_Handle_IntegerAttributeModifier)>& InFunc,
        const TFunction<bool(FCk_Handle_IntegerAttributeModifier)>& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current) -> void;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Integer",
              DisplayName="[Ck][IntegerAttribute] Request Clear All Modifiers",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static void
    Request_ClearAllModifiers(
        FCk_Handle_IntegerAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);
};

// --------------------------------------------------------------------------------------------------------------------
