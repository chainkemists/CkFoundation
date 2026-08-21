#include "CkCrowd/Queue/CkCrowdQueueAdapter_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkQueue/Queue/CkQueue_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdQueueAdapter_Dispatch);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdQueueAdapter_ObserveOutcome);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdQueueAdapter_MaintainFacing);
CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdQueueAdapter_EndPlay);

namespace
{
    auto
    NextNonZeroCorrelation(
        int32& InOutNextCorrelation)
    -> int32
    {
        if (InOutNextCorrelation <= 0)
        { InOutNextCorrelation = 1; }

        const auto Result = InOutNextCorrelation;
        InOutNextCorrelation = InOutNextCorrelation == MAX_int32
            ? 1
            : InOutNextCorrelation + 1;
        return Result;
    }

    auto
    StopOwnedEpisode(
        FCk_Handle_CrowdAgent& InAgent,
        ck::FFragment_CrowdQueueAdapter& InAdapter)
    -> void
    {
        const auto IssuedCorrelation = InAdapter.Get_IssuedCrowdCorrelationId();
        if (IssuedCorrelation != 0
            && UCk_Utils_CrowdAgent_UE::Get_ActiveMoveCorrelationId(InAgent) == IssuedCorrelation)
        { UCk_Utils_CrowdAgent_UE::Request_Stop(InAgent, {}); }

    }

    auto
    GetCurrentSnapshot(
        const FCk_Handle_CrowdAgent& InAgent,
        const ck::FFragment_CrowdQueueAdapter& InAdapter,
        FCk_Queue_MemberSnapshot& OutSnapshot)
    -> bool
    {
        if (ck::Is_NOT_Valid(InAdapter.Get_Queue())
            || NOT UCk_Utils_Queue_UE::Has(InAdapter.Get_Queue()))
        { return false; }

        return UCk_Utils_Queue_UE::TryGet_MemberSnapshot(
            InAdapter.Get_Queue(),
            FCk_Handle{InAgent},
            OutSnapshot);
    }
}

