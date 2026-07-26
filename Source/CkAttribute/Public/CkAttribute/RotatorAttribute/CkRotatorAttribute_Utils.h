#pragma once

#include <GameplayTagContainer.h>

#include "CkAttribute/CkAttribute_Utils.h"
#include "CkAttribute/RotatorAttribute/CkRotatorAttribute_Fragment.h"
#include "CkAttribute/RotatorAttribute/CkRotatorAttribute_Fragment_Data.h"
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkRotatorAttribute_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_RotatorAttribute"))
class CKATTRIBUTE_API UCk_Utils_RotatorAttribute_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_RotatorAttribute_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_RotatorAttribute);

public:
    friend class UCk_Utils_RotatorAttributeModifier_UE;

private:
    using RotatorAttribute_Utils_Min = ck::TUtils_Attribute<ck::FFragment_RotatorAttribute_Min>;
    using RotatorAttribute_Utils_Max = ck::TUtils_Attribute<ck::FFragment_RotatorAttribute_Max>;
    using RotatorAttribute_Utils_Current = ck::TUtils_Attribute<ck::FFragment_RotatorAttribute_Current>;

    using RecordOfRotatorAttributes_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfRotatorAttributes>;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName="[Ck][RotatorAttribute] Add New Attribute")
    static FCk_Handle_RotatorAttribute
    Add(
        UPARAM(ref) FCk_Handle& InAttributeOwnerEntity,
        const FCk_Fragment_RotatorAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName="[Ck][RotatorAttribute] Add Multiple New Attributes")
    static TArray<FCk_Handle_RotatorAttribute>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InAttributeOwnerEntity,
        const FCk_Fragment_MultipleRotatorAttribute_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InAttributeOwnerEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName="[Ck][RotatorAttribute] Has Any Attribute")
    static bool
    Has_Any(
        const FCk_Handle& InAttributeOwnerEntity);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Attribute|Rotator",
        DisplayName="[Ck][RotatorAttribute] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_RotatorAttribute
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Attribute|Rotator",
        DisplayName="[Ck][RotatorAttribute] Handle -> RotatorAttribute Handle",
        meta = (CompactNodeTitle = "<AsRotatorAttribute>", BlueprintAutocast))
    static FCk_Handle_RotatorAttribute
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid RotatorAttribute Handle",
        Category = "Ck|Utils|RotatorAttribute",
        meta = (CompactNodeTitle = "INVALID_RotatorAttributeHandle", Keywords = "make"))
    static FCk_Handle_RotatorAttribute
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName="[Ck][RotatorAttribute] Try Get Attribute")
    static FCk_Handle_RotatorAttribute
    TryGet(
        const FCk_Handle& InAttributeOwnerEntity,
        UPARAM(meta = (Categories = "RotatorAttribute")) FGameplayTag InAttributeName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName="[Ck][RotatorAttribute] Try Get Entity With Attribute In Ownership Chain")
    static FCk_Handle
    TryGet_Entity_WithAttribute_InOwnershipChain(
        const FCk_Handle& InHandle,
        UPARAM(meta = (Categories = "RotatorAttribute")) FGameplayTag InAttributeName);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName="[Ck][RotatorAttribute] For Each",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_RotatorAttribute>
    ForEach(
        UPARAM(ref) FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_RotatorAttribute)>& InFunc) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName="[Ck][RotatorAttribute] For Each If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_RotatorAttribute>
    ForEach_If(
        UPARAM(ref) FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate);
    static auto
    ForEach_If(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_RotatorAttribute)>& InFunc,
        const TFunction<bool(FCk_Handle_RotatorAttribute)>& InPredicate) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName="[Ck][RotatorAttribute] For Each (By Name)",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_RotatorAttribute>
    ForEach_ByName(
        UPARAM(ref) FCk_Handle& InAttributeOwner,
        UPARAM(meta = (Categories = "RotatorAttribute")) FGameplayTag InAttributeName,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_ByName(
        FCk_Handle& InAttributeOwner,
        FGameplayTag InAttributeName,
        const TFunction<void(FCk_Handle_RotatorAttribute)>& InFunc) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName="[Ck][RotatorAttribute] Has Component")
    static bool
    Has_Component(
        const FCk_Handle_RotatorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Min);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName="[Ck][RotatorAttribute] Get Base Value")
    static FRotator
    Get_BaseValue(
        const FCk_Handle_RotatorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName="[Ck][RotatorAttribute] Get Bonus Value")
    static FRotator
    Get_BonusValue(
        const FCk_Handle_RotatorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName="[Ck][RotatorAttribute] Get Final Value")
    static FRotator
    Get_FinalValue(
        const FCk_Handle_RotatorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName="[Ck][RotatorAttribute] Request Override Base Value")
    static FCk_Handle_RotatorAttribute
    Request_Override(
        UPARAM(ref) FCk_Handle_RotatorAttribute& InAttribute,
        FRotator InNewBaseValue,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName = "[Ck][RotatorAttribute] Bind To OnValueChanged")
    static FCk_Handle_RotatorAttribute
    BindTo_OnValueChanged(
        UPARAM(ref) FCk_Handle_RotatorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        const FCk_Delegate_RotatorAttribute_OnValueChanged& InDelegate,
        ECk_Signal_BindingPolicy InBehavior = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName = "[Ck][RotatorAttribute] Unbind From OnValueChanged")
    static FCk_Handle_RotatorAttribute
    UnbindFrom_OnValueChanged(
        UPARAM(ref) FCk_Handle_RotatorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        const FCk_Delegate_RotatorAttribute_OnValueChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName = "[Ck][RotatorAttribute] Bind To OnMinClamped",
              meta = (Keywords = "Depleted"))
    static FCk_Handle_RotatorAttribute
    BindTo_OnMinClamped(
        UPARAM(ref) FCk_Handle_RotatorAttribute& InAttribute,
        const FCk_Delegate_RotatorAttribute_OnClamped& InDelegate,
        ECk_Signal_BindingPolicy InBehavior = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName = "[Ck][RotatorAttribute] Unbind From OnMinClamped",
              meta = (Keywords = "Depleted"))
    static FCk_Handle_RotatorAttribute
    UnbindFrom_OnMinClamped(
        UPARAM(ref) FCk_Handle_RotatorAttribute& InAttribute,
        const FCk_Delegate_RotatorAttribute_OnClamped& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName = "[Ck][RotatorAttribute] Bind To OnMaxClamped",
              meta = (Keywords = "Filled"))
    static FCk_Handle_RotatorAttribute
    BindTo_OnMaxClamped(
        UPARAM(ref) FCk_Handle_RotatorAttribute& InAttribute,
        const FCk_Delegate_RotatorAttribute_OnClamped& InDelegate,
        ECk_Signal_BindingPolicy InBehavior = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Attribute|Rotator",
              DisplayName = "[Ck][RotatorAttribute] Unbind From OnMaxClamped",
              meta = (Keywords = "Filled"))
    static FCk_Handle_RotatorAttribute
    UnbindFrom_OnMaxClamped(
        UPARAM(ref) FCk_Handle_RotatorAttribute& InAttribute,
        const FCk_Delegate_RotatorAttribute_OnClamped& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_RotatorAttribute FCk_Handle_RotatorAttributeModifier"))
class CKATTRIBUTE_API UCk_Utils_RotatorAttributeModifier_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_RotatorAttributeModifier_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_RotatorAttributeModifier);

