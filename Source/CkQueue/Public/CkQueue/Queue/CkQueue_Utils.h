#pragma once

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkQueue/Queue/CkQueue_Fragment.h"

#include <NativeGameplayTags.h>

#include "CkQueue_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

CKQUEUE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Queue_CategoryName);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Queue"))
class CKQUEUE_API UCk_Utils_Queue_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Queue_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Queue);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Add")
    static FCk_Handle_Queue
    Add(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_Fragment_Queue_ParamsData& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Has Any Queue")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Queue
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Handle -> Queue Handle",
              meta = (CompactNodeTitle = "<AsQueue>", BlueprintAutocast))
    static FCk_Handle_Queue
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid Queue Handle",
              Category = "Ck|Utils|Queue",
              meta = (CompactNodeTitle = "INVALID_QueueHandle", Keywords = "make"))
    static FCk_Handle_Queue
    Get_InvalidHandle() { return {}; }

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Get Members")
    static TArray<FCk_Queue_MemberSnapshot>
    Get_Members(
        const FCk_Handle_Queue& InQueue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Get Member Count")
    static int32
    Get_MemberCount(
        const FCk_Handle_Queue& InQueue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Get Pressure")
    static FCk_Queue_Pressure
    Get_Pressure(
        const FCk_Handle_Queue& InQueue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Get State")
    static ECk_Queue_State
    Get_State(
        const FCk_Handle_Queue& InQueue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Get Can Accept Requests")
    static bool
    Get_CanAcceptRequests(
        const FCk_Handle_Queue& InQueue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Get Revision")
    static int32
    Get_Revision(
        const FCk_Handle_Queue& InQueue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Get Category")
    static FGameplayTag
    Get_Category(
        const FCk_Handle_Queue& InQueue);

    // A copied diagnostic view for debugger and PIE rendering. InAnyEntityInWorld only selects an ECS world; no
    // returned value retains that entity, a registry, an ECS handle, a fragment, or a UObject.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Get Debug Snapshots",
              meta = (DevelopmentOnly))
    static TArray<FCk_Queue_DebugSnapshot>
    Get_DebugSnapshots(
        const FCk_Handle& InAnyEntityInWorld);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Get Is Debug Draw Enabled",
              meta = (DevelopmentOnly))
    static bool
    Get_IsDebugDrawEnabled();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Set Debug Draw Enabled",
              meta = (DevelopmentOnly))
    static void
    Set_DebugDrawEnabled(
        bool InEnabled);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Get Layout Algorithm")
    static ECk_Queue_LayoutAlgorithm
    Get_LayoutAlgorithm(
        const FCk_Handle_Queue& InQueue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Get Slot Claim Policy")
    static ECk_Queue_SlotClaimPolicy
    Get_SlotClaimPolicy(
        const FCk_Handle_Queue& InQueue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Get Slot Spacing")
    static float
    Get_SlotSpacingUu(
        const FCk_Handle_Queue& InQueue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Get Slot Claim Radius")
    static float
    Get_SlotClaimRadiusUu(
        const FCk_Handle_Queue& InQueue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Get Slot Settle Radius")
    static float
    Get_SlotSettleRadiusUu(
        const FCk_Handle_Queue& InQueue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Get Slot Reacquire Radius")
    static float
    Get_SlotReacquireRadiusUu(
        const FCk_Handle_Queue& InQueue);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Get Is Member")
    static bool
    Get_IsMember(
        const FCk_Handle_Queue& InQueue,
        const FCk_Handle& InMember);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Try Get Member Snapshot")
    static bool
    TryGet_MemberSnapshot(
        const FCk_Handle_Queue& InQueue,
        const FCk_Handle& InMember,
        FCk_Queue_MemberSnapshot& OutResult);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Request Join",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Queue
    Request_Join(
        UPARAM(ref) FCk_Handle_Queue& InQueue,
        const FCk_Request_Queue_Join& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Request Restore Join",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Queue
    Request_RestoreJoin(
        UPARAM(ref) FCk_Handle_Queue& InQueue,
        const FCk_Request_Queue_RestoreJoin& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Request Leave",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Queue
    Request_Leave(
        UPARAM(ref) FCk_Handle_Queue& InQueue,
        const FCk_Request_Queue_Leave& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Request Advance",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Queue
    Request_Advance(
        UPARAM(ref) FCk_Handle_Queue& InQueue,
        const FCk_Request_Queue_Advance& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Request Set Layout",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Queue
    Request_SetLayout(
        UPARAM(ref) FCk_Handle_Queue& InQueue,
        const FCk_Request_Queue_SetLayout& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Request Set Movement Suppressed",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Queue
    Request_SetMovementSuppressed(
        UPARAM(ref) FCk_Handle_Queue& InQueue,
        const FCk_Request_Queue_SetMovementSuppressed& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Request Report Movement Outcome",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Queue
    Request_ReportMovementOutcome(
        UPARAM(ref) FCk_Handle_Queue& InQueue,
        const FCk_Request_Queue_ReportMovementOutcome& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Bind To On Member State Changed")
    static FCk_Handle_Queue
    BindTo_OnQueueMemberStateChanged(
        UPARAM(ref) FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Queue_OnMemberStateChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Unbind From On Member State Changed")
    static FCk_Handle_Queue
    UnbindFrom_OnQueueMemberStateChanged(
        UPARAM(ref) FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Queue_OnMemberStateChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Bind To On Pressure Changed")
    static FCk_Handle_Queue
    BindTo_OnQueuePressureChanged(
        UPARAM(ref) FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Queue_OnPressureChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Unbind From On Pressure Changed")
    static FCk_Handle_Queue
    UnbindFrom_OnQueuePressureChanged(
        UPARAM(ref) FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Queue_OnPressureChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Bind To On Formation State Changed")
    static FCk_Handle_Queue
    BindTo_OnQueueFormationStateChanged(
        UPARAM(ref) FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Queue_OnFormationStateChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Unbind From On Formation State Changed")
    static FCk_Handle_Queue
    UnbindFrom_OnQueueFormationStateChanged(
        UPARAM(ref) FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Queue_OnFormationStateChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Bind To On Invalidated")
    static FCk_Handle_Queue
    BindTo_OnQueueInvalidated(
        UPARAM(ref) FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Queue_OnInvalidated& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Queue",
              DisplayName = "[Ck][Queue] Unbind From On Invalidated")
    static FCk_Handle_Queue
    UnbindFrom_OnQueueInvalidated(
        UPARAM(ref) FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Queue_OnInvalidated& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
