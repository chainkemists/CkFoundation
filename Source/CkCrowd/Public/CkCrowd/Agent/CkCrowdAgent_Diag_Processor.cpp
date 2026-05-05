#include "CkCrowdAgent_Diag_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DiagRecorder);

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    // Sample rate. Read once per ForEachEntity (cheap) so a console toggle is responsive without a
    // separate dirty path. 10Hz over a 9s diag-gym cycle = ~90 samples per agent.
    static TAutoConsoleVariable<int32> CVarSampleHz(
        TEXT("ck.Crowd.SampleHz"),
        10,
        TEXT("Sample rate (Hz) for the per-agent crowd path recorder.\n")
        TEXT("  Higher = more granular path/metric data, more memory per cycle.\n")
        TEXT("  Default 10Hz (~90 samples per 9s diag-gym cycle)."),
        ECVF_Cheat);

    // Arrival-window radius for the "reached goal" check. Decoupled from CrowdAgent's _ArrivalRadius
    // because the recorder only cares about "was the agent meaningfully close to the goal at any
    // point" — false negatives from a tighter threshold would inflate t_to_goal misses.
    constexpr auto ArrivalWindowCm = 60.0f;

    // Threshold for counting a heading change as a "reversal". 90° = the agent flipped its forward
    // direction by a full quarter turn between consecutive samples.
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
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_DiagRecorder& InRecorder)
        -> void
    {
        const auto Dt = static_cast<float>(InDeltaT.Get_Seconds());
        InRecorder._ElapsedSec += Dt;
        InRecorder._SecsSinceLastSample += Dt;

        const auto SampleHz = FMath::Max(1, CVarSampleHz.GetValueOnGameThread());
        const auto SampleIntervalSec = 1.0f / static_cast<float>(SampleHz);
        if (InRecorder._SecsSinceLastSample < SampleIntervalSec)
        { return; }
        InRecorder._SecsSinceLastSample = 0.0f;

        // Sample fields ------------------------------------------------------------------------

        auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
        if (ck::Is_NOT_Valid(TransformHandle))
        { return; }
        const auto Pos = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(TransformHandle);

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

        // Min separation across cycle. Reads the trimmed-and-sorted-by-distance neighbor cache;
        // first entry (if any) is the nearest neighbor. The cache stores cylinder-edge distance,
        // not centre-to-centre, so values < 0 mean overlap (clipping).
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

        // Max angular delta + reversal count — compare heading against previous sample.
        if (InRecorder._Samples.Num() >= 2)
        {
            const auto& Prev = InRecorder._Samples[InRecorder._Samples.Num() - 2];
            // Skip if either sample had no velocity — direction is meaningless.
            if (Prev._Speed > KINDA_SMALL_NUMBER && Speed > KINDA_SMALL_NUMBER)
            {
                auto DeltaRad = DirRad - Prev._DirRad;
                // Wrap to [-PI, PI] so a flip from -179° to +179° reads as 2°, not 358°.
                while (DeltaRad >  PI) { DeltaRad -= 2.0f * PI; }
                while (DeltaRad < -PI) { DeltaRad += 2.0f * PI; }
                const auto DeltaDeg = FMath::Abs(FMath::RadiansToDegrees(DeltaRad));
                if (DeltaDeg > InRecorder._MaxAngularDeltaDeg)
                { InRecorder._MaxAngularDeltaDeg = DeltaDeg; }
                if (DeltaDeg >= ReversalAngleDeg)
                { ++InRecorder._DirReversalCount; }
            }
        }

        // Reached-goal sticky flag.
        if (NOT InRecorder._Reached)
        {
            const auto DistToGoal = static_cast<float>(FVector::Dist(Pos, InRecorder._GoalPos));
            if (DistToGoal <= ArrivalWindowCm)
            {
                InRecorder._Reached = true;
                InRecorder._TimeToGoal = InRecorder._ElapsedSec;
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
