#include "CkQueue/Queue/CkQueue_Formation_Processor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkQueue/Navigation/CkQueue_NavigationRevisionSubsystem.h"
#include "CkQueue/Queue/CkQueue_Layout_Algorithm.h"

#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "NavigationData.h"
#include "NavigationSystem.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Queue_Formation);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Queue_Formation::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue_Current& InCurrent)
        -> void
    {
        auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InQueue);
        const auto WorldIsValid = ck::IsValid(World);
        CK_ENSURE_IF_NOT(WorldIsValid,
            TEXT("Queue [{}] cannot resolve formation without a valid world"), InQueue)
        {}
        if (NOT WorldIsValid)
        { return; }

        auto RevisionSubsystem = World->GetSubsystem<UCk_Queue_NavigationRevisionSubsystem_UE>();
        const auto RevisionSubsystemIsValid = ck::IsValid(RevisionSubsystem);
        CK_ENSURE_IF_NOT(RevisionSubsystemIsValid,
            TEXT("Queue [{}] cannot resolve formation without its navigation revision subsystem"), InQueue)
        {}
        if (NOT RevisionSubsystemIsValid)
        { return; }

        RevisionSubsystem->TryEnsureBound();

        const auto NavigationRevision = RevisionSubsystem->Get_Revision();
        if (InCurrent._LastNavigationRevision == INDEX_NONE)
        { InCurrent._LastNavigationRevision = NavigationRevision; }

        if (InCurrent._State == ECk_Queue_State::WaitingForNavigationChange)
        {
            if (InCurrent._LastNavigationRevision == NavigationRevision)
            { return; }

            InCurrent._LastNavigationRevision = NavigationRevision;
            InCurrent._RetryEpisode = 0;
            InCurrent._NextFormationRetryWorldSeconds = 0.0;
            InCurrent._State = ECk_Queue_State::WaitingForFormation;
            ++InCurrent._Revision;

            UUtils_Signal_OnQueueFormationStateChanged::Broadcast(
                InQueue,
                MakePayload(
                    InQueue,
                    FCk_Queue_FormationState{
                        InCurrent._State,
                        ECk_Queue_EventReason::NavigationChanged,
                        InCurrent._Revision,
                        InCurrent._RetryEpisode}));
        }

        const auto WorldTimeSeconds = static_cast<double>(World->GetTimeSeconds());
        if (WorldTimeSeconds < InCurrent._NextFormationRetryWorldSeconds)
        { return; }

        if (InCurrent._RetryEpisode > 0 && InCurrent._NextFormationRetryWorldSeconds > 0.0)
        {
            ++InCurrent._Revision;
            InCurrent._NextFormationRetryWorldSeconds = 0.0;
            UUtils_Signal_OnQueueFormationStateChanged::Broadcast(
                InQueue,
                MakePayload(
                    InQueue,
                    FCk_Queue_FormationState{
                        InCurrent._State,
                        ECk_Queue_EventReason::NavigationRetryStarted,
                        InCurrent._Revision,
                        InCurrent._RetryEpisode}));
        }

        auto NavigationSystem = UNavigationSystemV1::GetCurrent(World);
        auto NavigationData = ck::IsValid(NavigationSystem)
            ? NavigationSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate)
            : nullptr;

        if (ck::Is_NOT_Valid(NavigationSystem) || ck::Is_NOT_Valid(NavigationData))
        {
            RecordRetryableFailure(
                InQueue,
                InParams,
                InCurrent,
                ECk_Queue_EventReason::NavigationUnavailable,
                WorldTimeSeconds,
                NavigationRevision);
            return;
        }

        const auto ProjectionExtent = FVector{
            InParams.Get_AgentRadiusUu() + InParams.Get_ClearanceMarginUu(),
            InParams.Get_AgentRadiusUu() + InParams.Get_ClearanceMarginUu(),
            InParams.Get_AgentHalfHeightUu() + InParams.Get_ClearanceMarginUu()};
        const auto CapsuleRadius = InParams.Get_AgentRadiusUu() + InParams.Get_ClearanceMarginUu();
        const auto CapsuleHalfHeight = InParams.Get_AgentHalfHeightUu() + InParams.Get_ClearanceMarginUu();

        auto QueryParams = FCollisionQueryParams{SCENE_QUERY_STAT(CkQueueFormation), false};
        const auto Validator = [&](FTransform& InOutCandidate, const TOptional<FTransform>& InPrevious) -> bool
        {
            auto Projected = FNavLocation{};
            if (NOT NavigationSystem->ProjectPointToNavigation(
                    InOutCandidate.GetLocation(),
                    Projected,
                    ProjectionExtent,
                    NavigationData))
            { return false; }

            const auto ProjectionShift = FVector::Dist2D(InOutCandidate.GetLocation(), Projected.Location);
            if (ProjectionShift > InParams.Get_ClearanceMarginUu())
            { return false; }

            InOutCandidate.SetLocation(Projected.Location);

            if (InPrevious.IsSet())
            {
                const auto PlanarSpacing = FVector::Dist2D(
                    InPrevious->GetLocation(),
                    InOutCandidate.GetLocation());
                if (NOT FMath::IsNearlyEqual(PlanarSpacing, InParams.Get_SlotSpacingUu(), 1.0f))
                { return false; }

                auto HitLocation = FVector::ZeroVector;
                if (NavigationData->Raycast(
                        InPrevious->GetLocation(),
                        InOutCandidate.GetLocation(),
                        HitLocation,
                        FSharedConstNavQueryFilter{}))
                { return false; }
            }

            constexpr auto FloorClearanceUu = 1.0f;
            const auto CapsuleCenter = InOutCandidate.GetLocation()
                + FVector::UpVector * (CapsuleHalfHeight + FloorClearanceUu);
            return NOT World->OverlapBlockingTestByChannel(
                CapsuleCenter,
                FQuat::Identity,
                ECC_Pawn,
                FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight),
                QueryParams);
        };

        const auto LayoutResult = queue::layout::Build(
            InCurrent._LastOwnerWorldTransform,
            InCurrent._Origins,
            InCurrent._Members.Num(),
            InParams.Get_SlotSpacingUu(),
            InParams.Get_MaxFormationSearchNodes(),
            InCurrent._LayoutAlgorithm,
            Validator);

        if (NOT LayoutResult.IsSuccess())
        {
            if (LayoutResult.Outcome == queue::layout::EBuildOutcome::SearchBudgetExhausted)
            {
                ++InCurrent._Revision;
                InCurrent._State = ECk_Queue_State::WaitingForNavigationChange;
                InCurrent._NextFormationRetryWorldSeconds = 0.0;
                InCurrent._LastNavigationRevision = NavigationRevision;
                UUtils_Signal_OnQueueFormationStateChanged::Broadcast(
                    InQueue,
                    MakePayload(
                        InQueue,
                        FCk_Queue_FormationState{
                            InCurrent._State,
                            ECk_Queue_EventReason::SearchBudgetExhausted,
                            InCurrent._Revision,
                            InCurrent._RetryEpisode}));
                return;
            }

            RecordRetryableFailure(
                InQueue,
                InParams,
                InCurrent,
                ECk_Queue_EventReason::NoViableFormation,
                WorldTimeSeconds,
                NavigationRevision);
            return;
        }

        const auto PlacementCountMatches = LayoutResult.Placements.Num() == InCurrent._Members.Num();
        CK_ENSURE_IF_NOT(PlacementCountMatches,
            TEXT("Queue [{}] layout returned [{}] placements for [{}] members"),
            InQueue,
            LayoutResult.Placements.Num(),
            InCurrent._Members.Num())
        {}
        if (NOT PlacementCountMatches)
        { return; }

        const auto PreviousMembers = InCurrent._Members;
        ++InCurrent._Revision;
        const auto AssignmentRevision = InCurrent._Revision;

        auto OriginCounts = TArray<int32>{};
        OriginCounts.Init(0, InCurrent._Origins.Num());

        for (auto MemberIndex = 0; MemberIndex < InCurrent._Members.Num(); ++MemberIndex)
        {
            const auto& Previous = PreviousMembers[MemberIndex];
            const auto& Placement = LayoutResult.Placements[MemberIndex];
            if (OriginCounts.IsValidIndex(Placement.OriginIndex))
            { ++OriginCounts[Placement.OriginIndex]; }

            InCurrent._Members[MemberIndex] = FCk_Queue_MemberSnapshot{
                Previous.Get_Member(),
                Previous.Get_Mover(),
                Previous.Get_Ticket(),
                Placement.OriginIndex,
                Placement.OriginRank,
                Placement.TargetWorldTransform,
                AssignmentRevision,
                Previous.Get_MovementSuppressed(),
                Previous.Get_MovementSuppressed()
                    ? ECk_Queue_MemberState::MovementSuppressed
                    : ECk_Queue_MemberState::Assigned};
        }

        InCurrent._State = ECk_Queue_State::Ready;
        InCurrent._RetryEpisode = 0;
        InCurrent._NextFormationRetryWorldSeconds = 0.0;
        InCurrent._LastNavigationRevision = NavigationRevision;
        InCurrent._Pressure = FCk_Queue_Pressure{
            InCurrent._Members.Num(),
            InParams.Get_SoftLimit(),
            InParams.Get_HardLimit(),
            InParams.Get_SoftLimit() > 0 && InCurrent._Members.Num() >= InParams.Get_SoftLimit(),
            InParams.Get_HardLimit() > 0 && InCurrent._Members.Num() >= InParams.Get_HardLimit(),
            OriginCounts,
            InCurrent._Revision};
        InQueue.Remove<MarkedDirtyBy>();

        for (auto MemberIndex = 0; MemberIndex < InCurrent._Members.Num(); ++MemberIndex)
        {
            const auto& Previous = PreviousMembers[MemberIndex];
            const auto& Current = InCurrent._Members[MemberIndex];
            const auto Reason = Previous.Get_OriginIndex() == INDEX_NONE
                ? ECk_Queue_EventReason::OriginAssigned
                : Previous.Get_OriginIndex() != Current.Get_OriginIndex()
                    ? ECk_Queue_EventReason::OriginReassigned
                    : ECk_Queue_EventReason::Reflowed;

            UUtils_Signal_OnQueueMemberStateChanged::Broadcast(
                InQueue,
                MakePayload(
                    InQueue,
                    FCk_Queue_MemberEvent{
                        InQueue,
                        Current,
                        Previous.Get_State(),
                        Reason,
                        InCurrent._Revision}));
        }

        UUtils_Signal_OnQueuePressureChanged::Broadcast(
            InQueue,
            MakePayload(InQueue, InCurrent._Pressure));
        UUtils_Signal_OnQueueFormationStateChanged::Broadcast(
            InQueue,
            MakePayload(
                InQueue,
                FCk_Queue_FormationState{
                    InCurrent._State,
                    ECk_Queue_EventReason::Reflowed,
                    InCurrent._Revision,
                    InCurrent._RetryEpisode}));
    }

    auto
        FProcessor_Queue_Formation::
        RecordRetryableFailure(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue_Current& InCurrent,
            ECk_Queue_EventReason InReason,
            double InWorldTimeSeconds,
            int32 InNavigationRevision)
        -> void
    {
        ++InCurrent._Revision;
        ++InCurrent._RetryEpisode;
        InCurrent._LastNavigationRevision = InNavigationRevision;

        const auto RetryBudgetRemains = InCurrent._RetryEpisode < InParams.Get_MaxNavigationRetries();
        if (RetryBudgetRemains)
        {
            InCurrent._State = ECk_Queue_State::WaitingForFormation;
            InCurrent._NextFormationRetryWorldSeconds = InWorldTimeSeconds
                + static_cast<double>(InParams.Get_NavigationRetryDelaySeconds());
        }
        else
        {
            InCurrent._State = ECk_Queue_State::WaitingForNavigationChange;
            InCurrent._NextFormationRetryWorldSeconds = 0.0;
        }

        UUtils_Signal_OnQueueFormationStateChanged::Broadcast(
            InQueue,
            MakePayload(
                InQueue,
                FCk_Queue_FormationState{
                    InCurrent._State,
                    RetryBudgetRemains
                        ? InReason
                        : ECk_Queue_EventReason::NavigationRetryExhausted,
                    InCurrent._Revision,
                    InCurrent._RetryEpisode}));
    }
}

// --------------------------------------------------------------------------------------------------------------------
