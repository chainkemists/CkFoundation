#include "CkCrowdAgent_Diag_Utils.h"

#include "CkCrowd/CkCrowd_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_Diag_UE::
    Track(
        FCk_Handle_CrowdAgent& InAgent,
        FVector InStartPos,
        FVector InGoalPos)
    -> FCk_Handle_CrowdAgent
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAgent),
        TEXT("Invalid CrowdAgent handle [{}] passed to Track"), InAgent)
    { return InAgent; }

    InAgent.AddOrGet<ck::FTag_CrowdDiag_Tracked>();

    auto& Recorder = InAgent.AddOrGet<ck::FFragment_CrowdAgent_DiagRecorder>();
    Recorder._Samples.Reset();
    Recorder._ElapsedSec = 0.0f;
    Recorder._SecsSinceLastSample = 0.0f;
    Recorder._StartPos = InStartPos;
    Recorder._GoalPos = InGoalPos;
    Recorder._MinSepAcrossCycle = TNumericLimits<float>::Max();
    Recorder._MinSepTime = 0.0f;
    Recorder._DirReversalCount = 0;
    Recorder._MaxAngularDeltaDeg = 0.0f;
    Recorder._Reached = false;
    Recorder._TimeToGoal = 0.0f;

    return InAgent;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_Diag_UE::
    Is_Tracked(
        const FCk_Handle_CrowdAgent& InAgent)
    -> bool
{
    if (NOT ck::IsValid(InAgent))
    { return false; }
    return InAgent.Has<ck::FTag_CrowdDiag_Tracked>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_Diag_UE::
    Get_RecorderData(
        const FCk_Handle_CrowdAgent& InAgent)
    -> FCk_Fragment_CrowdAgent_DiagRecorderData
{
    if (NOT ck::IsValid(InAgent))
    { return {}; }
    if (NOT InAgent.Has<ck::FFragment_CrowdAgent_DiagRecorder>())
    { return {}; }
    return InAgent.Get<ck::FFragment_CrowdAgent_DiagRecorder>();
}

// --------------------------------------------------------------------------------------------------------------------
