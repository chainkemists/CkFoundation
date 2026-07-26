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

    // Emits grep-able digest lines prefixed [CrowdDiag][C{cycle}][{station}][A{idx}]; the path
    // samples are RDP-simplified at ck.Crowd.RDPEpsilon. Line format: CkCrowd/CLAUDE.md.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CrowdAgent|Diag",
              DisplayName = "[Ck][CrowdAgent][Diag] Emit Digest For Agent")
    static void
    EmitDigest_ForAgent(
        const FCk_Handle_CrowdAgent& InAgent,
        int32 InCycleNumber,
        const FString& InStationName,
        int32 InAgentIndex);
};

// --------------------------------------------------------------------------------------------------------------------
