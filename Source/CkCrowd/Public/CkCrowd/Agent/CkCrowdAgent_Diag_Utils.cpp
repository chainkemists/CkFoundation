#include "CkCrowdAgent_Diag_Utils.h"

#include "CkCrowd/CkCrowd_Log.h"

#include "HAL/IConsoleManager.h"

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

namespace ck_crowd_agent_diag_utils
{
    static TAutoConsoleVariable<float> CVarRDPEpsilon(
        TEXT("ck.Crowd.RDPEpsilon"),
        8.0f,
        TEXT("Ramer-Douglas-Peucker epsilon (cm) for path-simplification in the cycle digest.\n")
        TEXT("Lower = more keypoints kept; higher = more aggressive collapse. Default 8cm."),
        ECVF_Default);

    auto PerpendicularDistanceXY(const FVector& Point, const FVector& LineStart, const FVector& LineEnd) -> float
    {
        const auto Line = FVector(LineEnd.X - LineStart.X, LineEnd.Y - LineStart.Y, 0.0);
        const auto LineSqLen = Line.SizeSquared();
        if (LineSqLen < KINDA_SMALL_NUMBER)
        {
            const auto Dx = Point.X - LineStart.X;
            const auto Dy = Point.Y - LineStart.Y;
            return static_cast<float>(FMath::Sqrt(Dx * Dx + Dy * Dy));
        }
        const auto ToPt = FVector(Point.X - LineStart.X, Point.Y - LineStart.Y, 0.0);
        const auto T = FMath::Clamp(static_cast<float>(FVector::DotProduct(ToPt, Line) / LineSqLen), 0.0f, 1.0f);
        const auto Closest = FVector(LineStart.X + Line.X * T, LineStart.Y + Line.Y * T, 0.0);
        const auto Dx = Point.X - Closest.X;
        const auto Dy = Point.Y - Closest.Y;
        return static_cast<float>(FMath::Sqrt(Dx * Dx + Dy * Dy));
    }

    // Caller pre-marks the endpoints in OutKeep; this only fills the interior.
    auto RDP_Recursive(
        const TArray<FCk_CrowdDiag_PathSample>& InSamples,
        int32 InStartIdx,
        int32 InEndIdx,
        float InEpsilon,
        TArray<bool>& OutKeep) -> void
    {
        if (InEndIdx - InStartIdx < 2)
        { return; }

        auto MaxDist = 0.0f;
        auto MaxIdx = InStartIdx;
        for (auto i = InStartIdx + 1; i < InEndIdx; ++i)
        {
            const auto D = PerpendicularDistanceXY(
                InSamples[i].Get_Pos(),
                InSamples[InStartIdx].Get_Pos(),
                InSamples[InEndIdx].Get_Pos());
            if (D > MaxDist)
            {
                MaxDist = D;
                MaxIdx = i;
            }
        }

        if (MaxDist > InEpsilon)
        {
            OutKeep[MaxIdx] = true;
            RDP_Recursive(InSamples, InStartIdx, MaxIdx, InEpsilon, OutKeep);
            RDP_Recursive(InSamples, MaxIdx, InEndIdx, InEpsilon, OutKeep);
        }
    }
}

auto
    UCk_Utils_CrowdAgent_Diag_UE::
    EmitDigest_ForAgent(
        const FCk_Handle_CrowdAgent& InAgent,
        int32 InCycleNumber,
        const FString& InStationName,
        int32 InAgentIndex)
    -> void
{
    if (NOT ck::IsValid(InAgent))
    { return; }
    if (NOT InAgent.Has<ck::FFragment_CrowdAgent_DiagRecorder>())
    { return; }

    const auto& Recorder = InAgent.Get<ck::FFragment_CrowdAgent_DiagRecorder>();
    const auto& Samples = Recorder.Get_Samples();

    const auto Prefix = FString::Printf(TEXT("[CrowdDiag][C%d][%s][A%d]"),
        InCycleNumber, *InStationName, InAgentIndex);

    auto PathLen = 0.0;
    for (auto i = 1; i < Samples.Num(); ++i)
    {
        const auto& A = Samples[i - 1].Get_Pos();
        const auto& B = Samples[i].Get_Pos();
        const auto Dx = B.X - A.X;
        const auto Dy = B.Y - A.Y;
        PathLen += FMath::Sqrt(Dx * Dx + Dy * Dy);
    }
    const auto Straight = FVector::Dist(Recorder.Get_StartPos(), Recorder.Get_GoalPos());
    const auto Efficiency = (PathLen > KINDA_SMALL_NUMBER) ? (Straight / PathLen) : 0.0;

    ck::crowd::Display(TEXT("{} start=({:.1f}, {:.1f}, {:.1f}) goal=({:.1f}, {:.1f}, {:.1f})"),
        Prefix,
        Recorder.Get_StartPos().X, Recorder.Get_StartPos().Y, Recorder.Get_StartPos().Z,
        Recorder.Get_GoalPos().X, Recorder.Get_GoalPos().Y, Recorder.Get_GoalPos().Z);

    ck::crowd::Display(TEXT("{} reached={} t_to_goal={:.2f}"),
        Prefix,
        Recorder.Get_Reached() ? TEXT("true") : TEXT("false"),
        Recorder.Get_TimeToGoal());

    ck::crowd::Display(TEXT("{} path_len={:.1f} straight={:.1f} efficiency={:.3f}"),
        Prefix, PathLen, Straight, Efficiency);

    constexpr auto NoNeighboursObservedSentinel = -1.0f;
    const auto MinSep = (Recorder.Get_MinSepAcrossCycle() == TNumericLimits<float>::Max())
        ? NoNeighboursObservedSentinel
        : Recorder.Get_MinSepAcrossCycle();
    ck::crowd::Display(TEXT("{} min_sep_to_neighbors={:.1f} at t={:.2f}"),
        Prefix, MinSep, Recorder.Get_MinSepTime());

    ck::crowd::Display(TEXT("{} dir_reversals={} max_angular_delta={:.1f}"),
        Prefix, Recorder.Get_DirReversalCount(), Recorder.Get_MaxAngularDeltaDeg());

    if (Samples.Num() == 0)
    { return; }

    auto Keep = TArray<bool>{};
    Keep.SetNumZeroed(Samples.Num());
    Keep[0] = true;
    Keep[Samples.Num() - 1] = true;

    if (Samples.Num() >= 3)
    {
        const auto Epsilon = FMath::Max(0.1f, ck_crowd_agent_diag_utils::CVarRDPEpsilon.GetValueOnGameThread());
        ck_crowd_agent_diag_utils::RDP_Recursive(Samples, 0, Samples.Num() - 1, Epsilon, Keep);
    }

    for (auto i = 0; i < Samples.Num(); ++i)
    {
        if (NOT Keep[i])
        { continue; }
        const auto& S = Samples[i];
        ck::crowd::Display(TEXT("{} simplified_path: t={:.2f} x={:.1f} y={:.1f} z={:.1f} v={}"),
            Prefix, S.Get_T(), S.Get_Pos().X, S.Get_Pos().Y, S.Get_Pos().Z, static_cast<int32>(S.Get_Speed()));
    }
}

// --------------------------------------------------------------------------------------------------------------------
