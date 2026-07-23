#include "CkAggro_Processor.h"

#include "CkAggro/CkAggro_Utils.h"
#include "CkAggro/CkAggroTarget_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Time/CkTime_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Aggro_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Aggro_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Aggro_EvaluationPacer);
CK_REGISTER_PROCESSOR(ck::FProcessor_Aggro_SelectActiveTarget);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_aggro_processor
{
    auto
    Get_Now(
        const FCk_Handle& InHandle)
        -> FCk_Time
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        const auto TimeParams = FCk_Utils_Time_GetWorldTime_Params{World};
        return UCk_Utils_Time_UE::Get_WorldTime(TimeParams).Get_WorldTime().Get_Time();
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Aggro_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Aggro_EvaluationParams& InEvaluationParams,
            FFragment_Aggro_EvaluationClock& InEvaluationClock)
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        const auto IntervalSeconds = InEvaluationParams.Get_EvaluationInterval().Get_Seconds();
        const auto JitterSeconds   = InEvaluationParams.Get_EvaluationJitter().Get_Seconds();
        const auto ArmSeconds      = IntervalSeconds + FMath::FRandRange(0.0, JitterSeconds);

        InEvaluationClock._EvaluationChrono = FCk_Chrono{FCk_Time{ArmSeconds}};
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Aggro_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InAggro,
            FFragment_Aggro_Requests& InRequests,
            FFragment_Aggro_TargetMap& InTargetMap,
            FFragment_Aggro_Current& InCurrent) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            DoHandleRequest(InAggro, InTargetMap, InCurrent, InRequest);

            if (InRequest.Get_IsRequestHandleValid())
            { InRequest.GetAndDestroyRequestHandle(); }
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        { InAggro.Remove<MarkedDirtyBy>(); }
    }

    auto
        FProcessor_Aggro_HandleRequests::
        DoHandleRequest(
            HandleType InAggro,
            FFragment_Aggro_TargetMap& InTargetMap,
            FFragment_Aggro_Current& InCurrent,
            const FCk_Request_Aggro_AddThreat& InRequest)
        -> void
    {
        const auto Tracked      = InRequest.Get_TrackedEntity();
        const auto TrackedValid = ck::IsValid(Tracked);
        CK_ENSURE_IF_NOT(TrackedValid,
            TEXT("Aggro [{}] AddThreat with an INVALID tracked entity — dropping"), InAggro)
        {}
        if (NOT TrackedValid)
        { return; }

        if (const auto Found = InTargetMap._TargetsByTrackedEntity.Find(Tracked))
        {
            auto Target = *Found;
            UCk_Utils_AggroTarget_UE::Request_AddThreat(Target, InRequest.Get_ThreatAmount());
            InAggro.AddOrGet<ck::FTag_Aggro_SelectionPending>();
            return;
        }

        const auto CanCreate = InRequest.Get_CreatePolicy() == ECk_Aggro_TargetCreationPolicy::CreateIfMissing;
        CK_ENSURE_IF_NOT(CanCreate,
            TEXT("Aggro [{}] AddThreat (RequireExisting) for untracked entity [{}] — dropping"), InAggro, Tracked)
        {}
        if (NOT CanCreate)
        { return; }

        auto TargetParams = FCk_Fragment_AggroTarget_ParamsData{Tracked};
        const auto& RequestTargetParams = InRequest.Get_TargetParams();
        TargetParams.Set_Instigator(InRequest.Get_Instigator())
                    .Set_Source(InRequest.Get_Source())
                    .Set_ThreatParams(RequestTargetParams.Get_ThreatParams())
                    .Set_ScoreParams(RequestTargetParams.Get_ScoreParams())
                    .Set_LifetimeParams(RequestTargetParams.Get_LifetimeParams());

        auto NewTarget = UCk_Utils_Aggro_UE::CreateTarget(InAggro, TargetParams);
        if (ck::IsValid(NewTarget))
        {
            UCk_Utils_AggroTarget_UE::Request_AddThreat(NewTarget, InRequest.Get_ThreatAmount());
            InAggro.AddOrGet<ck::FTag_Aggro_SelectionPending>();
        }
    }

    auto
        FProcessor_Aggro_HandleRequests::
        DoHandleRequest(
            HandleType InAggro,
            FFragment_Aggro_TargetMap& InTargetMap,
            FFragment_Aggro_Current& InCurrent,
            const FCk_Request_Aggro_RemoveTarget& InRequest)
        -> void
    {
        if (const auto Found = InTargetMap._TargetsByTrackedEntity.Find(InRequest.Get_TrackedEntity()))
        {
            auto Target = *Found;
            if (ck::IsValid(Target))
            { Target.AddOrGet<ck::FTag_AggroTarget_PendingForget>(); }
        }
    }

    auto
        FProcessor_Aggro_HandleRequests::
        DoHandleRequest(
            HandleType InAggro,
            FFragment_Aggro_TargetMap& InTargetMap,
            FFragment_Aggro_Current& InCurrent,
            const FCk_Request_Aggro_ClearAllTargets& InRequest)
        -> void
    {
        for (auto& Pair : InTargetMap._TargetsByTrackedEntity)
        {
            if (ck::IsValid(Pair.Value))
            { Pair.Value.AddOrGet<ck::FTag_AggroTarget_PendingForget>(); }
        }
    }

    auto
        FProcessor_Aggro_HandleRequests::
        DoHandleRequest(
            HandleType InAggro,
            FFragment_Aggro_TargetMap& InTargetMap,
            FFragment_Aggro_Current& InCurrent,
            const FCk_Request_Aggro_SetActiveTarget& InRequest)
        -> void
    {
        const auto Found = InTargetMap._TargetsByTrackedEntity.Find(InRequest.Get_TrackedEntity());
        if (Found == nullptr)
        { return; }

        auto NewActive = *Found;
        if (ck::Is_NOT_Valid(NewActive))
        { return; }

        auto PrevActive = InCurrent._ActiveTarget;
        if (PrevActive == NewActive)
        { return; }

        const auto Now = ck_aggro_processor::Get_Now(InAggro);

        if (ck::IsValid(PrevActive))
        { PrevActive.Try_Remove<ck::FTag_AggroTarget_IsActive>(); }
        NewActive.AddOrGet<ck::FTag_AggroTarget_IsActive>();

        InCurrent._ActiveTarget          = NewActive;
        InCurrent._ActiveTargetStartTime = Now;
        InCurrent._LastSwitchTime        = Now;

        UUtils_Signal_OnAggroActiveTargetChanged::Broadcast(InAggro, MakePayload(InAggro, PrevActive, NewActive));
    }

    auto
        FProcessor_Aggro_HandleRequests::
        DoHandleRequest(
            HandleType InAggro,
            FFragment_Aggro_TargetMap& InTargetMap,
            FFragment_Aggro_Current& InCurrent,
            const FCk_Request_Aggro_ClearActiveTarget& InRequest)
        -> void
    {
        auto PrevActive = InCurrent._ActiveTarget;
        if (ck::Is_NOT_Valid(PrevActive))
        { return; }

        PrevActive.Try_Remove<ck::FTag_AggroTarget_IsActive>();
        InCurrent._ActiveTarget = FCk_Handle_AggroTarget{};
        InAggro.AddOrGet<ck::FTag_Aggro_SelectionPending>();

        UUtils_Signal_OnAggroActiveTargetChanged::Broadcast(InAggro, MakePayload(InAggro, PrevActive, FCk_Handle_AggroTarget{}));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Aggro_EvaluationPacer::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InAggro,
            const FFragment_Aggro_EvaluationParams& InEvaluationParams,
            FFragment_Aggro_EvaluationClock& InEvaluationClock)
        -> void
    {
        auto& Chrono = InEvaluationClock._EvaluationChrono;
        Chrono.Tick(InDeltaT);

        if (NOT Chrono.Get_IsDone())
        { return; }

        const auto IntervalSeconds = InEvaluationParams.Get_EvaluationInterval().Get_Seconds();
        const auto JitterSeconds   = InEvaluationParams.Get_EvaluationJitter().Get_Seconds();
        Chrono = FCk_Chrono{FCk_Time{IntervalSeconds + FMath::FRandRange(0.0, JitterSeconds)}};
        ++InEvaluationClock._DebugEvaluationCount;

        FUtils_RecordOfAggroTargets::ForEach_ValidEntry(InAggro, [](FCk_Handle_AggroTarget InTarget)
        {
            InTarget.AddOrGet<ck::FTag_AggroTarget_EvaluationPending>();
        });

        InAggro.AddOrGet<ck::FTag_Aggro_SelectionPending>();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Aggro_SelectActiveTarget::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InAggro,
            FFragment_Aggro_Current& InCurrent,
            const FFragment_Aggro_SelectionParams& InSelectionParams,
            const FFragment_Aggro_SpatialParams& InSpatialParams,
            const FFragment_Aggro_TargetMap& InTargetMap) const
        -> void
    {
        const auto Now = ck_aggro_processor::Get_Now(InAggro);

        // Argmax over eligible candidates in the O(1) map (cap-bounded, never an all-targets sort).
        auto Best      = FCk_Handle_AggroTarget{};
        auto BestScore = -TNumericLimits<double>::Max();

        const auto Is_Eligible = [&](const FCk_Handle_AggroTarget& InCandidate) -> bool
        {
            if (ck::Is_NOT_Valid(InCandidate) || NOT InCandidate.Has<ck::FFragment_AggroTarget_Score>())
            { return false; }

            const auto& Score   = InCandidate.Get<ck::FFragment_AggroTarget_Score>();
            const auto  Tracked = ck::UAggroTarget_TrackedEntity_Utils::Get_StoredEntity(InCandidate);

            return UCk_Utils_Aggro_UE::Is_EligibleTarget(
                InCandidate.Has<ck::FTag_AggroTarget_CannotBecomeActive>(),
                InCandidate.Has<ck::FTag_AggroTarget_PendingForget>(),
                ck::IsValid(Tracked),
                Score.Get_Score(), InSelectionParams.Get_MinimumTargetScore(),
                InCandidate.Has<ck::FTag_AggroTarget_WithinRetention>(),
                Score.Get_Distance(), InSpatialParams.Get_AcquisitionDistance());
        };

        for (const auto& Pair : InTargetMap.Get_TargetsByTrackedEntity())
        {
            const auto Candidate = Pair.Value;
            if (NOT Is_Eligible(Candidate))
            { continue; }

            const auto CandidateScore = Candidate.Get<ck::FFragment_AggroTarget_Score>().Get_Score();
            if (CandidateScore > BestScore)
            {
                BestScore = CandidateScore;
                Best      = Candidate;
            }
        }

        auto       Incumbent         = InCurrent.Get_ActiveTarget();
        const auto IncumbentEligible = Is_Eligible(Incumbent);

        auto NewActive = Incumbent;
        auto Changed   = false;

        if (NOT IncumbentEligible)
        {
            // Incumbent invalid/forgotten/ineligible -> immediate adoption of Best (or clear); hysteresis bypassed.
            NewActive = Best;
            Changed   = NewActive != Incumbent;
        }
        else if (ck::IsValid(Best) && Best != Incumbent)
        {
            const auto IncumbentScore  = Incumbent.Get<ck::FFragment_AggroTarget_Score>().Get_Score();
            const auto SecsSinceSwitch = (Now - InCurrent.Get_LastSwitchTime()).Get_Seconds();
            const auto SecsSinceStart  = (Now - InCurrent.Get_ActiveTargetStartTime()).Get_Seconds();

            if (UCk_Utils_Aggro_UE::Should_SwitchTarget(
                    BestScore, IncumbentScore, InSelectionParams.Get_CurrentTargetBias(),
                    InSelectionParams.Get_TargetSwitchThreshold(), SecsSinceSwitch,
                    InSelectionParams.Get_TargetSwitchCooldown().Get_Seconds(),
                    SecsSinceStart, InSelectionParams.Get_MinimumAggroDuration().Get_Seconds()))
            {
                NewActive = Best;
                Changed   = true;
            }
        }

        if (Changed)
        {
            if (ck::IsValid(Incumbent))
            { Incumbent.Try_Remove<ck::FTag_AggroTarget_IsActive>(); }

            if (ck::IsValid(NewActive))
            {
                NewActive.AddOrGet<ck::FTag_AggroTarget_IsActive>();
                InCurrent._ActiveTargetStartTime = Now;
                InCurrent._LastSwitchTime        = Now;
            }

            InCurrent._ActiveTarget = NewActive;

            UUtils_Signal_OnAggroActiveTargetChanged::Broadcast(InAggro, MakePayload(InAggro, Incumbent, NewActive));
        }

        InAggro.Remove<MarkedDirtyBy>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