private:
    using RotatorAttributeModifier_Utils_Current = ck::TUtils_AttributeModifier<ck::FFragment_RotatorAttributeModifier_Current>;
    using RotatorAttributeModifier_Utils_Min     = ck::TUtils_AttributeModifier<ck::FFragment_RotatorAttributeModifier_Min>;
    using RotatorAttributeModifier_Utils_Max     = ck::TUtils_AttributeModifier<ck::FFragment_RotatorAttributeModifier_Max>;

    using RecordOfRotatorAttributeModifiers_Utils_Current = RotatorAttributeModifier_Utils_Current::RecordOfAttributeModifiers_Utils;
    using RecordOfRotatorAttributeModifiers_Utils_Min     = RotatorAttributeModifier_Utils_Min::RecordOfAttributeModifiers_Utils;
    using RecordOfRotatorAttributeModifiers_Utils_Max     = RotatorAttributeModifier_Utils_Max::RecordOfAttributeModifiers_Utils;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Rotator",
              DisplayName="[Ck][RotatorAttribute] Add Modifier (Revocable)")
    static FCk_Handle_RotatorAttributeModifier
    Add_Revocable(
        UPARAM(ref) FCk_Handle_RotatorAttribute& InAttribute,
        FGameplayTag InModifierName,
        UPARAM(meta = (InvalidEnumValues="Override")) ECk_AttributeModifier_Operation InModifierOperation,
        const FCk_Fragment_RotatorAttributeModifier_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Rotator",
              DisplayName="[Ck][RotatorAttribute] Add Modifier (Not Revocable)")
    static void
    Add_NotRevocable(
        UPARAM(ref) FCk_Handle_RotatorAttribute& InAttribute,
        ECk_AttributeModifier_Operation InModifierOperation,
        const FCk_Fragment_RotatorAttributeModifier_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Rotator",
              DisplayName="[Ck][RotatorAttribute] Override Modifier")
    static FCk_Handle_RotatorAttributeModifier
    Override(
        UPARAM(ref) FCk_Handle_RotatorAttributeModifier& InAttributeModifierEntity,
        FRotator InNewDelta);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Rotator",
              DisplayName="[Ck][RotatorAttribute] Get Modifier Delta")
    static FRotator
    Get_Delta(
        const FCk_Handle_RotatorAttributeModifier& InAttributeModifierEntity,
        ECk_MinMaxCurrent InComponent = ECk_MinMaxCurrent::Current);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AttributeModifier|Rotator",
              DisplayName="[Ck][RotatorAttribute] Try Get Modifier")
    static FCk_Handle_RotatorAttributeModifier
    TryGet(
        const FCk_Handle_RotatorAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent = ECk_MinMaxCurrent::Current);
    static FCk_Handle_RotatorAttributeModifier
    TryGet_If(
        const FCk_Handle_RotatorAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent,
        const TFunction<bool(FCk_Handle_RotatorAttributeModifier)>& InPredicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Rotator",
              DisplayName="[Ck][RotatorAttribute] Remove Modifier")
    static FCk_Handle_RotatorAttribute
    Remove(
        UPARAM(ref) FCk_Handle_RotatorAttributeModifier& InAttributeModifierEntity);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InModifierEntity);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Rotator",
              DisplayName="[Ck][RotatorAttribute] For Each Modifier",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_RotatorAttributeModifier>
    ForEach(
        UPARAM(ref) FCk_Handle_RotatorAttribute& InAttribute,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);
    static auto
    ForEach(
        FCk_Handle_RotatorAttribute& InAttribute,
        const TFunction<void(FCk_Handle_RotatorAttributeModifier)>& InFunc,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current) -> void;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Rotator",
              DisplayName="[Ck][RotatorAttribute] For Each Modifier If",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_RotatorAttributeModifier>
    ForEach_If(
        UPARAM(ref) FCk_Handle_RotatorAttribute& InAttribute,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);
    static auto
    ForEach_If(
        FCk_Handle_RotatorAttribute& InAttribute,
        const TFunction<void(FCk_Handle_RotatorAttributeModifier)>& InFunc,
        const TFunction<bool(FCk_Handle_RotatorAttributeModifier)>& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current) -> void;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AttributeModifier|Rotator",
              DisplayName="[Ck][RotatorAttribute] Request Clear All Modifiers",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static void
    Request_ClearAllModifiers(
        FCk_Handle_RotatorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current);
};

// --------------------------------------------------------------------------------------------------------------------
