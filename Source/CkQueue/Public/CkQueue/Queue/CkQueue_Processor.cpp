#include "CkQueue/Queue/CkQueue_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkQueue/CkQueue_Log.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Queue_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Queue_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Queue_Reconcile);
CK_REGISTER_PROCESSOR(ck::FProcessor_Queue_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Queue_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_queue_processor
{
    auto
    IsClaimed(
        const FCk_Queue_MemberSnapshot& InMember)
        -> bool
    {
        return InMember.Get_State() == ECk_Queue_MemberState::AtFront
            || InMember.Get_State() == ECk_Queue_MemberState::AtSlot;
    }

    auto
    InvalidateLaterClaims(
        TArray<FCk_Queue_MemberSnapshot>& InOutMembers,
        const FCk_Queue_MemberSnapshot& InRemovedOrUnavailable)
        -> void
    {
        if (NOT IsClaimed(InRemovedOrUnavailable)) { return; }

        for (auto& Member : InOutMembers)
        {
            if (NOT IsClaimed(Member)
                || Member.Get_Rank() <= InRemovedOrUnavailable.Get_Rank())
            { continue; }

            Member = FCk_Queue_MemberSnapshot{
                Member.Get_Member(), Member.Get_Mover(), Member.Get_Ticket(),
                INDEX_NONE, FTransform::Identity, 0,
                Member.Get_MovementSuppressed(), ECk_Queue_MemberState::PendingAdmission};
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Queue_Setup::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue& InQueueComp)
        -> void
    {
        InQueueComp._LayoutAlgorithm = InParams.Get_LayoutAlgorithm();
        const auto QueueTransform = UCk_Utils_Transform_UE::Cast(InQueue);
        InQueueComp._LastOwnerWorldTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(QueueTransform);
        InQueueComp._State = ECk_Queue_State::Ready;
        InQueueComp._Revision = 1;
        InQueueComp._Pressure = FCk_Queue_Pressure{
            0,
            InParams.Get_SoftLimit(),
            InParams.Get_HardLimit(),
            false,
            false,
            InQueueComp._Revision};

        InQueue.Remove<MarkedDirtyBy>();

        UUtils_Signal_OnQueueFormationStateChanged::Broadcast(
            InQueue,
            MakePayload(
                InQueue,
                FCk_Queue_FormationState{
                    InQueueComp._State,
                    ECk_Queue_EventReason::None,
                    InQueueComp._Revision,
                    InQueueComp._RetryEpisode}));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Queue_HandleRequests::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue& InQueueComp,
            FFragment_Queue_Requests& InRequests)
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InQueue, Result);

            if (DoHandleRequest(InQueue, InParams, InQueueComp, InRequest))
            { Result = ECk_Request_OperationResult::Succeeded; }
        }), policy::DontResetContainer{});

        if (InQueueComp._State == ECk_Queue_State::WaitingForFormation)
        { InvalidateAssignmentsForReflow(InQueue, InParams, InQueueComp); }

        if (InRequests._Requests.IsEmpty())
        { InQueue.Remove<MarkedDirtyBy>(); }
    }

    auto
        FProcessor_Queue_HandleRequests::
        TryApplyReachedClaim(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue& InQueueComp,
            int32 InMemberIndex,
            TOptional<int32> InExpectedAssignmentRevision)
        -> bool
    {
        if (NOT InQueueComp._Members.IsValidIndex(InMemberIndex)
            || InQueueComp._State != ECk_Queue_State::Ready)
        { return false; }

        const auto Previous = InQueueComp._Members[InMemberIndex];
        const auto HasExpectedAssignment = NOT InExpectedAssignmentRevision.IsSet()
            || Previous.Get_AssignmentRevision() == InExpectedAssignmentRevision.GetValue();
        const auto HasClaimableAssignment = HasExpectedAssignment
            && Previous.Get_AssignmentRevision() > 0
            && Previous.Get_Rank() != INDEX_NONE
            && NOT Previous.Get_MovementSuppressed()
            && (Previous.Get_State() == ECk_Queue_MemberState::Assigned
                || Previous.Get_State() == ECk_Queue_MemberState::MovingToSlot);
        if (NOT HasClaimableAssignment)
        { return false; }

        const auto ClaimAlreadyExists = InQueueComp._Members.ContainsByPredicate(
            [&Previous](const FCk_Queue_MemberSnapshot& InMember)
            {
                return ck_queue_processor::IsClaimed(InMember)
                    && InMember.Get_Rank() == Previous.Get_Rank();
            });
        // Queue requests drain serially. The first current-revision arrival claims the provisional slot;
        // same-drain contenders that arrive after it succeed as no-ops and are retargeted by the reflow it opens.
        if (ClaimAlreadyExists)
        { return true; }

        ++InQueueComp._Revision;
        InQueueComp._Members[InMemberIndex] = FCk_Queue_MemberSnapshot{
            Previous.Get_Member(),
            Previous.Get_Mover(),
            Previous.Get_Ticket(),
            Previous.Get_Rank(),
            Previous.Get_TargetWorldTransform(),
            Previous.Get_AssignmentRevision(),
            Previous.Get_MovementSuppressed(),
            Previous.Get_Rank() == 0
                ? ECk_Queue_MemberState::AtFront
                : ECk_Queue_MemberState::AtSlot};

        BroadcastMemberEvent(
            InQueue,
            InQueueComp._Members[InMemberIndex],
            Previous.Get_State(),
            ECk_Queue_EventReason::SlotReached,
            InQueueComp._Revision);
        if (InParams.Get_SlotClaimPolicy() == ECk_Queue_SlotClaimPolicy::ClaimFirstAvailableOnReach)
        {
            // Keep this request drain Ready: another contender may report the current offer reached this frame.
            // Formation runs after the drain and opens the next offer. The flag is required
            // because the formation processor early-outs on a settled Ready queue — without it the
            // reach event is swallowed and the next member is never offered a slot.
            InQueueComp._HasPendingClaimOffer = true;
            InQueue.AddOrGet<FTag_Queue_NeedsFormation>();
        }
        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Queue_HandleRequests::
        DoHandleRequest(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue& InQueueComp,
            const FCk_Request_Queue_RestoreJoin& InRequest)
        -> bool
    {
        const auto MemberIsValid = ck::IsValid(InRequest.Get_Member());
        const auto TicketIsValid = InRequest.Get_RestoredTicket() > 0
            && InRequest.Get_RestoredTicket() < MAX_int64;
        CK_ENSURE_IF_NOT(MemberIsValid && TicketIsValid,
            TEXT("Queue [{}] cannot restore join member [{}] with ticket [{}]"),
            InQueue, InRequest.Get_Member(), InRequest.Get_RestoredTicket())
        { return false; }

        const auto ExistingIndex = FindMemberIndex(InQueueComp, InRequest.Get_Member());
        const auto TicketOwnerIndex = InQueueComp._Members.IndexOfByPredicate(
            [&InRequest](const FCk_Queue_MemberSnapshot& InMember)
            { return InMember.Get_Ticket() == InRequest.Get_RestoredTicket(); });
        const auto TicketBelongsToAnotherMember = TicketOwnerIndex != INDEX_NONE
            && TicketOwnerIndex != ExistingIndex;
        CK_ENSURE_IF_NOT(NOT TicketBelongsToAnotherMember,
            TEXT("Queue [{}] restore ticket [{}] is already owned by another member"),
            InQueue, InRequest.Get_RestoredTicket())
        { return false; }

        if (ExistingIndex != INDEX_NONE)
        {
            const auto Existing = InQueueComp._Members[ExistingIndex];
            const auto TicketMatches = Existing.Get_Ticket() == InRequest.Get_RestoredTicket();
            CK_ENSURE_IF_NOT(TicketMatches,
                TEXT("Queue [{}] restore member [{}] conflicts with existing ticket [{}]"),
                InQueue, InRequest.Get_Member(), Existing.Get_Ticket())
            { return false; }

            const auto Mover = ck::IsValid(InRequest.Get_Mover())
                ? InRequest.Get_Mover()
                : Existing.Get_Mover();
            const auto WaitingForMoverNeedsReplacement = Existing.Get_State() == ECk_Queue_MemberState::WaitingForMover
                && NOT ck::IsValid(InRequest.Get_Mover());
            if (WaitingForMoverNeedsReplacement)
            { return false; }
            if (Mover == Existing.Get_Mover())
            { return true; }

            ++InQueueComp._Revision;
            InQueueComp._Members[ExistingIndex] = FCk_Queue_MemberSnapshot{
                Existing.Get_Member(), Mover, Existing.Get_Ticket(), INDEX_NONE,
                FTransform::Identity, 0, Existing.Get_MovementSuppressed(), ECk_Queue_MemberState::PendingAdmission};
            MarkFormationDirty(InQueue, InQueueComp, ECk_Queue_EventReason::Rejoined);
            BroadcastMemberEvent(InQueue, InQueueComp._Members[ExistingIndex], Existing.Get_State(),
                ECk_Queue_EventReason::Rejoined, InQueueComp._Revision);
            return true;
        }

        const auto HardLimit = InParams.Get_HardLimit();
        if (HardLimit > 0 && InQueueComp._Members.Num() >= HardLimit)
        {
            ++InQueueComp._Revision;
            const auto Rejected = FCk_Queue_MemberSnapshot{
                InRequest.Get_Member(), InRequest.Get_Mover(), 0, INDEX_NONE,
                FTransform::Identity, 0, false, ECk_Queue_MemberState::Rejected};
            BroadcastMemberEvent(InQueue, Rejected, ECk_Queue_MemberState::None,
                ECk_Queue_EventReason::HardLimitReached, InQueueComp._Revision);
            return false;
        }

        ++InQueueComp._Revision;
        InQueueComp._Members.Emplace(InRequest.Get_Member(), InRequest.Get_Mover(), InRequest.Get_RestoredTicket(),
            0, FTransform::Identity, 0, false, ECk_Queue_MemberState::PendingAdmission);
        InQueueComp._Members.Sort([](const FCk_Queue_MemberSnapshot& InA, const FCk_Queue_MemberSnapshot& InB)
        { return InA.Get_Ticket() < InB.Get_Ticket(); });
        InQueueComp._NextTicket = FMath::Max(InQueueComp._NextTicket, InRequest.Get_RestoredTicket() + 1);
        MarkFormationDirty(InQueue, InQueueComp, ECk_Queue_EventReason::Joined);
        RefreshPressure(InQueue, InParams, InQueueComp);

        const auto RestoredIndex = FindMemberIndex(InQueueComp, InRequest.Get_Member());
        BroadcastMemberEvent(InQueue, InQueueComp._Members[RestoredIndex], ECk_Queue_MemberState::None,
            ECk_Queue_EventReason::Joined, InQueueComp._Revision);
        return true;
    }

    auto
        FProcessor_Queue_HandleRequests::
        DoHandleRequest(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue& InQueueComp,
            const FCk_Request_Queue_Join& InRequest)
        -> bool
    {
        const auto MemberIsValid = ck::IsValid(InRequest.Get_Member());
        CK_ENSURE_IF_NOT(MemberIsValid,
            TEXT("Queue [{}] cannot join invalid member [{}]"),
            InQueue,
            InRequest.Get_Member())
        { return false; }

        const auto ExistingIndex = FindMemberIndex(InQueueComp, InRequest.Get_Member());
        if (ExistingIndex != INDEX_NONE)
        {
            const auto Existing = InQueueComp._Members[ExistingIndex];
            const auto Mover = ck::IsValid(InRequest.Get_Mover())
                ? InRequest.Get_Mover()
                : Existing.Get_Mover();

            const auto WaitingForMoverNeedsReplacement = Existing.Get_State() == ECk_Queue_MemberState::WaitingForMover
                && NOT ck::IsValid(InRequest.Get_Mover());
            if (WaitingForMoverNeedsReplacement)
            { return false; }

            if (Mover == Existing.Get_Mover())
            { return true; }

            ++InQueueComp._Revision;
            InQueueComp._Members[ExistingIndex] = FCk_Queue_MemberSnapshot{
                Existing.Get_Member(),
                Mover,
                Existing.Get_Ticket(),
                Existing.Get_Rank(),
                FTransform::Identity,
                0,
                Existing.Get_MovementSuppressed(),
                ECk_Queue_MemberState::PendingAdmission};

            MarkFormationDirty(InQueue, InQueueComp, ECk_Queue_EventReason::Rejoined);

            BroadcastMemberEvent(
                InQueue,
                InQueueComp._Members[ExistingIndex],
                Existing.Get_State(),
                ECk_Queue_EventReason::Rejoined,
                InQueueComp._Revision);
            return true;
        }

        const auto HardLimit = InParams.Get_HardLimit();
        if (HardLimit > 0 && InQueueComp._Members.Num() >= HardLimit)
        {
            ++InQueueComp._Revision;
            const auto Rejected = FCk_Queue_MemberSnapshot{
                InRequest.Get_Member(),
                InRequest.Get_Mover(),
                0,
                INDEX_NONE,
                FTransform::Identity,
                0,
                false,
                ECk_Queue_MemberState::Rejected};

            BroadcastMemberEvent(
                InQueue,
                Rejected,
                ECk_Queue_MemberState::None,
                ECk_Queue_EventReason::HardLimitReached,
                InQueueComp._Revision);
            return false;
        }

        const auto TicketIsAvailable = InQueueComp._NextTicket > 0
            && InQueueComp._NextTicket < MAX_int64;
        CK_ENSURE_IF_NOT(TicketIsAvailable,
            TEXT("Queue [{}] cannot join member [{}]: admission ticket space is exhausted"),
            InQueue, InRequest.Get_Member())
        { return false; }

        const auto Ticket = InQueueComp._NextTicket;
        InQueueComp._NextTicket = Ticket + 1;
        const auto Rank = InQueueComp._Members.Num();

        ++InQueueComp._Revision;

        InQueueComp._Members.Emplace(
            InRequest.Get_Member(),
            InRequest.Get_Mover(),
            Ticket,
            Rank,
            FTransform::Identity,
            0,
            false,
            ECk_Queue_MemberState::PendingAdmission);

        MarkFormationDirty(InQueue, InQueueComp, ECk_Queue_EventReason::Joined);
        RefreshPressure(InQueue, InParams, InQueueComp);
        BroadcastMemberEvent(
            InQueue,
            InQueueComp._Members.Last(),
            ECk_Queue_MemberState::None,
            ECk_Queue_EventReason::Joined,
            InQueueComp._Revision);
        return true;
    }

    auto
        FProcessor_Queue_HandleRequests::
        DoHandleRequest(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue& InQueueComp,
            const FCk_Request_Queue_Leave& InRequest)
        -> bool
    {
        const auto MemberIndex = FindMemberIndex(InQueueComp, InRequest.Get_Member());
        if (MemberIndex == INDEX_NONE)
        { return true; }

        const auto Removed = InQueueComp._Members[MemberIndex];
        InQueueComp._Members.RemoveAt(MemberIndex);
        ++InQueueComp._Revision;

        const auto ClaimOnReach = InParams.Get_SlotClaimPolicy() == ECk_Queue_SlotClaimPolicy::ClaimFirstAvailableOnReach;
        if (ClaimOnReach) { ck_queue_processor::InvalidateLaterClaims(InQueueComp._Members, Removed); }

        const auto RemovedSnapshot = FCk_Queue_MemberSnapshot{
            Removed.Get_Member(),
            Removed.Get_Mover(),
            Removed.Get_Ticket(),
            Removed.Get_Rank(),
            Removed.Get_TargetWorldTransform(),
            Removed.Get_AssignmentRevision(),
            false,
            ECk_Queue_MemberState::None};

        MarkFormationDirty(InQueue, InQueueComp, InRequest.Get_Reason());
        if (NOT ClaimOnReach) { RebuildRanks(InQueue, InQueueComp, ECk_Queue_EventReason::Reflowed); }
        RefreshPressure(InQueue, InParams, InQueueComp);
        BroadcastMemberEvent(
            InQueue,
            RemovedSnapshot,
            Removed.Get_State(),
            InRequest.Get_Reason(),
            InQueueComp._Revision);
        return true;
    }

    auto
        FProcessor_Queue_HandleRequests::
        DoHandleRequest(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue& InQueueComp,
            const FCk_Request_Queue_Advance& /*InRequest*/)
        -> bool
    {
        const auto MemberIndex = InQueueComp._Members.IndexOfByPredicate(
        [&](const FCk_Queue_MemberSnapshot& InMember)
        {
            return InMember.Get_Rank() == 0
                && InMember.Get_State() == ECk_Queue_MemberState::AtFront;
        });

        if (MemberIndex == INDEX_NONE)
        { return false; }

        const auto Removed = InQueueComp._Members[MemberIndex];
        InQueueComp._Members.RemoveAt(MemberIndex);
        ++InQueueComp._Revision;

        const auto ClaimOnReach = InParams.Get_SlotClaimPolicy() == ECk_Queue_SlotClaimPolicy::ClaimFirstAvailableOnReach;
        if (ClaimOnReach) { ck_queue_processor::InvalidateLaterClaims(InQueueComp._Members, Removed); }

        const auto Serving = FCk_Queue_MemberSnapshot{
            Removed.Get_Member(),
            Removed.Get_Mover(),
            Removed.Get_Ticket(),
            Removed.Get_Rank(),
            Removed.Get_TargetWorldTransform(),
            Removed.Get_AssignmentRevision(),
            false,
            ECk_Queue_MemberState::Serving};

        MarkFormationDirty(InQueue, InQueueComp, ECk_Queue_EventReason::Advanced);
        if (NOT ClaimOnReach) { RebuildRanks(InQueue, InQueueComp, ECk_Queue_EventReason::Advanced); }
        RefreshPressure(InQueue, InParams, InQueueComp);
        BroadcastMemberEvent(
            InQueue,
            Serving,
            Removed.Get_State(),
            ECk_Queue_EventReason::Advanced,
            InQueueComp._Revision);
        return true;
    }

    auto
    FProcessor_Queue_HandleRequests::
        RebuildRanks(
            HandleType InQueue,
            FFragment_Queue& InQueueComp,
            ECk_Queue_EventReason InReason)
        -> void
    {
        auto RankedMemberIndices = TArray<int32>{};
        for (auto MemberIndex = 0; MemberIndex < InQueueComp._Members.Num(); ++MemberIndex)
        {
            const auto& Member = InQueueComp._Members[MemberIndex];
            if (Member.Get_Rank() == INDEX_NONE
                || Member.Get_State() == ECk_Queue_MemberState::WaitingForMover
                || Member.Get_State() == ECk_Queue_MemberState::WaitingForNavigationChange)
            { continue; }

            RankedMemberIndices.Add(MemberIndex);
        }

        RankedMemberIndices.Sort([&Members = InQueueComp._Members](int32 InLeftIndex, int32 InRightIndex)
        {
            const auto& Left = Members[InLeftIndex];
            const auto& Right = Members[InRightIndex];
            if (Left.Get_Rank() != Right.Get_Rank())
            { return Left.Get_Rank() < Right.Get_Rank(); }
            return Left.Get_Ticket() < Right.Get_Ticket();
        });

        for (auto NextRank = 0; NextRank < RankedMemberIndices.Num(); ++NextRank)
        {
            const auto MemberIndex = RankedMemberIndices[NextRank];
            const auto Previous = InQueueComp._Members[MemberIndex];
            if (Previous.Get_Rank() == NextRank)
            { continue; }

            InQueueComp._Members[MemberIndex] = FCk_Queue_MemberSnapshot{
                Previous.Get_Member(),
                Previous.Get_Mover(),
                Previous.Get_Ticket(),
                NextRank,
                Previous.Get_TargetWorldTransform(),
                Previous.Get_AssignmentRevision(),
                Previous.Get_MovementSuppressed(),
                Previous.Get_State()};

            BroadcastMemberEvent(
                InQueue,
                InQueueComp._Members[MemberIndex],
                Previous.Get_State(),
                InReason,
                InQueueComp._Revision);
        }
    }

    auto
        FProcessor_Queue_HandleRequests::
        DoHandleRequest(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue& InQueueComp,
            const FCk_Request_Queue_SetLayout& InRequest)
        -> bool
    {
        if (InQueueComp._LayoutAlgorithm == InRequest.Get_LayoutAlgorithm())
        { return true; }

        InQueueComp._LayoutAlgorithm = InRequest.Get_LayoutAlgorithm();
        ++InQueueComp._Revision;
        if (InParams.Get_SlotClaimPolicy() == ECk_Queue_SlotClaimPolicy::ClaimFirstAvailableOnReach)
        {
            for (auto& Member : InQueueComp._Members)
            {
                if (Member.Get_MovementSuppressed()) { continue; }
                Member = FCk_Queue_MemberSnapshot{Member.Get_Member(), Member.Get_Mover(), Member.Get_Ticket(),
                    INDEX_NONE, FTransform::Identity, 0, false, ECk_Queue_MemberState::PendingAdmission};
            }
        }
        MarkFormationDirty(InQueue, InQueueComp, ECk_Queue_EventReason::LayoutChanged);
        return true;
    }

    auto
        FProcessor_Queue_HandleRequests::
        DoHandleRequest(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue& InQueueComp,
            const FCk_Request_Queue_SetMovementSuppressed& InRequest)
        -> bool
    {
        const auto MemberIndex = FindMemberIndex(InQueueComp, InRequest.Get_Member());
        if (MemberIndex == INDEX_NONE)
        { return false; }

        const auto Previous = InQueueComp._Members[MemberIndex];
        const auto Suppressed = InRequest.Get_MovementSuppressed() == ECk_EnableDisable::Enable;
        if (Previous.Get_MovementSuppressed() == Suppressed)
        { return true; }

        ++InQueueComp._Revision;
        const auto State = Suppressed
            ? ECk_Queue_MemberState::MovementSuppressed
            : ECk_Queue_MemberState::PendingAdmission;
        InQueueComp._Members[MemberIndex] = FCk_Queue_MemberSnapshot{
            Previous.Get_Member(),
            Previous.Get_Mover(),
            Previous.Get_Ticket(),
            Previous.Get_Rank(),
            Suppressed ? Previous.Get_TargetWorldTransform() : FTransform::Identity,
            0,
            Suppressed,
            State};

        if (NOT Suppressed)
        { MarkFormationDirty(InQueue, InQueueComp, ECk_Queue_EventReason::MovementResumed); }

        BroadcastMemberEvent(
            InQueue,
            InQueueComp._Members[MemberIndex],
            Previous.Get_State(),
            Suppressed
                ? ECk_Queue_EventReason::MovementSuppressed
                : ECk_Queue_EventReason::MovementResumed,
            InQueueComp._Revision);
        return true;
    }

    auto
        FProcessor_Queue_HandleRequests::
        DoHandleRequest(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue& InQueueComp,
            const FCk_Request_Queue_ReportMovementOutcome& InRequest)
        -> bool
    {
        const auto MemberIndex = FindMemberIndex(InQueueComp, InRequest.Get_Member());
        if (MemberIndex == INDEX_NONE)
        { return false; }

        const auto Previous = InQueueComp._Members[MemberIndex];
        if (Previous.Get_AssignmentRevision() != InRequest.Get_AssignmentRevision()
            || Previous.Get_MovementSuppressed()
            || InQueueComp.Get_State() != ECk_Queue_State::Ready)
        { return true; }

        const auto HasIssuedAssignment = Previous.Get_AssignmentRevision() > 0
            && (Previous.Get_State() == ECk_Queue_MemberState::Assigned
                || Previous.Get_State() == ECk_Queue_MemberState::MovingToSlot);
        if (NOT HasIssuedAssignment)
        { return false; }

        if (InRequest.Get_Outcome() == ECk_Queue_MovementOutcome::Reached)
        {
            return TryApplyReachedClaim(
                InQueue,
                InParams,
                InQueueComp,
                MemberIndex,
                TOptional<int32>{InRequest.Get_AssignmentRevision()});
        }

        auto State = Previous.Get_State();
        auto Reason = ECk_Queue_EventReason::None;

        switch (InRequest.Get_Outcome())
        {
            case ECk_Queue_MovementOutcome::Failed:
            {
                // A failed mover no longer owns a reservation. Keep its ticket, but move it behind viable
                // members and wait for a nav generation change before it can re-enter formation.
                ++InQueueComp._Revision;
                const auto Relinquished = FCk_Queue_MemberSnapshot{
                    Previous.Get_Member(),
                    Previous.Get_Mover(),
                    Previous.Get_Ticket(),
                    INDEX_NONE,
                    FTransform::Identity,
                    0,
                    Previous.Get_MovementSuppressed(),
                    ECk_Queue_MemberState::WaitingForNavigationChange};
                InQueueComp._Members.RemoveAt(MemberIndex);
                InQueueComp._Members.Add(Relinquished);
                BroadcastMemberEvent(
                    InQueue,
                    Relinquished,
                    Previous.Get_State(),
                    ECk_Queue_EventReason::MovementFailed,
                    InQueueComp._Revision);
                RefreshPressure(InQueue, InParams, InQueueComp);
                MarkFormationDirty(InQueue, InQueueComp, ECk_Queue_EventReason::MovementFailed);
                return true;
            }
            case ECk_Queue_MovementOutcome::Cancelled:
            {
                State = ECk_Queue_MemberState::Assigned;
                Reason = ECk_Queue_EventReason::MovementCancelled;
                break;
            }
            default:
            {
                CK_INVALID_ENUM(InRequest.Get_Outcome());
                return false;
            }
        }

        ++InQueueComp._Revision;
        InQueueComp._Members[MemberIndex] = FCk_Queue_MemberSnapshot{
            Previous.Get_Member(),
            Previous.Get_Mover(),
            Previous.Get_Ticket(),
            Previous.Get_Rank(),
            Previous.Get_TargetWorldTransform(),
            Previous.Get_AssignmentRevision(),
            Previous.Get_MovementSuppressed(),
            State};

        BroadcastMemberEvent(
            InQueue,
            InQueueComp._Members[MemberIndex],
            Previous.Get_State(),
            Reason,
            InQueueComp._Revision);
        return true;
    }

    auto
        FProcessor_Queue_HandleRequests::
        FindMemberIndex(
            const FFragment_Queue& InQueue,
            const FCk_Handle& InMember)
        -> int32
    {
        return InQueue.Get_Members().IndexOfByPredicate(
        [&](const FCk_Queue_MemberSnapshot& InSnapshot)
        {
            return InSnapshot.Get_Member() == InMember;
        });
    }

    auto
        FProcessor_Queue_HandleRequests::
        InvalidateAssignmentsForReflow(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue& InQueueComp)
        -> void
    {
        // Reserve assignments are the input to the next atomic compaction. WaitingForFormation prevents
        // adapters and movement outcomes from consuming them in the meantime; Formation validates live
        // movers and publishes the complete replacement mapping later in this same processor group.
        if (InParams.Get_SlotClaimPolicy() == ECk_Queue_SlotClaimPolicy::ReserveOnFormation)
        { return; }

        for (auto MemberIndex = 0; MemberIndex < InQueueComp._Members.Num(); ++MemberIndex)
        {
            const auto Previous = InQueueComp._Members[MemberIndex];
            if (Previous.Get_MovementSuppressed()
                || Previous.Get_State() == ECk_Queue_MemberState::WaitingForNavigationChange
                || Previous.Get_State() == ECk_Queue_MemberState::WaitingForMover
                || (InParams.Get_SlotClaimPolicy() == ECk_Queue_SlotClaimPolicy::ClaimFirstAvailableOnReach
                    && (Previous.Get_State() == ECk_Queue_MemberState::AtFront
                        || Previous.Get_State() == ECk_Queue_MemberState::AtSlot))
                || (Previous.Get_AssignmentRevision() == 0
                    && Previous.Get_State() == ECk_Queue_MemberState::PendingAdmission))
            { continue; }

            InQueueComp._Members[MemberIndex] = FCk_Queue_MemberSnapshot{
                Previous.Get_Member(),
                Previous.Get_Mover(),
                Previous.Get_Ticket(),
                Previous.Get_Rank(),
                FTransform::Identity,
                0,
                false,
                ECk_Queue_MemberState::PendingAdmission};

            BroadcastMemberEvent(
                InQueue,
                InQueueComp._Members[MemberIndex],
                Previous.Get_State(),
                ECk_Queue_EventReason::Reflowed,
                InQueueComp._Revision);
        }
    }

    auto
        FProcessor_Queue_HandleRequests::
        RefreshPressure(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue& InQueueComp)
        -> void
    {
        const auto Count = InQueueComp._Members.Num();
        const auto SoftLimited = InParams.Get_SoftLimit() > 0 && Count >= InParams.Get_SoftLimit();
        const auto HardLimited = InParams.Get_HardLimit() > 0 && Count >= InParams.Get_HardLimit();

        InQueueComp._Pressure = FCk_Queue_Pressure{
            Count,
            InParams.Get_SoftLimit(),
            InParams.Get_HardLimit(),
            SoftLimited,
            HardLimited,
            InQueueComp._Revision};

        UUtils_Signal_OnQueuePressureChanged::Broadcast(
            InQueue,
            MakePayload(InQueue, InQueueComp._Pressure));
    }

    auto
        FProcessor_Queue_HandleRequests::
        BroadcastMemberEvent(
            HandleType InQueue,
            const FCk_Queue_MemberSnapshot& InSnapshot,
            ECk_Queue_MemberState InPreviousState,
            ECk_Queue_EventReason InReason,
            int32 InRevision)
        -> void
    {
        UUtils_Signal_OnQueueMemberStateChanged::Broadcast(
            InQueue,
            MakePayload(
                InQueue,
                FCk_Queue_MemberEvent{
                    InQueue,
                    InSnapshot,
                    InPreviousState,
                    InReason,
                    InRevision}));
    }

    auto
    FProcessor_Queue_HandleRequests::
    MarkFormationDirty(
            HandleType InQueue,
            FFragment_Queue& InQueueComp,
            ECk_Queue_EventReason InReason)
        -> void
    {
        InQueueComp._State = InQueueComp._Members.IsEmpty()
            ? ECk_Queue_State::Ready
            : ECk_Queue_State::WaitingForFormation;

        if (InQueueComp._Members.IsEmpty())
        { InQueue.Try_Remove<FTag_Queue_NeedsFormation>(); }
        else
        { InQueue.AddOrGet<FTag_Queue_NeedsFormation>(); }

        UUtils_Signal_OnQueueFormationStateChanged::Broadcast(
            InQueue,
            MakePayload(
                InQueue,
                FCk_Queue_FormationState{
                    InQueueComp._State,
                    InReason,
                    InQueueComp._Revision,
                    InQueueComp._RetryEpisode}));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Queue_Reconcile::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue& InQueueComp)
        -> void
    {
        const auto QueueTransform = UCk_Utils_Transform_UE::Cast(InQueue);
        const auto OwnerWorldTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(QueueTransform);
        const auto LocationMoved = FVector::DistSquared(
            OwnerWorldTransform.GetLocation(),
            InQueueComp._LastOwnerWorldTransform.GetLocation())
            > FMath::Square(InParams.Get_TransformEpsilonUu());
        const auto RotationMoved = OwnerWorldTransform.GetRotation().AngularDistance(
            InQueueComp._LastOwnerWorldTransform.GetRotation())
            > FMath::DegreesToRadians(InParams.Get_RotationEpsilonDegrees());
        const auto ScaleChanged = NOT OwnerWorldTransform.GetScale3D().Equals(
            InQueueComp._LastOwnerWorldTransform.GetScale3D(),
            KINDA_SMALL_NUMBER);
        const auto OwnerMoved = LocationMoved || RotationMoved || ScaleChanged;

        if (OwnerMoved)
        {
            InQueueComp._LastOwnerWorldTransform = OwnerWorldTransform;
            if (NOT InQueueComp._Members.IsEmpty())
            {
                ++InQueueComp._Revision;

                const auto PreserveReserveAssignments = InParams.Get_SlotClaimPolicy()
                    == ECk_Queue_SlotClaimPolicy::ReserveOnFormation;
                if (NOT PreserveReserveAssignments)
                {
                    for (auto MemberIndex = 0; MemberIndex < InQueueComp._Members.Num(); ++MemberIndex)
                    {
                        const auto Previous = InQueueComp._Members[MemberIndex];
                        if (Previous.Get_MovementSuppressed())
                        { continue; }

                        InQueueComp._Members[MemberIndex] = FCk_Queue_MemberSnapshot{
                            Previous.Get_Member(),
                            Previous.Get_Mover(),
                            Previous.Get_Ticket(),
                            Previous.Get_Rank(),
                            FTransform::Identity,
                            0,
                            false,
                            ECk_Queue_MemberState::PendingAdmission};

                        UUtils_Signal_OnQueueMemberStateChanged::Broadcast(
                            InQueue,
                            MakePayload(
                                InQueue,
                                FCk_Queue_MemberEvent{
                                    InQueue,
                                    InQueueComp._Members[MemberIndex],
                                    Previous.Get_State(),
                                    ECk_Queue_EventReason::Reflowed,
                                    InQueueComp._Revision}));
                    }
                }

                InQueueComp._State = ECk_Queue_State::WaitingForFormation;
                InQueue.AddOrGet<FTag_Queue_NeedsFormation>();
                UUtils_Signal_OnQueueFormationStateChanged::Broadcast(
                    InQueue,
                    MakePayload(
                        InQueue,
                        FCk_Queue_FormationState{
                            InQueueComp._State,
                            ECk_Queue_EventReason::Reflowed,
                            InQueueComp._Revision,
                            InQueueComp._RetryEpisode}));
            }
        }

        const auto ReconcileArrivals = InQueueComp._State == ECk_Queue_State::Ready;
        if (ReconcileArrivals)
        {
            // HandleRequests has already consumed explicit outcomes this frame. This stable member
            // iteration therefore only supplies a transform-backed fallback for an unreported arrival.
            for (auto MemberIndex = 0; MemberIndex < InQueueComp._Members.Num(); ++MemberIndex)
            {
                const auto Previous = InQueueComp._Members[MemberIndex];
                const auto Member = Previous.Get_Member();
                const auto MemberIsLive = ck::IsValid(Member) && NOT Member.Has<FTag_DestroyEntity_Initiate>();
                const auto HasCurrentAssignment = MemberIsLive
                    && Previous.Get_AssignmentRevision() > 0
                    && Previous.Get_Rank() != INDEX_NONE
                    && NOT Previous.Get_MovementSuppressed()
                    && (Previous.Get_State() == ECk_Queue_MemberState::Assigned
                        || Previous.Get_State() == ECk_Queue_MemberState::MovingToSlot);
                if (NOT HasCurrentAssignment)
                { continue; }

                const auto Mover = Previous.Get_Mover();
                const auto MoverHasTransform = ck::IsValid(Mover)
                    && NOT Mover.Has<FTag_DestroyEntity_Initiate>()
                    && UCk_Utils_Transform_UE::Has(Mover);
                if (NOT MoverHasTransform || Previous.Get_TargetWorldTransform().ContainsNaN())
                { continue; }

                const auto MoverLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(
                    UCk_Utils_Transform_UE::CastChecked(Mover));
                const auto ReachedCurrentReservation = NOT MoverLocation.ContainsNaN()
                    && FVector::Dist(MoverLocation, Previous.Get_TargetWorldTransform().GetLocation())
                        <= InParams.Get_SlotClaimRadiusUu();
                if (NOT ReachedCurrentReservation)
                { continue; }

                FProcessor_Queue_HandleRequests::TryApplyReachedClaim(
                    InQueue,
                    InParams,
                    InQueueComp,
                    MemberIndex,
                    TOptional<int32>{Previous.Get_AssignmentRevision()});
            }
        }

        auto RemovedMembers = TArray<FCk_Queue_MemberSnapshot>{};
        auto MembersWithDestroyedMovers = TArray<FCk_Handle>{};

        for (auto MemberIndex = InQueueComp._Members.Num() - 1; MemberIndex >= 0; --MemberIndex)
        {
            const auto& Member = InQueueComp._Members[MemberIndex];
            if (ck::IsValid(Member.Get_Member()))
            {
                const auto HadMover = Member.Get_Mover() != FCk_Handle{};
                if (HadMover && ck::Is_NOT_Valid(Member.Get_Mover()))
                { MembersWithDestroyedMovers.Add(Member.Get_Member()); }
                continue;
            }

            RemovedMembers.Emplace(InQueueComp._Members[MemberIndex]);
            InQueueComp._Members.RemoveAt(MemberIndex);
        }

        if (RemovedMembers.IsEmpty() && MembersWithDestroyedMovers.IsEmpty())
        { return; }

        ++InQueueComp._Revision;

        const auto ClaimOnReach = InParams.Get_SlotClaimPolicy() == ECk_Queue_SlotClaimPolicy::ClaimFirstAvailableOnReach;
        if (ClaimOnReach)
        {
            for (const auto& Removed : RemovedMembers)
            { ck_queue_processor::InvalidateLaterClaims(InQueueComp._Members, Removed); }

            for (auto MemberIndex = 0; MemberIndex < InQueueComp._Members.Num(); ++MemberIndex)
            {
                const auto Previous = InQueueComp._Members[MemberIndex];
                if (NOT MembersWithDestroyedMovers.Contains(Previous.Get_Member())) { continue; }

                ck_queue_processor::InvalidateLaterClaims(InQueueComp._Members, Previous);
                InQueueComp._Members[MemberIndex] = FCk_Queue_MemberSnapshot{
                    Previous.Get_Member(), FCk_Handle{}, Previous.Get_Ticket(),
                    INDEX_NONE, FTransform::Identity, 0,
                    Previous.Get_MovementSuppressed(), ECk_Queue_MemberState::WaitingForMover};

                UUtils_Signal_OnQueueMemberStateChanged::Broadcast(
                    InQueue,
                    MakePayload(
                        InQueue,
                        FCk_Queue_MemberEvent{
                            InQueue,
                            InQueueComp._Members[MemberIndex],
                            Previous.Get_State(),
                            ECk_Queue_EventReason::MovementFailed,
                            InQueueComp._Revision}));
            }
        }
        else
        {
            auto NextRank = 0;
            for (auto MemberIndex = 0; MemberIndex < InQueueComp._Members.Num(); ++MemberIndex)
            {
                const auto Previous = InQueueComp._Members[MemberIndex];
                const auto MoverWasDestroyed = MembersWithDestroyedMovers.Contains(Previous.Get_Member());
                const auto Rank = MoverWasDestroyed ? INDEX_NONE : NextRank++;
                InQueueComp._Members[MemberIndex] = FCk_Queue_MemberSnapshot{
                    Previous.Get_Member(),
                    MoverWasDestroyed ? FCk_Handle{} : Previous.Get_Mover(),
                    Previous.Get_Ticket(),
                    Rank,
                    MoverWasDestroyed ? FTransform::Identity : Previous.Get_TargetWorldTransform(),
                    MoverWasDestroyed ? 0 : Previous.Get_AssignmentRevision(),
                    Previous.Get_MovementSuppressed(),
                    MoverWasDestroyed ? ECk_Queue_MemberState::WaitingForMover : Previous.Get_State()};

                if (MoverWasDestroyed)
                {
                    UUtils_Signal_OnQueueMemberStateChanged::Broadcast(
                        InQueue,
                        MakePayload(
                            InQueue,
                            FCk_Queue_MemberEvent{
                                InQueue,
                                InQueueComp._Members[MemberIndex],
                                Previous.Get_State(),
                                ECk_Queue_EventReason::MovementFailed,
                                InQueueComp._Revision}));
                }
            }
        }

        InQueueComp._State = InQueueComp._Members.IsEmpty()
            ? ECk_Queue_State::Ready
            : ECk_Queue_State::WaitingForFormation;

        if (InQueueComp._Members.IsEmpty())
        { InQueue.Try_Remove<FTag_Queue_NeedsFormation>(); }
        else
        { InQueue.AddOrGet<FTag_Queue_NeedsFormation>(); }

        const auto Count = InQueueComp._Members.Num();
        InQueueComp._Pressure = FCk_Queue_Pressure{
            Count,
            InParams.Get_SoftLimit(),
            InParams.Get_HardLimit(),
            InParams.Get_SoftLimit() > 0 && Count >= InParams.Get_SoftLimit(),
            InParams.Get_HardLimit() > 0 && Count >= InParams.Get_HardLimit(),
            InQueueComp._Revision};

        for (const auto& Removed : RemovedMembers)
        {
            const auto Invalidated = FCk_Queue_MemberSnapshot{
                Removed.Get_Member(),
                Removed.Get_Mover(),
                Removed.Get_Ticket(),
                Removed.Get_Rank(),
                Removed.Get_TargetWorldTransform(),
                Removed.Get_AssignmentRevision(),
                false,
                ECk_Queue_MemberState::Invalidated};

            UUtils_Signal_OnQueueMemberStateChanged::Broadcast(
                InQueue,
                MakePayload(
                    InQueue,
                    FCk_Queue_MemberEvent{
                        InQueue,
                        Invalidated,
                        Removed.Get_State(),
                        ECk_Queue_EventReason::MemberDestroyed,
                        InQueueComp._Revision}));
        }

        UUtils_Signal_OnQueuePressureChanged::Broadcast(
            InQueue,
            MakePayload(InQueue, InQueueComp._Pressure));
        UUtils_Signal_OnQueueFormationStateChanged::Broadcast(
            InQueue,
            MakePayload(
                InQueue,
                FCk_Queue_FormationState{
                    InQueueComp._State,
                    ECk_Queue_EventReason::MemberDestroyed,
                    InQueueComp._Revision,
                    InQueueComp._RetryEpisode}));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Queue_CancelPendingRequests::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InQueue,
            const FFragment_Queue_Requests& InRequests)
        -> void
    {
        request::FireCancelledForPending(InQueue, InRequests.Get_Requests());
    }

    auto
        FProcessor_Queue_EndPlay::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InQueue,
            FFragment_Queue& InQueueComp)
        -> void
    {
        ++InQueueComp._Revision;
        InQueueComp._State = ECk_Queue_State::Invalidated;

        for (const auto& Member : InQueueComp._Members)
        {
            const auto Invalidated = FCk_Queue_MemberSnapshot{
                Member.Get_Member(),
                Member.Get_Mover(),
                Member.Get_Ticket(),
                Member.Get_Rank(),
                Member.Get_TargetWorldTransform(),
                Member.Get_AssignmentRevision(),
                false,
                ECk_Queue_MemberState::Invalidated};

            UUtils_Signal_OnQueueMemberStateChanged::Broadcast(
                InQueue,
                MakePayload(
                    InQueue,
                    FCk_Queue_MemberEvent{
                        InQueue,
                        Invalidated,
                        Member.Get_State(),
                        ECk_Queue_EventReason::OwnerDestroyed,
                        InQueueComp._Revision}));
        }

        InQueueComp._Members.Reset();
        InQueueComp._Pressure = FCk_Queue_Pressure{
            0,
            InQueueComp._Pressure.Get_SoftLimit(),
            InQueueComp._Pressure.Get_HardLimit(),
            false,
            false,
            InQueueComp._Revision};

        const auto FormationState = FCk_Queue_FormationState{
            InQueueComp._State,
            ECk_Queue_EventReason::OwnerDestroyed,
            InQueueComp._Revision,
            InQueueComp._RetryEpisode};

        UUtils_Signal_OnQueueFormationStateChanged::Broadcast(
            InQueue,
            MakePayload(InQueue, FormationState));
        UUtils_Signal_OnQueueInvalidated::Broadcast(
            InQueue,
            MakePayload(InQueue, FormationState));
    }
}

// --------------------------------------------------------------------------------------------------------------------
