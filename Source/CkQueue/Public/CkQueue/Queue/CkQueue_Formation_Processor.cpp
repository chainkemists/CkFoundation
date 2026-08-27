#include "CkQueue/Queue/CkQueue_Formation_Processor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

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
        const auto NavigationChanged = InCurrent._LastNavigationRevision != NavigationRevision;
        const auto WorldTimeSeconds = static_cast<double>(World->GetTimeSeconds());
        const auto DistanceAwareReservation = InParams.Get_SlotClaimPolicy()
                == ECk_Queue_SlotClaimPolicy::ReserveOnFormation
            && InParams.Get_ReserveAssignmentPolicy()
                == ECk_Queue_ReserveAssignmentPolicy::DistanceThenTicket;

        const auto HasPartialWaiters = [&InCurrent]()
        {
            for (const auto& Member : InCurrent._Members)
            {
                if (Member.Get_State() == ECk_Queue_MemberState::WaitingForNavigationChange)
                { return true; }
            }
            return false;
        };

        // A settled queue holds FTag_Queue_NeedsFormation purely as a navigation-revision subscription.
        // This processor is the only thing that re-solves a formation, and no caller can send a "the world
        // changed" request, so a line that has already placed everyone must keep listening or it never
        // notices an obstacle appearing inside it -- a fixture dropped mid-queue otherwise leaves the
        // members walking to slots that now sit inside the new geometry. The subscription is the
        // mechanism; this early-out is what makes holding it every frame cheap.
        const auto IsSettledFormation = InCurrent._State == ECk_Queue_State::Ready
            && InCurrent._LastNavigationRevision == NavigationRevision
            && NOT HasPartialWaiters()
            && NOT InCurrent._HasPendingClaimOffer;
        if (IsSettledFormation)
        {
            if (NOT DistanceAwareReservation
                || WorldTimeSeconds < InCurrent._NextReserveAssignmentRefreshWorldSeconds)
            { return; }
        }

        // A failed mover stays counted for admission pressure but has no slot. Once navigation changes, it is
        // deterministically rearmed behind the viable members that were allowed to reflow meanwhile.
        if (HasPartialWaiters() && InCurrent._LastNavigationRevision != NavigationRevision)
        {
            InCurrent._LastNavigationRevision = NavigationRevision;
            ++InCurrent._Revision;
            for (auto& Member : InCurrent._Members)
            {
                if (Member.Get_State() != ECk_Queue_MemberState::WaitingForNavigationChange) { continue; }
                const auto Previous = Member;
                Member = FCk_Queue_MemberSnapshot{
                    Member.Get_Member(), Member.Get_Mover(), Member.Get_Ticket(), INDEX_NONE,
                    FTransform::Identity, 0, Member.Get_MovementSuppressed(), ECk_Queue_MemberState::PendingAdmission};
                UUtils_Signal_OnQueueMemberStateChanged::Broadcast(
                    InQueue,
                    MakePayload(InQueue, FCk_Queue_MemberEvent{
                        InQueue, Member, Previous.Get_State(), ECk_Queue_EventReason::NavigationChanged, InCurrent._Revision}));
            }
            InCurrent._State = ECk_Queue_State::WaitingForFormation;
            UUtils_Signal_OnQueueFormationStateChanged::Broadcast(
                InQueue,
                MakePayload(InQueue, FCk_Queue_FormationState{
                    InCurrent._State, ECk_Queue_EventReason::NavigationChanged, InCurrent._Revision, InCurrent._RetryEpisode}));
        }
        else if (HasPartialWaiters() && InCurrent._State == ECk_Queue_State::Ready
            && NOT InCurrent._HasPendingClaimOffer)
        {
            // Keep the tag as a cheap nav-revision listener, but do not rebuild the viable prefix every frame.
            return;
        }

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

        auto ActiveMemberIndices = TArray<int32>{};
        const auto ClaimOnReach = InParams.Get_SlotClaimPolicy() == ECk_Queue_SlotClaimPolicy::ClaimFirstAvailableOnReach;
        for (auto MemberIndex = 0; MemberIndex < InCurrent._Members.Num(); ++MemberIndex)
        {
            const auto& Member = InCurrent._Members[MemberIndex];
            const auto IsClaimed = Member.Get_State() == ECk_Queue_MemberState::AtFront
                || Member.Get_State() == ECk_Queue_MemberState::AtSlot;
            const auto IsSuppressedUnclaimed = ClaimOnReach && Member.Get_MovementSuppressed() && NOT IsClaimed;
            // WaitingForMover is a terminal-for-now lifecycle state in both policies. Initial synthetic no-mover
            // joins remain PendingAdmission and therefore keep the existing external/manual-outcome behavior.
            const auto IsWaitingForMover = Member.Get_State() == ECk_Queue_MemberState::WaitingForMover;
            if (Member.Get_State() != ECk_Queue_MemberState::WaitingForNavigationChange
                && NOT IsSuppressedUnclaimed
                && NOT IsWaitingForMover)
            { ActiveMemberIndices.Add(MemberIndex); }
        }

        if (ActiveMemberIndices.IsEmpty())
        {
            InCurrent._State = ECk_Queue_State::Ready;
            InCurrent._HasPendingClaimOffer = false;
            InCurrent._Pressure = FCk_Queue_Pressure{
                InCurrent._Members.Num(), InParams.Get_SoftLimit(), InParams.Get_HardLimit(),
                InParams.Get_SoftLimit() > 0 && InCurrent._Members.Num() >= InParams.Get_SoftLimit(),
                InParams.Get_HardLimit() > 0 && InCurrent._Members.Num() >= InParams.Get_HardLimit(),
                InCurrent._Revision};
            UUtils_Signal_OnQueuePressureChanged::Broadcast(InQueue, MakePayload(InQueue, InCurrent._Pressure));
            // Retained while the queue has members: the tag is this formation's nav-revision
            // subscription (see the settled early-out above), not just a "needs work now" flag.
            if (InCurrent._Members.IsEmpty()) { InQueue.Remove<MarkedDirtyBy>(); }
            else { InQueue.AddOrGet<FTag_Queue_NeedsFormation>(); }
            return;
        }

        const auto LayoutResult = queue::layout::Build(
            InCurrent._LastOwnerWorldTransform,
            ActiveMemberIndices.Num(),
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

        const auto PlacementCountMatches = LayoutResult.Placements.Num() == ActiveMemberIndices.Num();
        CK_ENSURE_IF_NOT(PlacementCountMatches,
            TEXT("Queue [{}] layout returned [{}] placements for [{}] members"),
            InQueue,
            LayoutResult.Placements.Num(),
            ActiveMemberIndices.Num())
        {}
        if (NOT PlacementCountMatches)
        { return; }

        const auto PreviousMembers = InCurrent._Members;
        const auto AssignmentRevision = InCurrent._Revision + 1;

        auto PlacementByMember = TArray<int32>{};
        PlacementByMember.Init(INDEX_NONE, InCurrent._Members.Num());
        auto PlacementIsUsed = TArray<bool>{};
        PlacementIsUsed.Init(false, LayoutResult.Placements.Num());
        auto MemberAssignmentChanged = TArray<bool>{};
        MemberAssignmentChanged.Init(false, InCurrent._Members.Num());
        auto NextUnclaimedPlacement = int32{INDEX_NONE};
        if (ClaimOnReach)
        {
            // Claims are identified by their queue-wide rank, never their current member-array index.
            for (auto MemberIndex = 0; MemberIndex < PreviousMembers.Num(); ++MemberIndex)
            {
                const auto& Previous = PreviousMembers[MemberIndex];
                const auto WasClaimed = Previous.Get_State() == ECk_Queue_MemberState::AtFront
                    || Previous.Get_State() == ECk_Queue_MemberState::AtSlot;
                if (NOT WasClaimed) { continue; }
                for (auto PlacementIndex = 0; PlacementIndex < LayoutResult.Placements.Num(); ++PlacementIndex)
                {
                    const auto& Placement = LayoutResult.Placements[PlacementIndex];
                    if (NOT PlacementIsUsed[PlacementIndex]
                        && Placement.Rank == Previous.Get_Rank())
                    {
                        PlacementByMember[MemberIndex] = PlacementIndex;
                        PlacementIsUsed[PlacementIndex] = true;
                        break;
                    }
                }
            }

            // Every unclaimed contender receives the same next free rank as a provisional target. Reached
            // reports decide who owns it; the winner remains claimed while the next formation retargets the
            // remaining contenders with a newer assignment revision.
            for (auto PlacementIndex = 0; PlacementIndex < LayoutResult.Placements.Num(); ++PlacementIndex)
            {
                if (PlacementIsUsed[PlacementIndex]) { continue; }
                NextUnclaimedPlacement = PlacementIndex;
                break;
            }

            const auto TakeNextUnusedPlacement = [&LayoutResult, &PlacementIsUsed]() -> int32
            {
                for (auto PlacementIndex = 0; PlacementIndex < LayoutResult.Placements.Num(); ++PlacementIndex)
                {
                    if (PlacementIsUsed[PlacementIndex]) { continue; }

                    PlacementIsUsed[PlacementIndex] = true;
                    return PlacementIndex;
                }
                return INDEX_NONE;
            };

            for (const auto MemberIndex : ActiveMemberIndices)
            {
                if (PlacementByMember[MemberIndex] != INDEX_NONE) { continue; }

                const auto NextPlacement = TakeNextUnusedPlacement();
                if (NextPlacement == INDEX_NONE) { break; }
                PlacementByMember[MemberIndex] = NextPlacement;
            }
        }
        else if (DistanceAwareReservation)
        {
            auto MemberIsUsed = TArray<bool>{};
            MemberIsUsed.Init(false, InCurrent._Members.Num());

            // Read every active mover once. The matching pass is intentionally O(M^2), but querying an
            // entity transform inside that inner loop magnifies the cost and can mix locations from frames.
            auto MoverLocations = TArray<FVector>{};
            MoverLocations.Init(FVector::ZeroVector, InCurrent._Members.Num());
            auto MoverHasTransform = TArray<bool>{};
            MoverHasTransform.Init(false, InCurrent._Members.Num());
            for (const auto MemberIndex : ActiveMemberIndices)
            {
                const auto Mover = PreviousMembers[MemberIndex].Get_Mover();
                const auto HasTransform = ck::IsValid(Mover) && UCk_Utils_Transform_UE::Has(Mover);
                if (NOT HasTransform) { continue; }

                MoverLocations[MemberIndex] = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(
                    UCk_Utils_Transform_UE::CastChecked(Mover)).GetLocation();
                MoverHasTransform[MemberIndex] = true;
            }

            // An arrived reservation is authoritative. Distance ordering only reshuffles movers that are
            // still travelling, so service readiness and SlotReached remain stable while the tail adapts.
            for (const auto MemberIndex : ActiveMemberIndices)
            {
                const auto& Previous = PreviousMembers[MemberIndex];
                const auto WasArrived = Previous.Get_State() == ECk_Queue_MemberState::AtFront
                    || Previous.Get_State() == ECk_Queue_MemberState::AtSlot;
                if (NOT WasArrived) { continue; }

                for (auto PlacementIndex = 0; PlacementIndex < LayoutResult.Placements.Num(); ++PlacementIndex)
                {
                    const auto& Placement = LayoutResult.Placements[PlacementIndex];
                    if (PlacementIsUsed[PlacementIndex]
                        || Placement.Rank != Previous.Get_Rank())
                    { continue; }

                    PlacementByMember[MemberIndex] = PlacementIndex;
                    PlacementIsUsed[PlacementIndex] = true;
                    MemberIsUsed[MemberIndex] = true;
                    break;
                }
            }

            const auto TryGetMoverLocation = [&MoverLocations, &MoverHasTransform](
                int32 InMemberIndex,
                FVector& OutLocation) -> bool
            {
                if (NOT MoverHasTransform.IsValidIndex(InMemberIndex)
                    || NOT MoverHasTransform[InMemberIndex])
                { return false; }

                OutLocation = MoverLocations[InMemberIndex];
                return true;
            };

            const auto HysteresisUu = InParams.Get_ReserveAssignmentHysteresisUu();
            for (auto PlacementIndex = 0; PlacementIndex < LayoutResult.Placements.Num(); ++PlacementIndex)
            {
                if (PlacementIsUsed[PlacementIndex]) { continue; }

                const auto& Placement = LayoutResult.Placements[PlacementIndex];
                auto CurrentMemberIndex = int32{INDEX_NONE};
                auto BestMemberIndex = int32{INDEX_NONE};
                auto BestDistanceUu = TNumericLimits<float>::Max();

                for (const auto MemberIndex : ActiveMemberIndices)
                {
                    if (MemberIsUsed[MemberIndex]) { continue; }

                    const auto& Previous = PreviousMembers[MemberIndex];
                    if (Previous.Get_Rank() == Placement.Rank)
                    { CurrentMemberIndex = MemberIndex; }

                    auto MoverLocation = FVector::ZeroVector;
                    if (NOT TryGetMoverLocation(MemberIndex, MoverLocation)) { continue; }

                    const auto DistanceUu = FVector::Dist2D(
                        MoverLocation,
                        Placement.TargetWorldTransform.GetLocation());
                    const auto IsCloser = DistanceUu < BestDistanceUu - KINDA_SMALL_NUMBER;
                    const auto IsDistanceTie = FMath::IsNearlyEqual(DistanceUu, BestDistanceUu, KINDA_SMALL_NUMBER);
                    if (BestMemberIndex == INDEX_NONE
                        || IsCloser
                        || (IsDistanceTie
                            && Previous.Get_Ticket() < PreviousMembers[BestMemberIndex].Get_Ticket()))
                    {
                        BestMemberIndex = MemberIndex;
                        BestDistanceUu = DistanceUu;
                    }
                }

                auto SelectedMemberIndex = BestMemberIndex;
                if (CurrentMemberIndex != INDEX_NONE)
                {
                    auto CurrentLocation = FVector::ZeroVector;
                    const auto CurrentHasTransform = TryGetMoverLocation(CurrentMemberIndex, CurrentLocation);
                    if (NOT CurrentHasTransform
                        && PreviousMembers[CurrentMemberIndex].Get_AssignmentRevision() > 0)
                    { SelectedMemberIndex = CurrentMemberIndex; }
                    else if (CurrentHasTransform && BestMemberIndex != INDEX_NONE)
                    {
                        const auto CurrentDistanceUu = FVector::Dist2D(
                            CurrentLocation,
                            Placement.TargetWorldTransform.GetLocation());
                        if (CurrentDistanceUu <= BestDistanceUu + HysteresisUu)
                        { SelectedMemberIndex = CurrentMemberIndex; }
                    }
                }

                if (SelectedMemberIndex == INDEX_NONE)
                {
                    // No remaining mover exposes Transform. Preserve deterministic FIFO behavior until
                    // distance evidence exists rather than stealing an established reservation blindly.
                    for (const auto MemberIndex : ActiveMemberIndices)
                    {
                        if (MemberIsUsed[MemberIndex]) { continue; }
                        if (SelectedMemberIndex == INDEX_NONE
                            || PreviousMembers[MemberIndex].Get_Ticket()
                                < PreviousMembers[SelectedMemberIndex].Get_Ticket())
                        { SelectedMemberIndex = MemberIndex; }
                    }
                }

                if (SelectedMemberIndex == INDEX_NONE) { continue; }
                PlacementByMember[SelectedMemberIndex] = PlacementIndex;
                PlacementIsUsed[PlacementIndex] = true;
                MemberIsUsed[SelectedMemberIndex] = true;
            }
        }
        else
        {
            for (auto PlacementIndex = 0; PlacementIndex < LayoutResult.Placements.Num(); ++PlacementIndex)
            {
                PlacementByMember[ActiveMemberIndices[PlacementIndex]] = PlacementIndex;
            }
        }
        for (auto MemberIndex = 0; MemberIndex < InCurrent._Members.Num(); ++MemberIndex)
        {
            const auto& Previous = PreviousMembers[MemberIndex];
            const auto PlacementIndex = PlacementByMember[MemberIndex];
            if (PlacementIndex == INDEX_NONE)
            {
                InCurrent._Members[MemberIndex] = Previous;
                continue;
            }

            const auto& Placement = LayoutResult.Placements[PlacementIndex];
            const auto ProvisionalPlacementIndex = ClaimOnReach
                && NextUnclaimedPlacement != INDEX_NONE
                ? NextUnclaimedPlacement
                : PlacementIndex;
            const auto& ProvisionalPlacement = LayoutResult.Placements[ProvisionalPlacementIndex];
            const auto PreviousWasArrived = Previous.Get_State() == ECk_Queue_MemberState::AtFront
                || Previous.Get_State() == ECk_Queue_MemberState::AtSlot;
            const auto RetainedReservation = NOT ClaimOnReach
                && Previous.Get_AssignmentRevision() > 0
                && (Previous.Get_State() == ECk_Queue_MemberState::Assigned
                    || Previous.Get_State() == ECk_Queue_MemberState::MovingToSlot
                    || PreviousWasArrived)
                && (NOT NavigationChanged || PreviousWasArrived)
                && Previous.Get_Rank() == Placement.Rank
                && Previous.Get_TargetWorldTransform().Equals(Placement.TargetWorldTransform);
            if (RetainedReservation)
            {
                // A non-navigation reflow may retain any materially unchanged reservation. During
                // navigation revalidation only an ARRIVED reservation is stable: an en-route mover
                // still needs a fresh assignment revision so Crowd replans a possibly invalid corridor.
                // Preserving the arrived state prevents a settled mover from reporting SlotReached
                // again every time its own stationary nav markup finishes rebuilding.
                InCurrent._Members[MemberIndex] = Previous;
                continue;
            }
            const auto RetainedClaim = ClaimOnReach
                && (Previous.Get_State() == ECk_Queue_MemberState::AtFront || Previous.Get_State() == ECk_Queue_MemberState::AtSlot)
                && Previous.Get_Rank() == Placement.Rank;
            if (RetainedClaim)
            {
                InCurrent._Members[MemberIndex] = Previous;
                continue;
            }

            const auto RetainedProvisional = ClaimOnReach
                && NOT NavigationChanged
                && Previous.Get_State() == ECk_Queue_MemberState::MovingToSlot
                && Previous.Get_AssignmentRevision() > 0
                && Previous.Get_Rank() == ProvisionalPlacement.Rank
                && Previous.Get_TargetWorldTransform().Equals(ProvisionalPlacement.TargetWorldTransform);
            if (RetainedProvisional)
            {
                InCurrent._Members[MemberIndex] = Previous;
                continue;
            }

            InCurrent._Members[MemberIndex] = FCk_Queue_MemberSnapshot{
                Previous.Get_Member(), Previous.Get_Mover(), Previous.Get_Ticket(),
                ClaimOnReach ? ProvisionalPlacement.Rank : Placement.Rank,
                ClaimOnReach ? ProvisionalPlacement.TargetWorldTransform : Placement.TargetWorldTransform,
                AssignmentRevision,
                Previous.Get_MovementSuppressed(),
                Previous.Get_MovementSuppressed() ? ECk_Queue_MemberState::MovementSuppressed
                    : ClaimOnReach ? ECk_Queue_MemberState::MovingToSlot : ECk_Queue_MemberState::Assigned};
            MemberAssignmentChanged[MemberIndex] = true;
        }

        const auto AnyMemberAssignmentChanged = MemberAssignmentChanged.Contains(true);
        ScheduleNextReserveAssignmentRefresh(
            InQueue,
            InParams,
            InCurrent,
            WorldTimeSeconds,
            IsSettledFormation && DistanceAwareReservation);
        if (IsSettledFormation && DistanceAwareReservation && NOT AnyMemberAssignmentChanged)
        { return; }

        ++InCurrent._Revision;
        check(InCurrent._Revision == AssignmentRevision);

        InCurrent._State = ECk_Queue_State::Ready;
        InCurrent._RetryEpisode = 0;
        InCurrent._NextFormationRetryWorldSeconds = 0.0;
        InCurrent._LastNavigationRevision = NavigationRevision;
        InCurrent._HasPendingClaimOffer = false;
        InCurrent._Pressure = FCk_Queue_Pressure{
            InCurrent._Members.Num(),
            InParams.Get_SoftLimit(),
            InParams.Get_HardLimit(),
            InParams.Get_SoftLimit() > 0 && InCurrent._Members.Num() >= InParams.Get_SoftLimit(),
            InParams.Get_HardLimit() > 0 && InCurrent._Members.Num() >= InParams.Get_HardLimit(),
            InCurrent._Revision};
        // Retained while the queue has members: the tag is this formation's nav-revision
        // subscription (see the settled early-out above), not just a "needs work now" flag.
        if (InCurrent._Members.IsEmpty()) { InQueue.Remove<MarkedDirtyBy>(); }
        else { InQueue.AddOrGet<FTag_Queue_NeedsFormation>(); }

        for (auto MemberIndex = 0; MemberIndex < InCurrent._Members.Num(); ++MemberIndex)
        {
            if (NOT MemberAssignmentChanged[MemberIndex])
            { continue; }

            const auto& Previous = PreviousMembers[MemberIndex];
            const auto& Current = InCurrent._Members[MemberIndex];
            UUtils_Signal_OnQueueMemberStateChanged::Broadcast(
                InQueue,
                MakePayload(
                    InQueue,
                    FCk_Queue_MemberEvent{
                        InQueue,
                        Current,
                        Previous.Get_State(),
                        ECk_Queue_EventReason::Reflowed,
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
        ScheduleNextReserveAssignmentRefresh(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue_Current& InCurrent,
            double InWorldTimeSeconds,
            bool InWasSettledRefresh)
        -> void
    {
        const auto RefreshSeconds = static_cast<double>(InParams.Get_ReserveAssignmentRefreshSeconds());
        if (RefreshSeconds <= 0.0)
        {
            // Zero intentionally means one atomic distance reassignment per frame.
            InCurrent._NextReserveAssignmentRefreshWorldSeconds = InWorldTimeSeconds;
            return;
        }

        const auto PhaseSpreadEnabled = InParams.Get_ReserveAssignmentRefreshPhaseSpread()
            == ECk_EnableDisable::Enable;
        if (NOT PhaseSpreadEnabled)
        {
            InCurrent._NextReserveAssignmentRefreshWorldSeconds = InWorldTimeSeconds + RefreshSeconds;
            return;
        }

        if (InWasSettledRefresh)
        {
            const auto PreviousDeadline = InCurrent._NextReserveAssignmentRefreshWorldSeconds;
            const auto DeadlineIsUsable = FMath::IsFinite(PreviousDeadline)
                && PreviousDeadline <= InWorldTimeSeconds;
            if (DeadlineIsUsable)
            {
                const auto MissedPeriods = FMath::FloorToDouble(
                    (InWorldTimeSeconds - PreviousDeadline) / RefreshSeconds) + 1.0;
                InCurrent._NextReserveAssignmentRefreshWorldSeconds = PreviousDeadline
                    + MissedPeriods * RefreshSeconds;
                return;
            }
        }

        // Same-registry entity hashes are commonly sequential, so scramble the stable handle hash before
        // mapping it to [0, 1). The denominator keeps the phase strictly below one full interval.
        auto PhaseHash = GetTypeHash(InQueue);
        PhaseHash ^= PhaseHash >> 16;
        PhaseHash *= 0x7feb352dU;
        PhaseHash ^= PhaseHash >> 15;
        PhaseHash *= 0x846ca68bU;
        PhaseHash ^= PhaseHash >> 16;
        constexpr auto Uint32Range = 4294967296.0;
        const auto PhaseFraction = static_cast<double>(PhaseHash) / Uint32Range;
        InCurrent._NextReserveAssignmentRefreshWorldSeconds = InWorldTimeSeconds + PhaseFraction * RefreshSeconds;
    }

    // --------------------------------------------------------------------------------------------------------------------

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
