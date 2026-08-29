#pragma once

#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkVisualLod/CkVisualLod_Fragment.h"
#include "CkVisualLod/CkVisualLod_Fragment_Data.h"

#include "CkVisualLod_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class ACk_Iskm_BatchedCrowd_Actor;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_VisualLod"))
class CKVISUALLOD_API UCk_Utils_VisualLod_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VisualLod_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_VisualLod);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLod",
              DisplayName="[Ck][VisualLod] Add Visual Lod")
    static FCk_Handle_VisualLod
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_VisualLod_ParamsData& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|VisualLod",
        DisplayName="[Ck][VisualLod] Has Visual Lod")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|VisualLod",
        DisplayName="[Ck][VisualLod] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_VisualLod
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|VisualLod",
        DisplayName="[Ck][VisualLod] Handle -> VisualLod Handle",
        meta = (CompactNodeTitle = "<AsVisualLod>", BlueprintAutocast))
    static FCk_Handle_VisualLod
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid VisualLod Handle",
        Category = "Ck|Utils|VisualLod",
        meta = (CompactNodeTitle = "INVALID_VisualLodHandle", Keywords = "make"))
    static FCk_Handle_VisualLod
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisualLod",
              DisplayName="[Ck][VisualLod] Get Representation")
    static ECk_VisualLod_Representation
    Get_Representation(
        const FCk_Handle_VisualLod& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisualLod",
              DisplayName="[Ck][VisualLod] Get Is Hidden")
    static bool
    Get_IsHidden(
        const FCk_Handle_VisualLod& InHandle);

    // 1 = the far member is fully visible, 0 = fully dissolved. 1 while no fade is running
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisualLod",
              DisplayName="[Ck][VisualLod] Get Fade Alpha")
    static float
    Get_FadeAlpha(
        const FCk_Handle_VisualLod& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisualLod",
              DisplayName="[Ck][VisualLod] Get Promote Lock Count")
    static int32
    Get_PromoteLockCount(
        const FCk_Handle_VisualLod& InHandle);

    // INDEX_NONE while the entity holds no crowd member slot
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisualLod",
              DisplayName="[Ck][VisualLod] Get Member Index")
    static int32
    Get_MemberIndex(
        const FCk_Handle_VisualLod& InHandle);

    // The crowd Get_MemberIndex addresses. Read this at signal-handler time — member indices are
    // only valid against the exact crowd recorded at acquisition
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisualLod",
              DisplayName="[Ck][VisualLod] Get Crowd")
    static ACk_Iskm_BatchedCrowd_Actor*
    Get_Crowd(
        const FCk_Handle_VisualLod& InHandle);

    // Invalid handle while not promoted
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisualLod",
              DisplayName="[Ck][VisualLod] Try Get Proxy")
    static FCk_Handle_IskmProxy
    TryGet_Proxy(
        const FCk_Handle_VisualLod& InHandle);

