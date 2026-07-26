#include "CkCrowdAgent_Diag_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"
#include "CkCrowd/CkCrowd_Stats.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DiagRecorder);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::DiagRecorder"), STAT_CkCrowd_DiagRecorderProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_diag_processor
{
    static TAutoConsoleVariable<int32> CVarSampleHz(
        TEXT("ck.Crowd.SampleHz"),
        10,
        TEXT("Sample rate (Hz) for the per-agent crowd path recorder.\n")
        TEXT("  Higher = more granular path/metric data, more memory per cycle.\n")
        TEXT("  Default 10Hz (~90 samples per 9s diag-gym cycle)."),
        ECVF_Cheat);

    // Deliberately wider than CrowdAgent's _ArrivalRadius: the recorder only asks "was the agent
    // meaningfully close at any point", and a tighter threshold would inflate t_to_goal misses.
    constexpr auto ArrivalWindowCm = 60.0f;

    constexpr auto ReversalAngleDeg = 90.0f;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_DiagRecorder::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_DiagRecorder& InRecorder)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DiagRecorderProc);

        const auto Dt = static_cast<float>(InDeltaT.Get_Seconds());
        InRecorder._ElapsedSec += Dt;
        InRecorder._SecsSinceLastSample += Dt;

        const auto SampleHz = FMath::Max(1, ck_crowd_agent_diag_processor::CVarSampleHz.GetValueOnGameThread());
        const auto SampleIntervalSec = 1.0f / static_cast<float>(SampleHz);
        if (InRecorder._SecsSinceLastSample < SampleIntervalSec)
        { return; }
        InRecorder._SecsSinceLastSample = 0.0f;

        // Sample fields ------------------------------------------------------------------------

        const auto Pos = InTransform.Get_Transform().GetLocation();

        const auto Vel = UCk_Utils_CrowdAgent_UE::Get_DesiredVelocity(InHandle);
        const auto Speed = static_cast<float>(Vel.Size());
        const auto DirRad = Speed > KINDA_SMALL_NUMBER
            ? static_cast<float>(FMath::Atan2(Vel.Y, Vel.X))
            : 0.0f;

        auto Sample = FCk_CrowdDiag_PathSample{};
        Sample._T      = InRecorder._ElapsedSec;
        Sample._Pos    = Pos;
        Sample._Speed  = Speed;
        Sample._DirRad = DirRad;
        InRecorder._Samples.Add(Sample);

        // Per-cycle metrics --------------------------------------------------------------------

        // The cache is sorted by distance and stores cylinder-EDGE distance, so entry 0 is the
        // nearest neighbour and a negative value means overlap.
        const auto& Neighbors = InNeighborCache.Get_Neighbors();
        if (Neighbors.Num() > 0)
        {
            const auto NearestDist = static_cast<float>(Neighbors[0].Get_Distance());
            if (NearestDist < InRecorder._MinSepAcrossCycle)
            {
                InRecorder._MinSepAcrossCycle = NearestDist;
                InRecorder._MinSepTime = InRecorder._ElapsedSec;
            }
        }

        if (InRecorder._Samples.Num() >= 2)
        {
            const auto& Prev = InRecorder._Samples[InRecorder._Samples.Num() - 2];
            const auto BothSamplesMoving = Prev._Speed > KINDA_SMALL_NUMBER && Speed > KINDA_SMALL_NUMBER;
            if (BothSamplesMoving)
            {
                auto DeltaRad = DirRad - Prev._DirRad;
                // Wrap to [-PI, PI] so a flip from -179° to +179° reads as 2°, not 358°.
                while (DeltaRad >  PI) { DeltaRad -= 2.0f * PI; }
                while (DeltaRad < -PI) { DeltaRad += 2.0f * PI; }
                const auto DeltaDeg = FMath::Abs(FMath::RadiansToDegrees(DeltaRad));
                if (DeltaDeg > InRecorder._MaxAngularDeltaDeg)
                { InRecorder._MaxAngularDeltaDeg = DeltaDeg; }
                if (DeltaDeg >= ck_crowd_agent_diag_processor::ReversalAngleDeg)
                { ++InRecorder._DirReversalCount; }
            }
        }

        if (NOT InRecorder._Reached)
        {
            const auto DistToGoal = static_cast<float>(FVector::Dist(Pos, InRecorder._GoalPos));
            if (DistToGoal <= ck_crowd_agent_diag_processor::ArrivalWindowCm)
            {
                InRecorder._Reached = true;
                InRecorder._TimeToGoal = InRecorder._ElapsedSec;
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
