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

namespace ck_crowd_queue_adapter_processor
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
        const auto HasSnapshot = ck_crowd_queue_adapter_processor::GetCurrentSnapshot(Agent, InAdapter, Snapshot);
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
        else if (NOT HasSnapshot)
        {
            // An established membership can disappear without an adapter-issued Leave:
            // service Advance and owner-side removal are authoritative queue outcomes.
            // Release only this adapter's episode and routing state, so a later queue join is
            // not rejected as a conflicting stale adapter.
            ck_crowd_queue_adapter_processor::StopOwnedEpisode(Agent, InAdapter);
            InAdapter._Queue = {};
            InAdapter._JoinPending = false;
            InAdapter._PendingQueueRevision = 0;
            InAdapter._IssuedQueueAssignmentRevision = 0;
            InAdapter._IssuedCrowdCorrelationId = 0;
            InAdapter._ReportedQueueAssignmentRevision = 0;
            return;
        }

        const auto MoverMatches = HasSnapshot && Snapshot.Get_Mover() == FCk_Handle{InAgent};
        const auto IsMovingToAssignment = HasSnapshot
            && (Snapshot.Get_State() == ECk_Queue_MemberState::Assigned
                || Snapshot.Get_State() == ECk_Queue_MemberState::MovingToSlot);
        const auto IsClaimedAssignment = HasSnapshot
            && (Snapshot.Get_State() == ECk_Queue_MemberState::AtSlot
                || Snapshot.Get_State() == ECk_Queue_MemberState::AtFront);
        const auto CanOwnAssignment = HasSnapshot
            && MoverMatches
            && NOT LeaveRequested
            && NOT Snapshot.Get_MovementSuppressed()
            && Snapshot.Get_AssignmentRevision() > 0
            && (IsMovingToAssignment || IsClaimedAssignment);

        if (NOT CanOwnAssignment)
        {
            ck_crowd_queue_adapter_processor::StopOwnedEpisode(Agent, InAdapter);
            InAdapter._IssuedQueueAssignmentRevision = 0;
            InAdapter._IssuedCrowdCorrelationId = 0;
            InAdapter._ReportedQueueAssignmentRevision = 0;
            return;
        }

        const auto TargetLocation = Snapshot.Get_TargetWorldTransform().GetLocation();
        const auto CurrentLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(
            UCk_Utils_Transform_UE::CastChecked(Agent));
        const auto DistanceToTarget = FVector::Dist(CurrentLocation, TargetLocation);
        const auto ReacquireRadius = UCk_Utils_Queue_UE::Get_SlotReacquireRadiusUu(InAdapter._Queue);

        if (InAdapter._IssuedQueueAssignmentRevision == Snapshot.Get_AssignmentRevision())
        {
            const auto ActiveCorrelation = UCk_Utils_CrowdAgent_UE::Get_ActiveMoveCorrelationId(Agent);
            const auto ActiveGoalMatches = UCk_Utils_CrowdAgent_UE::Get_ActiveGoal(Agent).Equals(
                TargetLocation,
                1.0f);
            const auto MovementState = UCk_Utils_CrowdAgent_UE::Get_MovementState(Agent);
            const auto OwnedEpisodeIsBlocked = UCk_Utils_CrowdAgent_UE::Get_IsGoalBlocked(Agent);
            const auto OwnedEpisodeIsFailedHeld = UCk_Utils_CrowdAgent_UE::Get_IsGoalFailedHold(Agent);
            const auto OwnedEpisodeIsTerminal = UCk_Utils_CrowdAgent_UE::Get_HasReachedActiveGoal(Agent)
                || OwnedEpisodeIsBlocked
                || OwnedEpisodeIsFailedHeld;
            const auto OwnedEpisodeIsIntact = InAdapter._IssuedCrowdCorrelationId > 0
                && ActiveCorrelation == InAdapter._IssuedCrowdCorrelationId
                && ActiveGoalMatches
                && (MovementState != ECk_CrowdAgent_MovementState::Idle || OwnedEpisodeIsTerminal);
            if (OwnedEpisodeIsIntact)
            {
                const auto ClaimedAndDisplaced = IsClaimedAssignment
                    && MovementState == ECk_CrowdAgent_MovementState::Idle
                    && DistanceToTarget > ReacquireRadius
                    && NOT OwnedEpisodeIsBlocked
                    && NOT OwnedEpisodeIsFailedHeld;
                if (NOT ClaimedAndDisplaced)
                { return; }
            }
        }

        // A claimed member inside its station-keeping hysteresis needs no active Crowd episode. If
        // another consumer owns movement, reclaim only after it actually displaces the member from
        // the slot; service Advance removes membership before issuing its exit movement.
        if (IsClaimedAssignment && DistanceToTarget <= ReacquireRadius)
        { return; }

        if (IsClaimedAssignment)
        {
            // Crossing the reacquire radius is the explicit ownership boundary: station keeping is
            // now taking over even if another consumer authored the active episode. Stop first so
            // HandleRequests zeros its outward desired velocity; a bare replacement MoveTo preserves
            // momentum by design and can otherwise send the agent around a wide return arc.
            UCk_Utils_CrowdAgent_UE::Request_Stop(Agent, {});
        }
        // An ordinary unclaimed assignment revision is a retarget, not a terminal movement
        // transition. MoveTo deliberately preserves momentum across back-to-back goals; inserting
        // Stop here creates a visible Stop -> PathPending -> accelerate pulse for every queue reflow.
        InAdapter._IssuedQueueAssignmentRevision = 0;
        InAdapter._IssuedCrowdCorrelationId = 0;
        InAdapter._ReportedQueueAssignmentRevision = 0;
        InAdapter._JoinPending = false;
        InAdapter._PendingQueueRevision = 0;
        const auto Correlation = ck_crowd_queue_adapter_processor::NextNonZeroCorrelation(
            InAdapter._NextCrowdCorrelationId);
        auto MoveTo = FCk_Request_CrowdAgent_MoveTo{TargetLocation};
        MoveTo.Set_CorrelationId(Correlation)
            .Set_ArrivalRadiusOverrideMode(ECk_Override::Override)
            .Set_ArrivalRadiusOverrideValue(
                UCk_Utils_Queue_UE::Get_SlotSettleRadiusUu(InAdapter._Queue));
        UCk_Utils_CrowdAgent_UE::Request_MoveTo(Agent, MoveTo, {});

        InAdapter._IssuedQueueAssignmentRevision = Snapshot.Get_AssignmentRevision();
        InAdapter._IssuedCrowdCorrelationId = Correlation;
        // Station keeping is a new Crowd episode for the SAME claimed assignment, not another
        // semantic arrival. Preserve the reported revision so ObserveOutcome cannot emit a second
        // SlotReached merely because push-apart displaced the member.
        InAdapter._ReportedQueueAssignmentRevision = IsClaimedAssignment
            ? Snapshot.Get_AssignmentRevision()
            : 0;
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
        if (NOT UCk_Utils_Queue_UE::Get_CanAcceptRequests(InAdapter._Queue))
        {
            ck_crowd_queue_adapter_processor::StopOwnedEpisode(Agent, InAdapter);
            InAdapter._Queue = {};
            InAdapter._JoinPending = false;
            InAdapter._PendingQueueRevision = 0;
            InAdapter._IssuedQueueAssignmentRevision = 0;
            InAdapter._IssuedCrowdCorrelationId = 0;
            InAdapter._ReportedQueueAssignmentRevision = 0;
            return;
        }

        auto Snapshot = FCk_Queue_MemberSnapshot{};
        if (NOT ck_crowd_queue_adapter_processor::GetCurrentSnapshot(Agent, InAdapter, Snapshot)
            || Snapshot.Get_Mover() != FCk_Handle{InAgent}
            || Snapshot.Get_MovementSuppressed()
            || Snapshot.Get_AssignmentRevision() != InAdapter._IssuedQueueAssignmentRevision
            || InAdapter._IssuedQueueAssignmentRevision <= 0
            || InAdapter._ReportedQueueAssignmentRevision == InAdapter._IssuedQueueAssignmentRevision
            || UCk_Utils_CrowdAgent_UE::Get_ActiveMoveCorrelationId(Agent) != InAdapter._IssuedCrowdCorrelationId)
        { return; }

        const auto IsClaimedAssignment = Snapshot.Get_State() == ECk_Queue_MemberState::AtSlot
            || Snapshot.Get_State() == ECk_Queue_MemberState::AtFront;
        if (IsClaimedAssignment)
        {
            InAdapter._ReportedQueueAssignmentRevision = Snapshot.Get_AssignmentRevision();
            return;
        }

        const auto IsMovingToAssignment = Snapshot.Get_State() == ECk_Queue_MemberState::Assigned
            || Snapshot.Get_State() == ECk_Queue_MemberState::MovingToSlot;
        if (NOT IsMovingToAssignment)
        { return; }

        const auto CurrentLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(
            UCk_Utils_Transform_UE::CastChecked(Agent));
        const auto DistanceToTarget = FVector::Dist(
            CurrentLocation,
            Snapshot.Get_TargetWorldTransform().GetLocation());
        const auto ClaimRadius = UCk_Utils_Queue_UE::Get_SlotClaimRadiusUu(InAdapter._Queue);

        auto Outcome = TOptional<ECk_Queue_MovementOutcome>{};
        if (DistanceToTarget <= ClaimRadius)
        { Outcome = ECk_Queue_MovementOutcome::Reached; }
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
        if (NOT ck_crowd_queue_adapter_processor::GetCurrentSnapshot(Agent, InAdapter, Snapshot)
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
        ck_crowd_queue_adapter_processor::StopOwnedEpisode(Agent, InAdapter);
        InAdapter._IssuedQueueAssignmentRevision = 0;
        InAdapter._IssuedCrowdCorrelationId = 0;
        InAdapter._ReportedQueueAssignmentRevision = 0;
    }
}
