#pragma once

#include "CkInteractionResolver_Fragment_Data.h"
#include "CkInteractionResolver_Fragment.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkInteractionResolver_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_InteractionResolver_Persistent;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_InteractionResolver"))
class CKINTERACTION_API UCk_Utils_InteractionResolver_UE : public UBlueprintFunctionLibrary
{
    friend class ck::FProcessor_InteractionResolver_Persistent;

    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_InteractionResolver_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_InteractionResolver);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InteractionResolver",
              DisplayName="[Ck][InteractionResolver] Add Feature")
    static FCk_Handle_InteractionResolver
    Add(
        UPARAM(ref) FCk_Handle& InInteractSource,
        const FCk_InteractionResolver_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InteractionResolver",
              DisplayName="[Ck][InteractionResolver] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractionResolver",
        DisplayName="[Ck][InteractionResolver] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_InteractionResolver
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|InteractionResolver",
        DisplayName="[Ck][InteractionResolver] Handle -> InteractionResolver Handle",
        meta = (CompactNodeTitle = "<AsInteractionResolver>", BlueprintAutocast))
    static FCk_Handle_InteractionResolver
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid InteractionResolver Handle",
        Category = "Ck|Utils|InteractionResolver",
        meta = (CompactNodeTitle = "INVALID_InteractionResolverHandle", Keywords = "make"))
    static FCk_Handle_InteractionResolver
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractionResolver",
        DisplayName="[Ck][InteractionResolver] Request Start Intent")
    static void
    Request_StartIntent(
        UPARAM(ref) FCk_Handle_InteractionResolver& InResolver,
        const FCk_Request_InteractionResolver_StartIntent& InRequest);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractionResolver",
        DisplayName="[Ck][InteractionResolver] Request Stop Intent")
    static void
    Request_StopIntent(
        UPARAM(ref) FCk_Handle_InteractionResolver& InResolver,
        const FCk_Request_InteractionResolver_StopIntent& InRequest);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractionResolver",
        DisplayName="[Ck][InteractionResolver] Request Add InteractTarget")
    static void
    Request_AddInteractTarget(
        UPARAM(ref) FCk_Handle_InteractionResolver& InResolver,
        const FCk_Request_InteractionResolver_AddInteractTarget& InRequest);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractionResolver",
        DisplayName="[Ck][InteractionResolver] Request Remove InteractTarget")
    static void
    Request_RemoveInteractTarget(
        UPARAM(ref) FCk_Handle_InteractionResolver& InResolver,
        const FCk_Request_InteractionResolver_RemoveInteractTarget& InRequest);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractionResolver",
        DisplayName="[Ck][InteractionResolver] Request Remove All Targets By Channel")
    static void
    Request_RemoveAllTargetsByChannel(
        UPARAM(ref) FCk_Handle_InteractionResolver& InResolver,
        const FCk_Request_InteractionResolver_RemoveAllTargetsByChannel& InRequest);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|InteractionResolver",
        DisplayName="[Ck][InteractionResolver] Get Best InteractTargets (Immediate)")
    static TArray<FCk_Handle_InteractTarget>
    Get_BestInteractTargets_Immediate(
        const FCk_Handle_InteractionResolver& InResolver,
        FGameplayTag InIntent,
        const TArray<FCk_Handle_InteractTarget>& InAvailableTargets);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|InteractionResolver",
        DisplayName="[Ck][InteractionResolver] Get Best InteractTargets")
    static TArray<FCk_Handle_InteractTarget>
    Get_BestInteractTargets(
        const FCk_Handle_InteractionResolver& InResolver,
        FGameplayTag InIntent);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InteractionResolver",
              DisplayName = "[Ck][InteractionResolver] Bind To OnBestTargetsChanged")
    static FCk_Handle_InteractionResolver
    BindTo_OnBestTargetsChanged(
        UPARAM(ref) FCk_Handle_InteractionResolver& InResolver,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_InteractionResolver_OnBestTargetsChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InteractionResolver",
              DisplayName = "[Ck][InteractionResolver] Unbind From OnBestTargetsChanged")
    static FCk_Handle_InteractionResolver
    UnbindFrom_OnBestTargetsChanged(
        UPARAM(ref) FCk_Handle_InteractionResolver& InResolver,
        const FCk_Delegate_InteractionResolver_OnBestTargetsChanged& InDelegate);

private:
    static auto
    DoResolveTargets_Internal(
        const FCk_Handle_InteractionResolver& InResolver,
        const FGameplayTag& InIntent,
        const TArray<FCk_Handle_InteractTarget>& InAvailableTargets) -> TArray<FCk_Handle_InteractTarget>;

    static auto
    DoSortByDistance(
        const FCk_Handle_InteractionResolver& InResolver,
        TArray<FCk_Handle_InteractTarget>& InOutTargets) -> void;
};

// --------------------------------------------------------------------------------------------------------------------