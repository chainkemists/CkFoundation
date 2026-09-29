#pragma once

#include "CkChain/CkChain_Fragment.h"
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"
#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkChain_Utils.generated.h"

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Chain"))
class CKCHAIN_API UCk_Utils_Chain_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Chain_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Chain);

    friend class UCk_Utils_Ecs_Base_UE;

    friend class UCk_Utils_ChainLink_UE;

private:
    struct RecordOfChains_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfChains> {};

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Add New Chain")
    static FCk_Handle_Chain
    Add(
        UPARAM(ref) FCk_Handle_Transform& InHead,
        const FCk_Chain_Spec& InParams);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Has Any Chain")
    static bool
    Has_Any(
        const FCk_Handle& InHead);

    static auto Has(const FCk_Handle& InHandle) -> bool;

private:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] DoCast", meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Chain
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);


    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] DoCastChecked", meta = (CompactNodeTitle = "<AsChain>", BlueprintAutocast))
    static FCk_Handle_Chain
    DoCastChecked(
        FCk_Handle InHandle);


    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Chain", DisplayName = "[Ck] Get Invalid Chain Handle",
        meta = (CompactNodeTitle = "INVALID_ChainHandle", Keywords = "make"))
    static FCk_Handle_Chain
    Get_InvalidHandle() { return {}; }

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Try Get Chain")
    static FCk_Handle_Chain
    TryGet_Chain(
        const FCk_Handle& InHead,
        UPARAM(meta = (Categories = "Chain")) FGameplayTag InChainName);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] For Each", meta = (AutoCreateRefTerm = "InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_Chain>
    ForEach_Chain(
        const FCk_Handle& InHead,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);

    static auto ForEach_Chain(const FCk_Handle& InHead, const TFunction<void(FCk_Handle_Chain)>& InFunc) -> void;

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Get Head")
    static FCk_Handle_Transform
    Get_Head(
        const FCk_Handle_Chain& InChain);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Get Solver")
    static ECk_Chain_Solver
    Get_Solver(
        const FCk_Handle_Chain& InChain);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Chain",
              DisplayName="[Ck][Chain] Get Net Policy")
    static ECk_Chain_NetPolicy
    Get_NetPolicy(
        const FCk_Handle_Chain& InChain);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Get Is Enabled")
    static bool
    Get_IsEnabled(
        const FCk_Handle_Chain& InChain);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Get Links")
    static TArray<FCk_Handle_ChainLink>
    Get_Links(
        const FCk_Handle_Chain& InChain);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Get Num Links")
    static int32
    Get_NumLinks(
        const FCk_Handle_Chain& InChain);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Get Length")
    static float
    Get_LengthCm(
        const FCk_Handle_Chain& InChain);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Get History Length")
    static float
    Get_HistoryLengthCm(
        const FCk_Handle_Chain& InChain);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Get Num History Samples")
    static int32
    Get_NumHistorySamples(
        const FCk_Handle_Chain& InChain);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Get Pose At Distance")
    static FTransform
    Get_PoseAtDistance(
        const FCk_Handle_Chain& InChain,
        float InDistanceFromHeadCm);

    static auto Get_HistorySamples(const FCk_Handle_Chain& InChain) -> TArray<FCk_Chain_PathSample>;

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Request Attach Link", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Chain
    Request_AttachLink(
        UPARAM(ref) FCk_Handle_Chain& InChain,
        const FCk_Request_Chain_AttachLink& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Request Detach Link", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Chain
    Request_DetachLink(
        UPARAM(ref) FCk_Handle_Chain& InChain,
        const FCk_Request_Chain_DetachLink& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Request Set Link Distance", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Chain
    Request_SetLinkDistance(
        UPARAM(ref) FCk_Handle_Chain& InChain,
        const FCk_Request_Chain_SetLinkDistance& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Request Split", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Chain
    Request_Split(
        UPARAM(ref) FCk_Handle_Chain& InChain,
        const FCk_Request_Chain_Split& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Request Reseed History", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Chain
    Request_ReseedHistory(
        UPARAM(ref) FCk_Handle_Chain& InChain,
        const FCk_Request_Chain_ReseedHistory& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Request Enable/Disable", meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Chain
    Request_EnableDisable(
        UPARAM(ref) FCk_Handle_Chain& InChain,
        const FCk_Request_Chain_EnableDisable& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Bind To OnLinkAttached")
    static FCk_Handle_Chain
    BindTo_OnLinkAttached(
        UPARAM(ref) FCk_Handle_Chain& InChain,
        const FCk_Delegate_Chain_OnLinkAttached& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Unbind From OnLinkAttached")
    static FCk_Handle_Chain
    UnbindFrom_OnLinkAttached(
        UPARAM(ref) FCk_Handle_Chain& InChain,
        const FCk_Delegate_Chain_OnLinkAttached& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Bind To OnLinkDetached")
    static FCk_Handle_Chain
    BindTo_OnLinkDetached(
        UPARAM(ref) FCk_Handle_Chain& InChain,
        const FCk_Delegate_Chain_OnLinkDetached& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Unbind From OnLinkDetached")
    static FCk_Handle_Chain
    UnbindFrom_OnLinkDetached(
        UPARAM(ref) FCk_Handle_Chain& InChain,
        const FCk_Delegate_Chain_OnLinkDetached& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Bind To OnSplit")
    static FCk_Handle_Chain
    BindTo_OnSplit(
        UPARAM(ref) FCk_Handle_Chain& InChain,
        const FCk_Delegate_Chain_OnSplit& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Unbind From OnSplit")
    static FCk_Handle_Chain
    UnbindFrom_OnSplit(
        UPARAM(ref) FCk_Handle_Chain& InChain,
        const FCk_Delegate_Chain_OnSplit& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Bind To OnHeadTeleported")
    static FCk_Handle_Chain
    BindTo_OnHeadTeleported(
        UPARAM(ref) FCk_Handle_Chain& InChain,
        const FCk_Delegate_Chain_OnHeadTeleported& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Chain", DisplayName = "[Ck][Chain] Unbind From OnHeadTeleported")
    static FCk_Handle_Chain
    UnbindFrom_OnHeadTeleported(
        UPARAM(ref) FCk_Handle_Chain& InChain,
        const FCk_Delegate_Chain_OnHeadTeleported& InDelegate);

};
