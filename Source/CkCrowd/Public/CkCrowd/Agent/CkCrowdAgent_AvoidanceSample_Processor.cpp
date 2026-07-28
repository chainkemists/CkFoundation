#include "CkCrowdAgent_AvoidanceSample_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_AvoidanceSample_Algorithm.h"
#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_AvoidanceSample);

DECLARE_CYCLE_STAT(TEXT("Crowd::AvoidanceSample"), STAT_CkCrowd_AvoidanceSampleProc, STATGROUP_CkCrowd);
DECLARE_CYCLE_STAT(TEXT("Crowd::AvoidanceSample (scan)"), STAT_CkCrowd_AvoidanceSample_Scan, STATGROUP_CkCrowd);
DECLARE_DWORD_COUNTER_STAT(TEXT("Crowd Agents Sampled"), STAT_CkCrowd_AgentsSampled, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto FProcessor_CrowdAgent_AvoidanceSample::ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_CrowdAgent_Params& InParams,
        const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
        FFragment_CrowdAgent_DesiredVelocity& InDesired) const -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_AvoidanceSampleProc);

        const auto SelfAgent = UCk_Utils_CrowdAgent_UE::Cast(ck::MakeHandle(InHandle.Get_Entity(), _TransientEntity));
        if (NOT ck_crowd_agent_avoidance_sample_algorithm::ShouldRunSampling(SelfAgent, InNeighborCache))
        { return; }

        INC_DWORD_STAT(STAT_CkCrowd_AgentsSampled);
        if (InNeighborCache.Get_Neighbors().Num() == 0)
        { return; }

        const auto* Settings = UCk_Utils_Crowd_Settings_UE::Get();
        if (NOT IsValid(Settings))
        { return; }

        const auto DesiredVelocity = InDesired.Get_Velocity();
        const auto Velocity = UCk_Utils_Velocity_UE::Cast(SelfAgent);
        const auto CurrentVelocity = ck::IsValid(Velocity) ? UCk_Utils_Velocity_UE::Get_CurrentVelocity(Velocity) : FVector::ZeroVector;
        const auto Parameters = ck_crowd_agent_avoidance_sample_algorithm::FScoringParameters{
            InParams.Get_Radius(), InParams.Get_MaxSpeed(), Settings->Get_AvoidanceHorizonTime(),
            Settings->Get_AvoidanceWeightDesVel(), Settings->Get_AvoidanceWeightCurVel(),
            Settings->Get_AvoidanceWeightSide(), Settings->Get_AvoidanceWeightToi(),
            Settings->Get_AvoidanceSidePreference(),
            ck_crowd_agent_avoidance_sample_algorithm::MakeReachabilityParameters(
                Settings->Get_AccelClampMode(),
                InDesired.Get_LastVelocity(),
                InParams.Get_MaxAcceleration(),
                InParams.Get_MaxTurnRate(),
                static_cast<float>(InDeltaT.Get_Seconds()))};
        auto Cloud = ck_crowd_agent_avoidance_sample_algorithm::BuildSampleCloud(
            DesiredVelocity, Parameters._MaxSpeed, Settings->Get_AvoidanceVelBias(),
            Settings->Get_AvoidanceSampleAngularDivs(), Settings->Get_AvoidanceSampleRings(),
            Settings->Get_AvoidanceSampleDepth());

        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_AvoidanceSample_Scan);
        const auto Winner = ck_crowd_agent_avoidance_sample_algorithm::SelectWinner(Cloud, DesiredVelocity, CurrentVelocity, InNeighborCache, Parameters);
        if (Cloud._Candidates.IsValidIndex(Winner._CandidateIndex))
        { InDesired._Velocity = Cloud._Candidates[Winner._CandidateIndex]; }
    }
}

// --------------------------------------------------------------------------------------------------------------------
