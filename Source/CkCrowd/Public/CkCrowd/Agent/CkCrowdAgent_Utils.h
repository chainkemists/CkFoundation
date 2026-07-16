#pragma once

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"
#include "CkEcsExt/CkEcsExt_Utils.h"

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
    // Add the crowd-agent feature to InOwner. Creates a new child entity carrying the
    // CrowdAgent fragments + FTag_CrowdAgent_NeedsSetup; returns the typesafe handle.
    // Gate 0: structural setup only. Gate 2+ adds path / steering / velocity-bridge fragments
    // through the same Add() call.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Add Feature")
    static FCk_Handle_CrowdAgent
    Add(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_Fragment_CrowdAgent_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

    // Read the steering processor's per-frame output. Sub-task 2C will copy this into
    // FFragment_Velocity_Current via the velocity-bridge processor; until then this getter is the
    // primary way for tests/diagnostics to observe what the steering layer is producing.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Desired Velocity")
    static FVector
    Get_DesiredVelocity(
        const FCk_Handle_CrowdAgent& InHandle);

    // Read the separation processor's per-frame output — the neighbor-avoidance force BEFORE
    // steering damps it into the desired velocity. Unlike Get_DesiredVelocity, this is observable on
    // an IDLE agent (the separation processor excludes only FTag_CrowdAgent_Asleep, not Idle), which
    // makes it the only way to assert on the agent's neighbor-detection VOLUME in isolation from
    // path-follow dynamics. That matters: the probe volume is Jolt-side geometry, and a defect in it
    // (see the Y-axis cylinder incident) is invisible to every behavioral crowd test we own.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Separation Force")
    static FVector
    Get_SeparationForce(
        const FCk_Handle_CrowdAgent& InHandle);

    // Read the steering processor's waypoint cursor — the index into the active path's waypoint
    // array that the agent is currently walking toward. Diagnostic: paired with the path's waypoint
    // list (e.g. UCk_Utils_PathNetworkFollower_UE::Get_RouteResult's compiled waypoints, which
    // InstallExternalPath copies verbatim into the nav path result) it says exactly WHICH point the
    // agent is steering at — the only way to tell a legitimately-unreached waypoint apart from one
    // the agent already passed but never retired.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Current Waypoint Index")
    static int32
    Get_CurrentWaypointIndex(
        const FCk_Handle_CrowdAgent& InHandle);

    // Read the face-angle processor's current target yaw in DEGREES (converted from the radians
    // stored on the fragment for caller convenience). The agent's actual yaw — lerped toward this
    // target at _MaxTurnRate — is on its Transform's SceneNode rotation.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Target Yaw (degrees)")
    static float
    Get_TargetYawDegrees(
        const FCk_Handle_CrowdAgent& InHandle);

    // Issue a move-to request. The agent transitions Idle/Walking → PathPending; CkNavigation
    // resolves the path; the OnPathResolved processor flips PathPending → Walking; steering walks
    // the path. OnGoalReached fires when the agent crosses _ActiveArrivalRadius of the final
    // waypoint. _ArrivalRadiusOverride on InRequest lets callers override the default per-request.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Request Move To")
    static FCk_Handle_CrowdAgent
    Request_MoveTo(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        const FCk_Request_CrowdAgent_MoveTo& InRequest);

    // Cancel any active path. Agent transitions Walking/PathPending → Idle, _DesiredVelocity is
    // zeroed. Subsequent ticks see the bridge writing zero into _CurrentVelocity → agent halts.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Request Stop")
    static FCk_Handle_CrowdAgent
    Request_Stop(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent);

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

    // Fires once when the agent discovers its goal is UNREACHABLE — a neighbour is standing on it, or
    // the agent has stopped making progress. Not a failure: under the default HoldAndRetry policy the
    // agent stops, waits, and resumes on its own when the goal clears. The payload names the blocker,
    // which is what a gameplay-side queue manager needs in order to send the NPC somewhere else instead
    // of having it stand and wait.
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

    // The goal of the agent's current/last MoveTo (ZeroVector if it never had one). Lets a caller
    // that periodically re-anchors an agent distinguish "still the goal it is blocked on — leave
    // HoldAndRetry alone" from "the goal moved — a fresh MoveTo is warranted", since any new
    // MoveTo clears GoalBlocked and resets the no-progress sampler.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Active Goal")
    static FVector
    Get_ActiveGoal(
        const FCk_Handle_CrowdAgent& InAgent);

    // ---- Identity colour ---------------------------------------------------------------------
    //
    // Per-agent identity colour shared by every visualisation (capsule, breadcrumb path,
    // planned-path overlay, debugger Agent List swatch). The fragment is opt-in: only present
    // on agents that explicitly Set_DebugColor. Get_DebugColor returns the fragment value if
    // set, or a hash-derived stable colour otherwise — so untracked production agents still get
    // distinguishable colours when an in-world overlay happens to render them.

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Set Debug Color")
    static FCk_Handle_CrowdAgent
    Set_DebugColor(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        FLinearColor InColor);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Get Debug Color")
    static FLinearColor
    Get_DebugColor(
        const FCk_Handle_CrowdAgent& InAgent);

    // ---- Debug override (debugger "take control") --------------------------------------------
    //
    // Set/clear the FTag_CrowdAgent_DebugOverride marker. While set, gameplay code (e.g. the NPC SM)
    // must check Get_HasDebugOverride and skip issuing its own MoveTo, so a debugger-issued goal
    // isn't immediately overwritten. The debugger sets this on "take control", clears it on release.

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent",
              DisplayName="[Ck][CrowdAgent] Set Debug Override")
    static FCk_Handle_CrowdAgent
    Request_SetDebugOverride(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        bool InOverride);

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
    Get_InvalidHandle() { return {}; };
};

// --------------------------------------------------------------------------------------------------------------------
