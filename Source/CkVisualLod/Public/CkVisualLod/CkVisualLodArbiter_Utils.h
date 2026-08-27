#pragma once

#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkVisualLod/CkVisualLodArbiter_Fragment.h"
#include "CkVisualLod/CkVisualLodArbiter_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkVisualLodArbiter_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class ACk_Iskm_BatchedCrowd_Actor;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_VisualLodArbiter"))
class CKVISUALLOD_API UCk_Utils_VisualLodArbiter_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VisualLodArbiter_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_VisualLodArbiter);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLodArbiter",
              DisplayName="[Ck][VisualLodArbiter] Add Visual Lod Arbiter")
    static FCk_Handle_VisualLodArbiter
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_VisualLodArbiter_ParamsData& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|VisualLodArbiter",
        DisplayName="[Ck][VisualLodArbiter] Has Visual Lod Arbiter")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|VisualLodArbiter",
        DisplayName="[Ck][VisualLodArbiter] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_VisualLodArbiter
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|VisualLodArbiter",
        DisplayName="[Ck][VisualLodArbiter] Handle -> VisualLodArbiter Handle",
        meta = (CompactNodeTitle = "<AsVisualLodArbiter>", BlueprintAutocast))
    static FCk_Handle_VisualLodArbiter
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid VisualLodArbiter Handle",
        Category = "Ck|Utils|VisualLodArbiter",
        meta = (CompactNodeTitle = "INVALID_VisualLodArbiterHandle", Keywords = "make"))
    static FCk_Handle_VisualLodArbiter
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisualLodArbiter",
              DisplayName="[Ck][VisualLodArbiter] Get Observer")
    static FCk_Handle
    Get_Observer(
        const FCk_Handle_VisualLodArbiter& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisualLodArbiter",
              DisplayName="[Ck][VisualLodArbiter] Get Promoted Count")
    static int32
    Get_PromotedCount(
        const FCk_Handle_VisualLodArbiter& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisualLodArbiter",
              DisplayName="[Ck][VisualLodArbiter] Get Near Promoted Count")
    static int32
    Get_NearPromotedCount(
        const FCk_Handle_VisualLodArbiter& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisualLodArbiter",
              DisplayName="[Ck][VisualLodArbiter] Get Locked Promoted Count")
    static int32
    Get_LockedPromotedCount(
        const FCk_Handle_VisualLodArbiter& InHandle);

    // AlwaysPromoted entities + pool-exhaustion fallbacks — promoted, but charged to no budget
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisualLodArbiter",
              DisplayName="[Ck][VisualLodArbiter] Get Unbudgeted Promoted Count")
    static int32
    Get_UnbudgetedPromotedCount(
        const FCk_Handle_VisualLodArbiter& InHandle);

    // Null until the crowd stands up (lazily, on the first entity that needs a slot)
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisualLodArbiter",
              DisplayName="[Ck][VisualLodArbiter] Get Crowd")
    static ACk_Iskm_BatchedCrowd_Actor*
    Get_Crowd(
        const FCk_Handle_VisualLodArbiter& InHandle,
        int32 InCrowdIndex);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLodArbiter",
              DisplayName="[Ck][VisualLodArbiter] Request Set Observer",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VisualLodArbiter
    Request_SetObserver(
        UPARAM(ref) FCk_Handle_VisualLodArbiter& InHandle,
        const FCk_Request_VisualLodArbiter_SetObserver& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLodArbiter",
              DisplayName="[Ck][VisualLodArbiter] Request Clear Observer",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VisualLodArbiter
    Request_ClearObserver(
        UPARAM(ref) FCk_Handle_VisualLodArbiter& InHandle,
        const FCk_Request_VisualLodArbiter_ClearObserver& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLodArbiter",
              DisplayName = "[Ck][VisualLodArbiter] Bind To OnCrowdCreated")
    static FCk_Handle_VisualLodArbiter
    BindTo_OnCrowdCreated(
        UPARAM(ref) FCk_Handle_VisualLodArbiter& InHandle,
        const FCk_Delegate_VisualLodArbiter_CrowdCreated& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLodArbiter",
              DisplayName = "[Ck][VisualLodArbiter] Unbind From OnCrowdCreated")
    static FCk_Handle_VisualLodArbiter
    UnbindFrom_OnCrowdCreated(
        UPARAM(ref) FCk_Handle_VisualLodArbiter& InHandle,
        const FCk_Delegate_VisualLodArbiter_CrowdCreated& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