namespace ck
{
    auto
        FProcessor_CrowdQueueAdapter_Dispatch::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InAgent,
            FFragment_CrowdQueueAdapter& InAdapter)
        -> void
    {
        if (NOT UCk_Utils_Net_UE::Get_HasAuthority(InAgent))
        { return; }

        auto Agent = InAgent;
        auto Snapshot = FCk_Queue_MemberSnapshot{};
        const auto HasSnapshot = GetCurrentSnapshot(Agent, InAdapter, Snapshot);
        const auto LeaveRequested = InAgent.Has<FTag_CrowdQueueAdapter_LeaveRequested>();

        if (HasSnapshot)
        { InAdapter._JoinPending = false; }
        else if (InAdapter._JoinPending)
        {
            const auto QueueIsValid = ck::IsValid(InAdapter._Queue)
                && UCk_Utils_Queue_UE::Has(InAdapter._Queue);
            const auto QueueStillPending = QueueIsValid
                && UCk_Utils_Queue_UE::Get_State(InAdapter._Queue) != ECk_Queue_State::Invalidated
                && UCk_Utils_Queue_UE::Get_Revision(InAdapter._Queue)
                    <= InAdapter._PendingQueueRevision;
            if (QueueStillPending)
            { return; }

            InAdapter._Queue = {};
            InAdapter._JoinPending = false;
            InAdapter._PendingQueueRevision = 0;
        }
        else if (LeaveRequested)
        {
            InAdapter._Queue = {};
            InAdapter._PendingQueueRevision = 0;
        }

        const auto MoverMatches = HasSnapshot && Snapshot.Get_Mover() == FCk_Handle{InAgent};
        const auto CanMove = HasSnapshot
            && MoverMatches
            && NOT LeaveRequested
            && NOT Snapshot.Get_MovementSuppressed()
            && Snapshot.Get_AssignmentRevision() > 0
            && Snapshot.Get_State() == ECk_Queue_MemberState::Assigned;

        if (NOT CanMove)
        {
            StopOwnedEpisode(Agent, InAdapter);
            InAdapter._IssuedQueueAssignmentRevision = 0;
            InAdapter._IssuedCrowdCorrelationId = 0;
            InAdapter._ReportedQueueAssignmentRevision = 0;
            return;
        }

        if (InAdapter._IssuedQueueAssignmentRevision == Snapshot.Get_AssignmentRevision())
        {
            const auto ActiveCorrelation = UCk_Utils_CrowdAgent_UE::Get_ActiveMoveCorrelationId(Agent);
            const auto ActiveGoalMatches = UCk_Utils_CrowdAgent_UE::Get_ActiveGoal(Agent).Equals(
                Snapshot.Get_TargetWorldTransform().GetLocation(),
                1.0f);
            const auto MovementState = UCk_Utils_CrowdAgent_UE::Get_MovementState(Agent);
            const auto OwnedEpisodeIsTerminal = UCk_Utils_CrowdAgent_UE::Get_HasReachedActiveGoal(Agent)
                || UCk_Utils_CrowdAgent_UE::Get_IsGoalFailedHold(Agent);
            const auto OwnedEpisodeIsIntact = InAdapter._IssuedCrowdCorrelationId > 0
                && ActiveCorrelation == InAdapter._IssuedCrowdCorrelationId
                && ActiveGoalMatches
                && (MovementState != ECk_CrowdAgent_MovementState::Idle || OwnedEpisodeIsTerminal);
            if (OwnedEpisodeIsIntact)
            { return; }
        }

        StopOwnedEpisode(Agent, InAdapter);
        InAdapter._IssuedQueueAssignmentRevision = 0;
        InAdapter._IssuedCrowdCorrelationId = 0;
        InAdapter._ReportedQueueAssignmentRevision = 0;
        InAdapter._JoinPending = false;
        InAdapter._PendingQueueRevision = 0;
        const auto Correlation = NextNonZeroCorrelation(InAdapter._NextCrowdCorrelationId);
        auto MoveTo = FCk_Request_CrowdAgent_MoveTo{Snapshot.Get_TargetWorldTransform().GetLocation()};
        MoveTo.Set_CorrelationId(Correlation);
        UCk_Utils_CrowdAgent_UE::Request_MoveTo(Agent, MoveTo, {});

        InAdapter._IssuedQueueAssignmentRevision = Snapshot.Get_AssignmentRevision();
        InAdapter._IssuedCrowdCorrelationId = Correlation;
        InAdapter._ReportedQueueAssignmentRevision = 0;
    }

    auto
        FProcessor_CrowdQueueAdapter_ObserveOutcome::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InAgent,
            FFragment_CrowdQueueAdapter& InAdapter)
        -> void
    {
        if (NOT UCk_Utils_Net_UE::Get_HasAuthority(InAgent))
        { return; }

        auto Agent = InAgent;
        auto Snapshot = FCk_Queue_MemberSnapshot{};
        if (NOT GetCurrentSnapshot(Agent, InAdapter, Snapshot)
            || Snapshot.Get_Mover() != FCk_Handle{InAgent}
            || Snapshot.Get_MovementSuppressed()
            || Snapshot.Get_AssignmentRevision() != InAdapter._IssuedQueueAssignmentRevision
            || InAdapter._IssuedQueueAssignmentRevision <= 0
            || InAdapter._ReportedQueueAssignmentRevision == InAdapter._IssuedQueueAssignmentRevision
            || UCk_Utils_CrowdAgent_UE::Get_ActiveMoveCorrelationId(Agent) != InAdapter._IssuedCrowdCorrelationId)
        { return; }

        auto Outcome = TOptional<ECk_Queue_MovementOutcome>{};
        if (UCk_Utils_CrowdAgent_UE::Get_HasReachedActiveGoal(Agent))
        {
            if (UCk_Utils_CrowdAgent_UE::Get_MovementState(Agent)
                != ECk_CrowdAgent_MovementState::Idle)
            { return; }
            Outcome = ECk_Queue_MovementOutcome::Reached;
        }
        else if (UCk_Utils_CrowdAgent_UE::Get_IsGoalFailedHold(Agent))
        { Outcome = ECk_Queue_MovementOutcome::Failed; }

        if (NOT Outcome.IsSet())
        { return; }

        auto Queue = InAdapter._Queue;
        UCk_Utils_Queue_UE::Request_ReportMovementOutcome(
            Queue,
            FCk_Request_Queue_ReportMovementOutcome{
                FCk_Handle{InAgent},
                InAdapter._IssuedQueueAssignmentRevision,
                Outcome.GetValue()},
            {});
        InAdapter._ReportedQueueAssignmentRevision = InAdapter._IssuedQueueAssignmentRevision;
    }

    auto
        FProcessor_CrowdQueueAdapter_MaintainFacing::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InAgent,
            const FFragment_CrowdQueueAdapter& InAdapter,
            const FFragment_CrowdAgent_FaceAngle& InFaceAngle)
        -> void
    {
        if (NOT UCk_Utils_Net_UE::Get_HasAuthority(InAgent)
            || UCk_Utils_CrowdAgent_UE::Get_MovementState(InAgent) != ECk_CrowdAgent_MovementState::Idle)
        { return; }

        auto Agent = InAgent;
        auto Snapshot = FCk_Queue_MemberSnapshot{};
        if (NOT GetCurrentSnapshot(Agent, InAdapter, Snapshot)
            || Snapshot.Get_Mover() != FCk_Handle{InAgent}
            || (Snapshot.Get_State() != ECk_Queue_MemberState::AtSlot
                && Snapshot.Get_State() != ECk_Queue_MemberState::AtFront))
        { return; }

        auto Transform = UCk_Utils_Transform_UE::CastChecked(Agent);
        const auto TargetRotation = Snapshot.Get_TargetWorldTransform().GetRotation().Rotator();
        const auto CurrentRotation = UCk_Utils_Transform_UE::Get_EntityCurrentRotation(Transform);
        if (CurrentRotation.Equals(TargetRotation, 0.1f) && NOT InFaceAngle.Get_FacingEngaged())
        { return; }

        // An engaged Crowd-facing pass can have queued a same-frame rotation even while the current
        // transform still matches this target. Enqueue after it until Crowd facing disengages; once
        // settled, the equality gate avoids an indefinite request per idle queue member per frame.
        UCk_Utils_Transform_UE::Request_SetRotation(
            Transform,
            FCk_Request_Transform_SetRotation{TargetRotation}.Set_LocalWorld(ECk_LocalWorld::World),
            {});
    }

    auto
        FProcessor_CrowdQueueAdapter_EndPlay::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InAgent,
            FFragment_CrowdQueueAdapter& InAdapter)
        -> void
    {
        auto Agent = InAgent;
        StopOwnedEpisode(Agent, InAdapter);
        InAdapter._IssuedQueueAssignmentRevision = 0;
        InAdapter._IssuedCrowdCorrelationId = 0;
        InAdapter._ReportedQueueAssignmentRevision = 0;
    }
}
