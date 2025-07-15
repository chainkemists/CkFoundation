#pragma once

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"

#include "CkInteraction/Interaction/CkInteraction_Fragment_Data.h"
#include "CkInteraction/Interaction/CkInteraction_Fragment.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkInteraction_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKINTERACTION_API UCk_Utils_Interaction_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Interaction_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Interaction);

public:
    using RecordOfInteractions_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfInteractions>;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Interaction",
              DisplayName="Add Interaction")
    static FCk_Handle_Interaction
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Interaction_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Interaction",
              DisplayName="Has Interaction")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Interaction",
        DisplayName="[Ck][Interaction] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Interaction
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Interaction",
        DisplayName="[Ck][Interaction] Handle -> Interaction Handle",
        meta = (CompactNodeTitle = "<AsInteraction>", BlueprintAutocast))
    static FCk_Handle_Interaction
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Interaction Handle",
        Category = "Ck|Utils|Interaction",
        meta = (CompactNodeTitle = "INVALID_InteractionHandle", Keywords = "make"))
    static FCk_Handle_Interaction
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Interaction",
        DisplayName="[Ck][Interaction] Request End Interaction")
    static FCk_Handle_Interaction
    Request_EndInteraction(
        UPARAM(ref) FCk_Handle_Interaction& InInteraction,
        const FCk_Request_Interaction_EndInteraction& InRequest);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Interaction",
        DisplayName = "[Ck][Interaction] Bind To OnInteractionFinished")
    static FCk_Handle_Interaction
    BindTo_OnInteractionFinished(
        UPARAM(ref) FCk_Handle_Interaction& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Interaction_OnInteractionFinished& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Interaction",
        DisplayName = "[Ck][Interaction] Unbind From OnInteractionFinished")
    static FCk_Handle_Interaction
    UnbindFrom_OnInteractionFinished(
        UPARAM(ref) FCk_Handle_Interaction& InHandle,
        const FCk_Delegate_Interaction_OnInteractionFinished& InDelegate);

public:
    UFUNCTION(BlueprintPure,
          Category = "Ck|Utils|Interaction",
          DisplayName="[Ck][Interaction] Try Get Interaction")
    static FCk_Handle_Interaction
    TryGet(
        const FCk_Handle& InInteractionOwner,
        const FCk_Handle& InSource,
        const FCk_Handle& InTarget,
        UPARAM(meta = (Categories = "InteractionChannel")) FGameplayTag InInteractionChannel);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Interaction",
        DisplayName="[Ck][Interaction] For Each",
        meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_Interaction>
    ForEach(
        UPARAM(ref) FCk_Handle& InInteractionOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach(
        FCk_Handle& InInteractionOwner,
        const TFunction<void(FCk_Handle_Interaction)>& InFunc) -> void;

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Interaction",
        DisplayName = "[Ck][Interaction] Get Interaction Distance (Source <-> Target)")
    static float
    Get_InteractionDistance(
        const FCk_Handle_Interaction& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Interaction",
        DisplayName = "[Ck][Interaction] Get Interaction Source")
    static FCk_Handle
    Get_InteractionSource(
        const FCk_Handle_Interaction& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Interaction",
        DisplayName = "[Ck][Interaction] Get Interaction Source Actor")
    static AActor*
    Get_InteractionSourceActor(
        const FCk_Handle_Interaction& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Interaction",
        DisplayName = "[Ck][Interaction] Get Interaction Instigator")
    static FCk_Handle
    Get_InteractionInstigator(
        const FCk_Handle_Interaction& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Interaction",
        DisplayName = "[Ck][Interaction] Get Interaction Target")
    static FCk_Handle
    Get_InteractionTarget(
        const FCk_Handle_Interaction& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Interaction",
        DisplayName = "[Ck][Interaction] Get Interaction Target Actor")
    static AActor*
    Get_InteractionTargetActor(
        const FCk_Handle_Interaction& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Interaction",
        DisplayName = "[Ck][Interaction] Get Interaction Channel")
    static const FGameplayTag&
    Get_InteractionChannel(
        const FCk_Handle_Interaction& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Interaction",
        DisplayName = "[Ck][Interaction] Get Interaction Completion Policy")
    static ECk_Interaction_CompletionPolicy
    Get_InteractionCompletionPolicy(
        const FCk_Handle_Interaction& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Interaction",
        DisplayName = "[Ck][Interaction] Get Interaction Duration")
    static FCk_Time
    Get_InteractionDuration(
        const FCk_Handle_Interaction& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Interaction",
        DisplayName = "[Ck][Interaction] Get Interaction Time Elapsed")
    static FCk_Time
    Get_InteractionTimeElapsed(
        const FCk_Handle_Interaction& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Interaction",
        DisplayName = "[Ck][Interaction] Get Interaction Time Attribute")
    static FCk_Handle_FloatAttribute
    Get_InteractionTimeAttribute(
        const FCk_Handle_Interaction& InHandle);
};

// --------------------------------------------------------------------------------------------------------------------