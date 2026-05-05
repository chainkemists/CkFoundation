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
    // Opt this agent into diagnostic recording. Adds FTag_CrowdDiag_Tracked + the recorder
    // fragment, captures InStartPos / InGoalPos for digest-time efficiency math, and resets
    // sample/metric state. Idempotent — calling on an already-tracked agent re-initialises.
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

    // Read the recorder data for digest emission, debugger overlay, etc. Returns a default-
    // constructed payload if the agent has never been tracked. The returned struct is a copy —
    // safe to read across frames.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CrowdAgent|Diag",
              DisplayName = "[Ck][CrowdAgent][Diag] Get Recorder Data")
    static FCk_Fragment_CrowdAgent_DiagRecorderData
    Get_RecorderData(
        const FCk_Handle_CrowdAgent& InAgent);
};

// --------------------------------------------------------------------------------------------------------------------
