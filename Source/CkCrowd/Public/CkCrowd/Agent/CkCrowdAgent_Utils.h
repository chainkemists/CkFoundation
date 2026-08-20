#pragma once

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CkCrowdAgent_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_CrowdAgent"))
class CKCROWD_API UCk_Utils_CrowdAgent_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_CrowdAgent_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_CrowdAgent);

public:
    // Compose the crowd-agent feature DIRECTLY onto InOwner (no child entity; the Transform feature is
    // required at the type level and is what every agent processor drives). At most ONE agent per entity.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Add Feature")
    static FCk_Handle_CrowdAgent
    Add(
        UPARAM(ref) FCk_Handle_Transform& InOwner,
        const FCk_Fragment_CrowdAgent_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    // The steering processor's per-frame output — what the steering layer produced this frame.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Desired Velocity")
    static FVector
    Get_DesiredVelocity(
        const FCk_Handle_CrowdAgent& InHandle);

    // The separation processor's per-frame output, BEFORE steering folds it into the desired velocity.
    // Unlike Get_DesiredVelocity this is readable on an IDLE agent, so it is the only way to assert on
    // the agent's neighbor-detection VOLUME in isolation from path-follow dynamics.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Separation Force")
    static FVector
    Get_SeparationForce(
        const FCk_Handle_CrowdAgent& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Transient Personal Space Scale")
    static float
    Get_TransientPersonalSpaceScale(
        const FCk_Handle_CrowdAgent& InAgent);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Transient Personal Space Remaining Seconds")
    static float
    Get_TransientPersonalSpaceRemainingSeconds(
        const FCk_Handle_CrowdAgent& InAgent);

    // The steering waypoint cursor — the index into the active path's waypoints the agent is walking
    // toward. Paired with the path's waypoint list it distinguishes a legitimately-unreached waypoint
    // from one the agent already passed but never retired.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Current Waypoint Index")
    static int32
    Get_CurrentWaypointIndex(
        const FCk_Handle_CrowdAgent& InHandle);

    // The face-angle processor's COMMITTED target pitch in DEGREES (the fragment stores radians).
    // Stays 0 for a grounded agent: only the flying facing processor pitches toward the climb or dive.
    // It holds its last value while the agent is below the facing speed floor.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Target Pitch (degrees)")
    static float
    Get_TargetPitchDegrees(
        const FCk_Handle_CrowdAgent& InHandle);

    // The face-angle processor's COMMITTED target yaw in DEGREES (the fragment stores radians) — the
    // heading the body is actually turning toward, not the raw desired-velocity heading: it holds
    // below the facing speed floor, and a large change is committed only once it has persisted. The
    // agent's actual yaw — slewed toward this target at _MaxTurnRate — is on its Transform's SceneNode
    // rotation.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Target Yaw (degrees)")
    static float
    Get_TargetYawDegrees(
        const FCk_Handle_CrowdAgent& InHandle);

    // Issue a move-to request: Idle/Walking → PathPending; CkNavigation resolves the path; OnPathResolved
    // flips PathPending → Walking. OnGoalReached fires when the agent crosses _ActiveArrivalRadius of the
    // final waypoint; _ArrivalRadiusOverride on InRequest overrides that default per-request.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Request Move To",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_CrowdAgent
    Request_MoveTo(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        const FCk_Request_CrowdAgent_MoveTo& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Request_MoveTo with a LIVE transform handle as the goal: the agent re-paths toward it on the
    // request's repath cadence, re-engaging after an arrival when the target walks out of reach again.
    // Ends on a plain MoveTo, a Stop, or the target dying (the agent keeps its last resolved goal).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Request Follow Target",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_CrowdAgent
    Request_FollowTarget(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        const FCk_Request_CrowdAgent_FollowTarget& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Cancel any active path. Agent transitions Walking/PathPending → Idle, _DesiredVelocity is
    // zeroed. Subsequent ticks see the bridge writing zero into _CurrentVelocity → agent halts.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Request Stop",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_CrowdAgent
    Request_Stop(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Override the agent's max speed at runtime (sprint/flee gaits). Applies from the next tick
    // and persists until the next SetMaxSpeed; does not disturb the active path or goal.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Request Set Max Speed",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_CrowdAgent
    Request_SetMaxSpeed(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        float InMaxSpeed,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Temporarily scales only reactive separation/personal space. Valid requests replace the
    // current scale and expiry (they never stack); expiry restores the normal scale automatically.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Request Set Transient Personal Space Scale",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_CrowdAgent
    Request_SetTransientPersonalSpaceScale(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        float InScale,
        float InDurationSeconds,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Replace the query-filter policy used by every Recast path this agent issues. Active Recast
    // and PathNetwork routes are rebuilt by default; Voxel paths do not consume this policy and
    // remain undisturbed. Set ForceReplan to Disable to defer until the next applicable request.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Request Set Nav Query Filter",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_CrowdAgent
    Request_SetNavQueryFilter(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        const FCk_Request_CrowdAgent_SetNavQueryFilter& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Nav Query Filter")
    static FGameplayTag
    Get_NavQueryFilter(
        const FCk_Handle_CrowdAgent& InAgent);

    // The params fragment's current max speed (post any runtime SetMaxSpeed override).
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Max Speed")
    static float
    Get_MaxSpeed(
        const FCk_Handle_CrowdAgent& InAgent);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Bind To OnGoalReached")
    static FCk_Handle_CrowdAgent
    BindTo_OnGoalReached(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        const FCk_Delegate_CrowdAgent_OnGoalReached& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Unbind From OnGoalReached")
    static FCk_Handle_CrowdAgent
    UnbindFrom_OnGoalReached(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        const FCk_Delegate_CrowdAgent_OnGoalReached& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Bind To OnGoalFailed")
    static FCk_Handle_CrowdAgent
    BindTo_OnGoalFailed(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        const FCk_Delegate_CrowdAgent_OnGoalFailed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Unbind From OnGoalFailed")
    static FCk_Handle_CrowdAgent
    UnbindFrom_OnGoalFailed(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        const FCk_Delegate_CrowdAgent_OnGoalFailed& InDelegate);

    // Fires once when the agent discovers its goal is UNREACHABLE — a neighbour standing on it, or no
    // progress. Not a failure: under the default HoldAndRetry policy the agent stops and resumes on its
    // own when the goal clears. The payload names the blocker, for a gameplay-side queue manager.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Bind To OnGoalBlocked")
    static FCk_Handle_CrowdAgent
    BindTo_OnGoalBlocked(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        const FCk_Delegate_CrowdAgent_OnGoalBlocked& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Unbind From OnGoalBlocked")
    static FCk_Handle_CrowdAgent
    UnbindFrom_OnGoalBlocked(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        const FCk_Delegate_CrowdAgent_OnGoalBlocked& InDelegate);

    // True while the agent is holding at a goal it cannot reach. Such an agent is Idle (it has stopped)
    // but still WANTS the goal, and will resume by itself the moment the goal clears.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Is Goal Blocked")
    static bool
    Get_IsGoalBlocked(
        const FCk_Handle_CrowdAgent& InAgent);

    // True after a terminal path/no-progress failure while the active goal is retained in a
    // stable Idle hold. Identical ordinary MoveTo requests are coalesced until an explicit
    // ForceRepath, a new correlation, or a genuinely different goal starts a fresh episode.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Is Goal Failed Hold")
    static bool
    Get_IsGoalFailedHold(
        const FCk_Handle_CrowdAgent& InAgent);

    // The agent's steering state (Idle / PathPending / Walking) — the state tags are EnTT
    // fragments BP/AS cannot read directly. None only before setup completes.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Movement State")
    static ECk_CrowdAgent_MovementState
    Get_MovementState(
        const FCk_Handle_CrowdAgent& InAgent);

    // The goal of the agent's current/last MoveTo (ZeroVector if it never had one). Lets a caller that
    // periodically re-anchors an agent tell "still the goal it is blocked on — leave HoldAndRetry alone"
    // from "the goal moved — a fresh MoveTo is warranted".
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Active Goal")
    static FVector
    Get_ActiveGoal(
        const FCk_Handle_CrowdAgent& InAgent);

    // Monotonic identity for the accepted MoveTo episode that owns ActiveGoal. It advances only
    // when HandleRequests accepts a new movement episode; an ignored same-goal request while
    // Walking leaves it unchanged. Zero means no MoveTo has been accepted in this runtime.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Active Move Episode")
    static int32
    Get_ActiveMoveEpisode(
        const FCk_Handle_CrowdAgent& InAgent);

    // Opaque identity supplied by the caller whose accepted MoveTo owns the active goal.
    // Zero means that request did not opt into correlation. Same-goal requests with the
    // same identity remain no-ops; a different nonzero identity claims a fresh episode.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Active Move Correlation Id")
    static int32
    Get_ActiveMoveCorrelationId(
        const FCk_Handle_CrowdAgent& InAgent);

    // True only after the current/last MoveTo genuinely reached its full-path goal. False while a new move/stop
    // request is queued, while pathing/walking, after Stop, and after a failed or partial-path completion. This is
    // retained runtime state rather than an edge, so a consumer later in the same runtime can reconcile it.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Has Reached Active Goal")
    static bool
    Get_HasReachedActiveGoal(
        const FCk_Handle_CrowdAgent& InAgent);

    // True as soon as the stationary cost-area painter exists. This deliberately does NOT mean
    // the async navmesh tile rebuild has finished; use Get_IsStationaryMarkupConfirmed before
    // assuming a path query can see the area. Exposed separately so diagnostics and tests can
    // observe the paint-to-confirm window without guessing from a timer.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Is Stationary Markup Painted")
    static bool
    Get_IsStationaryMarkupPainted(
        const FCk_Handle_CrowdAgent& InAgent);

    // True once the agent's stationary-markup cost disc is painted AND the rebuilt navmesh
    // actually reports the crowd cost area at the disc's location (ground truth, not a timer —
    // see FProcessor_CrowdAgent_PathRefresh). False while the agent is moving, before the
    // stationary delay elapses, or while the async tile rebake is still in flight. Lets tests
    // and gameplay gate on "this standing agent is genuinely priced into pathing right now"
    // instead of guessing with settle timers.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Is Stationary Markup Confirmed")
    static bool
    Get_IsStationaryMarkupConfirmed(
        const FCk_Handle_CrowdAgent& InAgent);

    // ---- Identity colour ---------------------------------------------------------------------

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Set Debug Color")
    static FCk_Handle_CrowdAgent
    Set_DebugColor(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        FLinearColor InColor);

    // The agent's Set_DebugColor value, or a hash-derived stable colour when it never opted in.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Debug Color")
    static FLinearColor
    Get_DebugColor(
        const FCk_Handle_CrowdAgent& InAgent);

    // ---- Debug override (debugger "take control") --------------------------------------------

    // While set, gameplay code (e.g. the NPC SM) must check Get_HasDebugOverride and skip issuing its
    // own MoveTo, so a debugger-issued goal isn't immediately overwritten.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Set Debug Override",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_CrowdAgent
    Request_SetDebugOverride(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        bool InOverride,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Has Debug Override")
    static bool
    Get_HasDebugOverride(
        const FCk_Handle_CrowdAgent& InAgent);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|CrowdAgent",
        DisplayName="[Ck][CrowdAgent] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_CrowdAgent
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|CrowdAgent",
        DisplayName="[Ck][CrowdAgent] Handle -> CrowdAgent Handle",
        meta = (CompactNodeTitle = "<AsCrowdAgent>", BlueprintAutocast))
    static FCk_Handle_CrowdAgent
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid CrowdAgent Handle",
        Category = "Ck|Utils|CrowdAgent",
        meta = (CompactNodeTitle = "INVALID_CrowdAgentHandle", Keywords = "make"))
    static FCk_Handle_CrowdAgent
    Get_InvalidHandle() { return {}; }
};

// --------------------------------------------------------------------------------------------------------------------