public:
    /**
     * Inspection surface. C++ only: Get_FadePhase returns a ck:: enum, and the rest are the
     * arbiter's raw per-member bookkeeping — the debugger reads them directly, no game does.
     */

    // Distance to the view as of the last arbiter update that RANKED this member. -1 until the
    // arbiter has ranked it once; stale on any tick the member was skipped before that point
    // (suspended, hidden with no slot, or promoted through the pool-exhaustion fallback)
    static auto
    Get_LastDistance(
        const FCk_Handle_VisualLod& InHandle) -> float;

    // In-view as the ranking saw it, with the same staleness as Get_LastDistance
    static auto
    Get_LastInView(
        const FCk_Handle_VisualLod& InHandle) -> bool;

    static auto
    Get_FadePhase(
        const FCk_Handle_VisualLod& InHandle) -> ck::EVisualLod_FadePhase;

    static auto
    Get_PromotedViaLock(
        const FCk_Handle_VisualLod& InHandle) -> bool;

    static auto
    Get_PromotedUnbudgeted(
        const FCk_Handle_VisualLod& InHandle) -> bool;

    static auto
    Get_PreemptDemote(
        const FCk_Handle_VisualLod& InHandle) -> bool;

    // Last sequence/rate pushed to the PROMOTED PROXY; INDEX_NONE while not promoted
    static auto
    Get_ProxySequenceIndex(
        const FCk_Handle_VisualLod& InHandle) -> int32;

    static auto
    Get_ProxyRate(
        const FCk_Handle_VisualLod& InHandle) -> float;

    // Last sequence/rate pushed to the CROWD SLOT; INDEX_NONE while the member holds no slot
    static auto
    Get_FarSequenceIndex(
        const FCk_Handle_VisualLod& InHandle) -> int32;

    static auto
    Get_FarRate(
        const FCk_Handle_VisualLod& InHandle) -> float;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLod",
              DisplayName="[Ck][VisualLod] Request Set Arbiter",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VisualLod
    Request_SetArbiter(
        UPARAM(ref) FCk_Handle_VisualLod& InHandle,
        const FCk_Request_VisualLod_SetArbiter& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLod",
              DisplayName="[Ck][VisualLod] Request Set Visibility",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VisualLod
    Request_SetVisibility(
        UPARAM(ref) FCk_Handle_VisualLod& InHandle,
        const FCk_Request_VisualLod_SetVisibility& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLod",
              DisplayName="[Ck][VisualLod] Request Set Far Anim",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VisualLod
    Request_SetFarAnim(
        UPARAM(ref) FCk_Handle_VisualLod& InHandle,
        const FCk_Request_VisualLod_SetFarAnim& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLod",
              DisplayName="[Ck][VisualLod] Request Set Renderer",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VisualLod
    Request_SetRenderer(
        UPARAM(ref) FCk_Handle_VisualLod& InHandle,
        const FCk_Request_VisualLod_SetRenderer& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLod",
              DisplayName="[Ck][VisualLod] Request Suspend",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VisualLod
    Request_Suspend(
        UPARAM(ref) FCk_Handle_VisualLod& InHandle,
        const FCk_Request_VisualLod_Suspend& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLod",
              DisplayName="[Ck][VisualLod] Request Resume",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VisualLod
    Request_Resume(
        UPARAM(ref) FCk_Handle_VisualLod& InHandle,
        const FCk_Request_VisualLod_Resume& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Immediate mutator: bumps the lock counter inline and enqueues nothing — the arbiter's next
    // update evaluates it, so callers poll for the proxy rather than expecting one this frame
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLod",
              DisplayName="[Ck][VisualLod] Request Acquire Promote Lock",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VisualLod
    Request_AcquirePromoteLock(
        UPARAM(ref) FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Immediate mutator — see Request_AcquirePromoteLock
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLod",
              DisplayName="[Ck][VisualLod] Request Release Promote Lock",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VisualLod
    Request_ReleasePromoteLock(
        UPARAM(ref) FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLod",
              DisplayName = "[Ck][VisualLod] Bind To OnMemberAcquired")
    static FCk_Handle_VisualLod
    BindTo_OnMemberAcquired(
        UPARAM(ref) FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_VisualLod_MemberEvent& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLod",
              DisplayName = "[Ck][VisualLod] Bind To OnPromoted")
    static FCk_Handle_VisualLod
    BindTo_OnPromoted(
        UPARAM(ref) FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_VisualLod_Promoted& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLod",
              DisplayName = "[Ck][VisualLod] Bind To OnDemoteFinishing")
    static FCk_Handle_VisualLod
    BindTo_OnDemoteFinishing(
        UPARAM(ref) FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_VisualLod_MemberEvent& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLod",
              DisplayName = "[Ck][VisualLod] Bind To OnMemberReleased")
    static FCk_Handle_VisualLod
    BindTo_OnMemberReleased(
        UPARAM(ref) FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_VisualLod_MemberEvent& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLod",
              DisplayName = "[Ck][VisualLod] Unbind From OnMemberAcquired")
    static FCk_Handle_VisualLod
    UnbindFrom_OnMemberAcquired(
        UPARAM(ref) FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_VisualLod_MemberEvent& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLod",
              DisplayName = "[Ck][VisualLod] Unbind From OnPromoted")
    static FCk_Handle_VisualLod
    UnbindFrom_OnPromoted(
        UPARAM(ref) FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_VisualLod_Promoted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLod",
              DisplayName = "[Ck][VisualLod] Unbind From OnDemoteFinishing")
    static FCk_Handle_VisualLod
    UnbindFrom_OnDemoteFinishing(
        UPARAM(ref) FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_VisualLod_MemberEvent& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisualLod",
              DisplayName = "[Ck][VisualLod] Unbind From OnMemberReleased")
    static FCk_Handle_VisualLod
    UnbindFrom_OnMemberReleased(
        UPARAM(ref) FCk_Handle_VisualLod& InHandle,
        const FCk_Delegate_VisualLod_MemberEvent& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
