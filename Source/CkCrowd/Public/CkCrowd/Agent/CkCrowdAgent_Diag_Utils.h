#pragma once

#include "CkCrowd/Agent/CkCrowdAgent_Diag_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkCrowdAgent_Diag_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_CrowdAgent"))
class CKCROWD_API UCk_Utils_CrowdAgent_Diag_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_CrowdAgent_Diag_UE);

public:
    // Idempotent — calling on an already-tracked agent re-initialises its sample/metric state.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent|Diag",
              DisplayName = "[Ck][CrowdAgent][Diag] Track")
    static FCk_Handle_CrowdAgent
    Track(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        FVector InStartPos,
        FVector InGoalPos);

    // As Track(), but measures spatial-loop angle/radius around InSpatialCenterPos while all
    // progress, arrival, and efficiency evidence remains relative to InProgressGoalPos.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent|Diag",
              DisplayName = "[Ck][CrowdAgent][Diag] Track With Spatial Center")
    static FCk_Handle_CrowdAgent
    TrackWithSpatialCenter(
        UPARAM(ref) FCk_Handle_CrowdAgent& InAgent,
        FVector InStartPos,
        FVector InProgressGoalPos,
        FVector InSpatialCenterPos);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent|Diag",
              DisplayName = "[Ck][CrowdAgent][Diag] Is Tracked")
    static bool
    Is_Tracked(
        const FCk_Handle_CrowdAgent& InAgent);

    // Returns a default-constructed payload when the agent was never tracked. The returned struct
    // is a copy — safe to read across frames.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent|Diag",
              DisplayName = "[Ck][CrowdAgent][Diag] Get Recorder Data")
    static FCk_Fragment_CrowdAgent_DiagRecorderData
    Get_RecorderData(
        const FCk_Handle_CrowdAgent& InAgent);

    // Emit one cycle's worth of grep-able digest log lines for this tracked agent. Lines are
    // prefixed [CrowdDiag][C{cycle}][{station}][A{idx}] so a Saved/Logs/CkTests.log can be
    // grepped per agent or per cycle. The path samples are RDP-simplified (Ramer-Douglas-Peucker)
    // at ck.Crowd.RDPEpsilon (default 8cm) so a 90-sample raw path collapses to ~10-20 keypoints
    // — enough to trace what the agent did without flooding the log with frame-by-frame noise.
    //
    // Output format:
    //   [CrowdDiag][C{c}][{station}][A{i}] start=(x,y,z) goal=(x,y,z)
    //   [CrowdDiag][C{c}][{station}][A{i}] reached={true|false} t_to_goal={s}
    //   [CrowdDiag][C{c}][{station}][A{i}] path_len={cm} straight={cm} efficiency={0..1}
    //   [CrowdDiag][C{c}][{station}][A{i}] nearest_center_distance={cm} at t={s}
    //   [CrowdDiag][C{c}][{station}][A{i}] dir_reversals={n} max_angular_delta={deg}
    //   [CrowdDiag][C{c}][{station}][A{i}] spatial_turn=... loop_qualified=...
    //   [CrowdDiag][C{c}][{station}][A{i}] pipeline: ... (qualified loop, or recent opt-in window)
    //   [CrowdDiag][C{c}][{station}][A{i}] simplified_path: t={s} x={cm} y={cm} v={cm/s}
    //   ... (one per RDP-kept sample)
    //
    // InEmitRecentPipeline is diagnostic-only. It emits the latest five seconds of stored stage
    // and sampler rows instead of the latched qualified window so short/reversible visual loops
    // can be classified without changing recorder qualification or crowd behavior.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent|Diag",
              DisplayName = "[Ck][CrowdAgent][Diag] Emit Digest For Agent")
    static void
    EmitDigest_ForAgent(
        const FCk_Handle_CrowdAgent& InAgent,
        int32 InCycleNumber,
        const FString& InStationName,
        int32 InAgentIndex,
        bool InEmitRecentPipeline = false);
};

// --------------------------------------------------------------------------------------------------------------------
