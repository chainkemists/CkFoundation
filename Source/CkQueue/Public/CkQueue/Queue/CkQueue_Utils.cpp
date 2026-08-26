#include "CkQueue_Utils.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkEntityTag/CkEntityTag_Utils.h"

#include "CkQueue/CkQueue_Log.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(Tag_Queue_CategoryName, TEXT("Queue.Category"));

namespace
{
    constexpr auto QueueDebugDrawCVarName = TEXT("ck.Queue.DebugDraw");
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_queue_utils
{
    auto
    IsValidLayoutAlgorithm(
        ECk_Queue_LayoutAlgorithm InLayoutAlgorithm)
    -> bool
    {
        return InLayoutAlgorithm == ECk_Queue_LayoutAlgorithm::OrthogonalSnake
            || InLayoutAlgorithm == ECk_Queue_LayoutAlgorithm::Linear;
    }

    auto
    IsValidSlotClaimPolicy(
        ECk_Queue_SlotClaimPolicy InPolicy)
    -> bool
    {
        return InPolicy == ECk_Queue_SlotClaimPolicy::ReserveOnFormation
            || InPolicy == ECk_Queue_SlotClaimPolicy::ClaimFirstAvailableOnReach;
    }

    auto
    IsValidMovementOutcome(
        ECk_Queue_MovementOutcome InOutcome)
    -> bool
    {
        return InOutcome == ECk_Queue_MovementOutcome::Reached
            || InOutcome == ECk_Queue_MovementOutcome::Failed
            || InOutcome == ECk_Queue_MovementOutcome::Cancelled;
    }

    auto
    IsValidEnableDisable(
        ECk_EnableDisable InValue)
    -> bool
    {
        return InValue == ECk_EnableDisable::Enable
            || InValue == ECk_EnableDisable::Disable;
    }

    auto
    AreValidOrigins(
        const TArray<FCk_Queue_Origin>& InOrigins)
    -> bool
    {
        if (InOrigins.IsEmpty())
        { return false; }

        for (const auto& Origin : InOrigins)
        {
            if (Origin.Get_LocalTransform().ContainsNaN()
                || Origin.Get_Weight() <= 0
                || Origin.Get_HardLimitOverride() < INDEX_NONE)
            { return false; }
        }

        return true;
    }

    auto
    IsValidQueue(
        const FCk_Handle_Queue& InQueue)
    -> bool
    {
        return ck::IsValid(InQueue)
            && UCk_Utils_Queue_UE::Has(InQueue);
    }

    auto
    CanAcceptRequests(
        const FCk_Handle_Queue& InQueue)
    -> bool
    {
        return IsValidQueue(InQueue)
            && NOT InQueue.Has<ck::FTag_DestroyEntity_Initiate>()
            && InQueue.Get<ck::FFragment_Queue_Current>().Get_State() != ECk_Queue_State::Invalidated;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Queue_UE::
    Add(
        FCk_Handle& InOwner,
        const FCk_Fragment_Queue_ParamsData& InParams)
    -> FCk_Handle_Queue
{
    const auto OwnerIsValid = ck::IsValid(InOwner);
    CK_ENSURE_IF_NOT(OwnerIsValid,
        TEXT("Cannot add Queue: owner handle is invalid"))
    {}
    if (NOT OwnerIsValid)
    { return {}; }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InOwner);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Cannot add Queue to non-authoritative owner [{}]"), InOwner)
    {}
    if (NOT HasAuthority)
    { return {}; }

    const auto QueueIsAbsent = NOT Has(InOwner);
    CK_ENSURE_IF_NOT(QueueIsAbsent,
        TEXT("Cannot add Queue to [{}]: Queue is already present"), InOwner)
    {}
    if (NOT QueueIsAbsent)
    { return {}; }

    const auto OwnerHasTransform = UCk_Utils_Transform_UE::Has(InOwner);
    CK_ENSURE_IF_NOT(OwnerHasTransform,
        TEXT("Cannot add Queue to [{}]: a spatial queue owner requires Ck Transform"), InOwner)
    {}
    if (NOT OwnerHasTransform)
    { return {}; }

    const auto OriginsAreValid = ck_queue_utils::AreValidOrigins(InParams.Get_Origins());
    const auto SlotSpacingIsValid = InParams.Get_SlotSpacingUu() > 0.0f;
    const auto SlotMovementRadiiAreValid = FMath::IsFinite(InParams.Get_SlotClaimRadiusUu())
        && FMath::IsFinite(InParams.Get_SlotSettleRadiusUu())
        && FMath::IsFinite(InParams.Get_SlotReacquireRadiusUu())
        && InParams.Get_SlotSettleRadiusUu() > 0.0f
        && InParams.Get_SlotReacquireRadiusUu() >= InParams.Get_SlotSettleRadiusUu()
        && InParams.Get_SlotClaimRadiusUu() >= InParams.Get_SlotReacquireRadiusUu();
    const auto SearchBudgetIsValid = InParams.Get_MaxFormationSearchNodes() > 0;
    const auto RadiusIsValid = InParams.Get_AgentRadiusUu() > 0.0f;
    const auto HalfHeightIsValid = InParams.Get_AgentHalfHeightUu() >= InParams.Get_AgentRadiusUu();
    const auto SpacingPreventsOverlap = InParams.Get_SlotSpacingUu()
        >= 2.0f * (InParams.Get_AgentRadiusUu() + InParams.Get_ClearanceMarginUu());
    const auto LimitsAreValid = InParams.Get_SoftLimit() >= 0
        && InParams.Get_HardLimit() >= 0
        && (InParams.Get_HardLimit() == 0 || InParams.Get_SoftLimit() <= InParams.Get_HardLimit());
    const auto OtherParamsAreValid = ck_queue_utils::IsValidLayoutAlgorithm(InParams.Get_LayoutAlgorithm())
        && ck_queue_utils::IsValidSlotClaimPolicy(InParams.Get_SlotClaimPolicy())
        && InParams.Get_TransformEpsilonUu() >= 0.0f
        && InParams.Get_RotationEpsilonDegrees() >= 0.0f
        && InParams.Get_MaxNavigationRetries() >= 0
        && InParams.Get_NavigationRetryDelaySeconds() >= 0.0f
        && InParams.Get_ClearanceMarginUu() >= 0.0f;
    const auto ParamsAreValid = OriginsAreValid
        && SlotSpacingIsValid
        && SlotMovementRadiiAreValid
        && SearchBudgetIsValid
        && RadiusIsValid
        && HalfHeightIsValid
        && SpacingPreventsOverlap
        && LimitsAreValid
        && OtherParamsAreValid;
    CK_ENSURE_IF_NOT(ParamsAreValid,
        TEXT("Cannot add Queue to [{}]: parameters are invalid"), InOwner)
    {}
    if (NOT ParamsAreValid)
    { return {}; }

    InOwner.Add<ck::FFragment_Queue_Params>(InParams);
    InOwner.Add<ck::FFragment_Queue_Current>();
    InOwner.Add<ck::FTag_Queue_NeedsSetup>();

    if (InParams.Get_Category().IsValid())
    { UCk_Utils_EntityTag_UE::Add_UsingGameplayTag(InOwner, InParams.Get_Category()); }

    return Cast(InOwner);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_Queue_UE,
    FCk_Handle_Queue,
    ck::FFragment_Queue_Params,
    ck::FFragment_Queue_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Queue_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Queue_UE::
    Get_Members(
        const FCk_Handle_Queue& InQueue)
    -> TArray<FCk_Queue_MemberSnapshot>
{
    const auto QueueIsValid = ck_queue_utils::IsValidQueue(InQueue);
    CK_ENSURE_IF_NOT(QueueIsValid, TEXT("Get_Members called with invalid Queue [{}]"), InQueue)
    {}
    if (NOT QueueIsValid)
    { return {}; }
    return InQueue.Get<ck::FFragment_Queue_Current>().Get_Members();
}

auto
    UCk_Utils_Queue_UE::
    Get_MemberCount(
        const FCk_Handle_Queue& InQueue)
    -> int32
{
    const auto QueueIsValid = ck_queue_utils::IsValidQueue(InQueue);
    CK_ENSURE_IF_NOT(QueueIsValid, TEXT("Get_MemberCount called with invalid Queue [{}]"), InQueue)
    {}
    if (NOT QueueIsValid)
    { return 0; }
    return InQueue.Get<ck::FFragment_Queue_Current>().Get_Members().Num();
}

auto
    UCk_Utils_Queue_UE::
    Get_Pressure(
        const FCk_Handle_Queue& InQueue)
    -> FCk_Queue_Pressure
{
    const auto QueueIsValid = ck_queue_utils::IsValidQueue(InQueue);
    CK_ENSURE_IF_NOT(QueueIsValid, TEXT("Get_Pressure called with invalid Queue [{}]"), InQueue)
    {}
    if (NOT QueueIsValid)
    { return {}; }
    return InQueue.Get<ck::FFragment_Queue_Current>().Get_Pressure();
}

auto
    UCk_Utils_Queue_UE::
    Get_State(
        const FCk_Handle_Queue& InQueue)
    -> ECk_Queue_State
{
    const auto QueueIsValid = ck_queue_utils::IsValidQueue(InQueue);
    CK_ENSURE_IF_NOT(QueueIsValid, TEXT("Get_State called with invalid Queue [{}]"), InQueue)
    {}
    if (NOT QueueIsValid)
    { return ECk_Queue_State::Invalidated; }
    return InQueue.Get<ck::FFragment_Queue_Current>().Get_State();
}

auto
    UCk_Utils_Queue_UE::
    Get_Revision(
        const FCk_Handle_Queue& InQueue)
    -> int32
{
    const auto QueueIsValid = ck_queue_utils::IsValidQueue(InQueue);
    CK_ENSURE_IF_NOT(QueueIsValid, TEXT("Get_Revision called with invalid Queue [{}]"), InQueue)
    {}
    if (NOT QueueIsValid)
    { return 0; }
    return InQueue.Get<ck::FFragment_Queue_Current>().Get_Revision();
}

auto
    UCk_Utils_Queue_UE::
    Get_Origins(
        const FCk_Handle_Queue& InQueue)
    -> TArray<FCk_Queue_Origin>
{
    const auto QueueIsValid = ck_queue_utils::IsValidQueue(InQueue);
    CK_ENSURE_IF_NOT(QueueIsValid, TEXT("Get_Origins called with invalid Queue [{}]"), InQueue)
    {}
    if (NOT QueueIsValid)
    { return {}; }
    return InQueue.Get<ck::FFragment_Queue_Current>().Get_Origins();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Queue_UE::
    Get_Category(
        const FCk_Handle_Queue& InQueue)
    -> FGameplayTag
{
    const auto QueueIsValid = ck_queue_utils::IsValidQueue(InQueue);
    CK_ENSURE_IF_NOT(QueueIsValid, TEXT("Get_Category called with invalid Queue [{}]"), InQueue)
    {}
    if (NOT QueueIsValid)
    { return FGameplayTag::EmptyTag; }

    return InQueue.Get<ck::FFragment_Queue_Params>().Get_Category();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Queue_UE::
    Get_DebugSnapshots(
        const FCk_Handle& InAnyEntityInWorld)
    -> TArray<FCk_Queue_DebugSnapshot>
{
    // Debug consumers race normal PIE teardown. An invalid selector means the world is gone, not a malformed
    // gameplay request, so fail closed without turning expected teardown into an ensure.
    if (ck::Is_NOT_Valid(InAnyEntityInWorld))
    { return {}; }

    const auto Registry = InAnyEntityInWorld.Get_RegistryView();
    auto Result = TArray<FCk_Queue_DebugSnapshot>{};
    Registry.View<ck::FFragment_Queue_Params, ck::FFragment_Queue_Current, CK_IGNORE_PENDING_KILL>().ForEach(
        [&Result, Registry](FCk_Entity InQueueEntity,
                            const ck::FFragment_Queue_Params& InParams,
                            const ck::FFragment_Queue_Current& InCurrent)
        {
            const auto Queue = FCk_Handle{InQueueEntity, Registry.Get_RegistryHandle()};
            const auto QueueTransform = UCk_Utils_Transform_UE::Cast(Queue);
            if (ck::Is_NOT_Valid(QueueTransform))
            { return; }

            const auto OwnerWorldTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(QueueTransform);
            auto OriginWorldTransforms = TArray<FTransform>{};
            OriginWorldTransforms.Reserve(InCurrent.Get_Origins().Num());
            for (const auto& Origin : InCurrent.Get_Origins())
            { OriginWorldTransforms.Add(Origin.Get_LocalTransform() * OwnerWorldTransform); }

            auto Members = TArray<FCk_Queue_DebugMemberSnapshot>{};
            Members.Reserve(InCurrent.Get_Members().Num());
            for (const auto& Member : InCurrent.Get_Members())
            {
                const auto MemberHandle = Member.Get_Member();
                const auto Mover = Member.Get_Mover();
                const auto MemberIsValid = ck::IsValid(MemberHandle);
                const auto MoverIsValid = ck::IsValid(Mover);
                const auto MoverHasTransform = MoverIsValid && UCk_Utils_Transform_UE::Has(Mover);
                const auto MoverTransform = MoverHasTransform
                    ? UCk_Utils_Transform_UE::Get_EntityCurrentTransform(UCk_Utils_Transform_UE::CastChecked(Mover))
                    : FTransform::Identity;
                Members.Emplace(
                    MemberIsValid ? static_cast<int64>(MemberHandle.Get_Entity().Get_ID()) : 0,
                    MoverIsValid ? static_cast<int64>(Mover.Get_Entity().Get_ID()) : 0,
                    MemberIsValid ? MemberHandle.Get_DebugName() : NAME_None,
                    MoverIsValid ? Mover.Get_DebugName() : NAME_None,
                    MoverHasTransform,
                    MoverTransform,
                    Member.Get_Ticket(),
                    Member.Get_OriginIndex(),
                    Member.Get_Rank(),
                    Member.Get_TargetWorldTransform(),
                    Member.Get_AssignmentRevision(),
                    Member.Get_MovementSuppressed(),
                    Member.Get_State());
            }

            Result.Emplace(
                static_cast<int64>(InQueueEntity.Get_ID()),
                Queue.Get_DebugName(),
                InParams.Get_Category(),
                OwnerWorldTransform,
                InCurrent.Get_State(),
                InCurrent.Get_Revision(),
                InCurrent.Get_RetryEpisode(),
                InCurrent.Get_LayoutAlgorithm(),
                InParams.Get_SlotSpacingUu(),
                InParams.Get_SlotClaimPolicy(),
                FCk_Queue_FormationState{
                    InCurrent.Get_State(),
                    ECk_Queue_EventReason::None,
                    InCurrent.Get_Revision(),
                    InCurrent.Get_RetryEpisode()},
                InCurrent.Get_Pressure(),
                MoveTemp(OriginWorldTransforms),
                MoveTemp(Members));
        });
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Queue_UE::
    Get_IsDebugDrawEnabled()
    -> bool
{
    const auto* DebugDraw = IConsoleManager::Get().FindConsoleVariable(QueueDebugDrawCVarName);
    return DebugDraw != nullptr && DebugDraw->GetInt() != 0;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Queue_UE::
    Set_DebugDrawEnabled(
        bool InEnabled)
    -> void
{
    auto* DebugDraw = IConsoleManager::Get().FindConsoleVariable(QueueDebugDrawCVarName);
    const auto DebugDrawIsRegistered = DebugDraw != nullptr;
    CK_ENSURE_IF_NOT(DebugDrawIsRegistered, TEXT("Queue debug draw CVar is not registered"))
    {}
    if (NOT DebugDrawIsRegistered)
    { return; }

    DebugDraw->Set(InEnabled ? 1 : 0, ECVF_SetByConsole);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Queue_UE::
    Get_LayoutAlgorithm(
        const FCk_Handle_Queue& InQueue)
    -> ECk_Queue_LayoutAlgorithm
{
    const auto QueueIsValid = ck_queue_utils::IsValidQueue(InQueue);
    CK_ENSURE_IF_NOT(QueueIsValid, TEXT("Get_LayoutAlgorithm called with invalid Queue [{}]"), InQueue)
    {}
    if (NOT QueueIsValid)
    { return ECk_Queue_LayoutAlgorithm::OrthogonalSnake; }
    return InQueue.Get<ck::FFragment_Queue_Current>().Get_LayoutAlgorithm();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Queue_UE::
    Get_SlotClaimPolicy(
        const FCk_Handle_Queue& InQueue)
    -> ECk_Queue_SlotClaimPolicy
{
    const auto QueueIsValid = ck_queue_utils::IsValidQueue(InQueue);
    CK_ENSURE_IF_NOT(QueueIsValid, TEXT("Get_SlotClaimPolicy called with invalid Queue [{}]"), InQueue)
    {}
    if (NOT QueueIsValid)
    { return ECk_Queue_SlotClaimPolicy::ReserveOnFormation; }
    return InQueue.Get<ck::FFragment_Queue_Params>().Get_SlotClaimPolicy();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Queue_UE::
    Get_SlotSpacingUu(
        const FCk_Handle_Queue& InQueue)
    -> float
{
    const auto QueueIsValid = ck_queue_utils::IsValidQueue(InQueue);
    CK_ENSURE_IF_NOT(QueueIsValid, TEXT("Get_SlotSpacingUu called with invalid Queue [{}]"), InQueue)
    {}
    if (NOT QueueIsValid)
    { return 0.0f; }
    return InQueue.Get<ck::FFragment_Queue_Params>().Get_SlotSpacingUu();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Queue_UE::
    Get_SlotClaimRadiusUu(
        const FCk_Handle_Queue& InQueue)
    -> float
{
    const auto QueueIsValid = ck_queue_utils::IsValidQueue(InQueue);
    CK_ENSURE_IF_NOT(QueueIsValid, TEXT("Get_SlotClaimRadiusUu called with invalid Queue [{}]"), InQueue)
    {}
    if (NOT QueueIsValid)
    { return 0.0f; }
    return InQueue.Get<ck::FFragment_Queue_Params>().Get_SlotClaimRadiusUu();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Queue_UE::
    Get_SlotSettleRadiusUu(
        const FCk_Handle_Queue& InQueue)
    -> float
{
    const auto QueueIsValid = ck_queue_utils::IsValidQueue(InQueue);
    CK_ENSURE_IF_NOT(QueueIsValid, TEXT("Get_SlotSettleRadiusUu called with invalid Queue [{}]"), InQueue)
    {}
    if (NOT QueueIsValid)
    { return 0.0f; }
    return InQueue.Get<ck::FFragment_Queue_Params>().Get_SlotSettleRadiusUu();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Queue_UE::
    Get_SlotReacquireRadiusUu(
        const FCk_Handle_Queue& InQueue)
    -> float
{
    const auto QueueIsValid = ck_queue_utils::IsValidQueue(InQueue);
    CK_ENSURE_IF_NOT(QueueIsValid, TEXT("Get_SlotReacquireRadiusUu called with invalid Queue [{}]"), InQueue)
    {}
    if (NOT QueueIsValid)
    { return 0.0f; }
    return InQueue.Get<ck::FFragment_Queue_Params>().Get_SlotReacquireRadiusUu();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Queue_UE::
    Get_IsMember(
        const FCk_Handle_Queue& InQueue,
        const FCk_Handle& InMember)
    -> bool
{
    const auto QueueIsValid = ck_queue_utils::IsValidQueue(InQueue);
    CK_ENSURE_IF_NOT(QueueIsValid, TEXT("Get_IsMember called with invalid Queue [{}]"), InQueue)
    {}
    if (NOT QueueIsValid)
    { return false; }

    auto Snapshot = FCk_Queue_MemberSnapshot{};
    return TryGet_MemberSnapshot(InQueue, InMember, Snapshot);
}

auto
    UCk_Utils_Queue_UE::
    TryGet_MemberSnapshot(
        const FCk_Handle_Queue& InQueue,
        const FCk_Handle& InMember,
        FCk_Queue_MemberSnapshot& OutResult)
    -> bool
{
    OutResult = {};
    if (NOT ck_queue_utils::IsValidQueue(InQueue) || ck::Is_NOT_Valid(InMember))
    { return false; }

    for (const auto& Snapshot : InQueue.Get<ck::FFragment_Queue_Current>().Get_Members())
    {
        if (Snapshot.Get_Member() == InMember)
        {
            OutResult = Snapshot;
            return true;
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Queue_UE::
    Request_Join(
        FCk_Handle_Queue& InQueue,
        const FCk_Request_Queue_Join& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Queue
{
    const auto QueueIsValid = ck_queue_utils::CanAcceptRequests(InQueue);
    const auto HasAuthority = QueueIsValid && UCk_Utils_Net_UE::Get_HasAuthority(InQueue);
    const auto RequestIsValid = ck::IsValid(InRequest.Get_Member());
    CK_ENSURE_IF_NOT(QueueIsValid && HasAuthority && RequestIsValid,
        TEXT("Cannot enqueue Queue Join on [{}]: queue, authority, or member is invalid"), InQueue)
    {}
    if (NOT QueueIsValid || NOT HasAuthority || NOT RequestIsValid)
    {
        InDelegate.ExecuteIfBound(InQueue, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InQueue;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }
    InQueue.AddOrGet<ck::FFragment_Queue_Requests>()._Requests.Emplace(InRequest);
    return InQueue;
}

auto
    UCk_Utils_Queue_UE::
    Request_RestoreJoin(
        FCk_Handle_Queue& InQueue,
        const FCk_Request_Queue_RestoreJoin& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Queue
{
    const auto QueueIsValid = ck_queue_utils::CanAcceptRequests(InQueue);
    const auto HasAuthority = QueueIsValid && UCk_Utils_Net_UE::Get_HasAuthority(InQueue);
    const auto TicketIsValid = InRequest.Get_RestoredTicket() > 0
        && InRequest.Get_RestoredTicket() < MAX_int64;
    const auto RequestIsValid = ck::IsValid(InRequest.Get_Member()) && TicketIsValid;
    CK_ENSURE_IF_NOT(QueueIsValid && HasAuthority && RequestIsValid,
        TEXT("Cannot enqueue Queue Restore Join on [{}]: queue, authority, member, or ticket is invalid"), InQueue)
    {}
    if (NOT QueueIsValid || NOT HasAuthority || NOT RequestIsValid)
    {
        InDelegate.ExecuteIfBound(InQueue, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InQueue;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }
    InQueue.AddOrGet<ck::FFragment_Queue_Requests>()._Requests.Emplace(InRequest);
    return InQueue;
}

auto
    UCk_Utils_Queue_UE::
    Request_Leave(
        FCk_Handle_Queue& InQueue,
        const FCk_Request_Queue_Leave& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Queue
{
    const auto QueueIsValid = ck_queue_utils::CanAcceptRequests(InQueue);
    const auto HasAuthority = QueueIsValid && UCk_Utils_Net_UE::Get_HasAuthority(InQueue);
    const auto RequestIsValid = ck::IsValid(InRequest.Get_Member());
    CK_ENSURE_IF_NOT(QueueIsValid && HasAuthority && RequestIsValid,
        TEXT("Cannot enqueue Queue Leave on [{}]: queue, authority, or member is invalid"), InQueue)
    {}
    if (NOT QueueIsValid || NOT HasAuthority || NOT RequestIsValid)
    {
        InDelegate.ExecuteIfBound(InQueue, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InQueue;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }
    InQueue.AddOrGet<ck::FFragment_Queue_Requests>()._Requests.Emplace(InRequest);
    return InQueue;
}

auto
    UCk_Utils_Queue_UE::
    Request_AdvanceOrigin(
        FCk_Handle_Queue& InQueue,
        const FCk_Request_Queue_AdvanceOrigin& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Queue
{
    const auto QueueIsValid = ck_queue_utils::CanAcceptRequests(InQueue);
    const auto HasAuthority = QueueIsValid && UCk_Utils_Net_UE::Get_HasAuthority(InQueue);
    const auto RequestIsValid = InRequest.Get_OriginIndex() >= 0;
    CK_ENSURE_IF_NOT(QueueIsValid && HasAuthority && RequestIsValid,
        TEXT("Cannot enqueue Queue AdvanceOrigin on [{}]: queue, authority, or origin index is invalid"), InQueue)
    {}
    if (NOT QueueIsValid || NOT HasAuthority || NOT RequestIsValid)
    {
        InDelegate.ExecuteIfBound(InQueue, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InQueue;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }
    InQueue.AddOrGet<ck::FFragment_Queue_Requests>()._Requests.Emplace(InRequest);
    return InQueue;
}

auto
    UCk_Utils_Queue_UE::
    Request_SetOrigins(
        FCk_Handle_Queue& InQueue,
        const FCk_Request_Queue_SetOrigins& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Queue
{
    const auto QueueIsValid = ck_queue_utils::CanAcceptRequests(InQueue);
    const auto HasAuthority = QueueIsValid && UCk_Utils_Net_UE::Get_HasAuthority(InQueue);
    const auto RequestIsValid = ck_queue_utils::AreValidOrigins(InRequest.Get_Origins());
    CK_ENSURE_IF_NOT(QueueIsValid && HasAuthority && RequestIsValid,
        TEXT("Cannot enqueue Queue SetOrigins on [{}]: queue, authority, or origins are invalid"), InQueue)
    {}
    if (NOT QueueIsValid || NOT HasAuthority || NOT RequestIsValid)
    {
        InDelegate.ExecuteIfBound(InQueue, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InQueue;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }
    InQueue.AddOrGet<ck::FFragment_Queue_Requests>()._Requests.Emplace(InRequest);
    return InQueue;
}

auto
    UCk_Utils_Queue_UE::
    Request_SetLayout(
        FCk_Handle_Queue& InQueue,
        const FCk_Request_Queue_SetLayout& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Queue
{
    const auto QueueIsValid = ck_queue_utils::CanAcceptRequests(InQueue);
    const auto HasAuthority = QueueIsValid && UCk_Utils_Net_UE::Get_HasAuthority(InQueue);
    const auto RequestIsValid = ck_queue_utils::IsValidLayoutAlgorithm(InRequest.Get_LayoutAlgorithm());
    CK_ENSURE_IF_NOT(QueueIsValid && HasAuthority && RequestIsValid,
        TEXT("Cannot enqueue Queue SetLayout on [{}]: queue, authority, or layout is invalid"), InQueue)
    {}
    if (NOT QueueIsValid || NOT HasAuthority || NOT RequestIsValid)
    {
        InDelegate.ExecuteIfBound(InQueue, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InQueue;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }
    InQueue.AddOrGet<ck::FFragment_Queue_Requests>()._Requests.Emplace(InRequest);
    return InQueue;
}

auto
    UCk_Utils_Queue_UE::
    Request_SetMovementSuppressed(
        FCk_Handle_Queue& InQueue,
        const FCk_Request_Queue_SetMovementSuppressed& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Queue
{
    const auto QueueIsValid = ck_queue_utils::CanAcceptRequests(InQueue);
    const auto HasAuthority = QueueIsValid && UCk_Utils_Net_UE::Get_HasAuthority(InQueue);
    const auto RequestIsValid = ck::IsValid(InRequest.Get_Member())
        && ck_queue_utils::IsValidEnableDisable(InRequest.Get_MovementSuppressed());
    CK_ENSURE_IF_NOT(QueueIsValid && HasAuthority && RequestIsValid,
        TEXT("Cannot enqueue Queue SetMovementSuppressed on [{}]: queue, authority, or request is invalid"), InQueue)
    {}
    if (NOT QueueIsValid || NOT HasAuthority || NOT RequestIsValid)
    {
        InDelegate.ExecuteIfBound(InQueue, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InQueue;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }
    InQueue.AddOrGet<ck::FFragment_Queue_Requests>()._Requests.Emplace(InRequest);
    return InQueue;
}

auto
    UCk_Utils_Queue_UE::
    Request_ReportMovementOutcome(
        FCk_Handle_Queue& InQueue,
        const FCk_Request_Queue_ReportMovementOutcome& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_Queue
{
    const auto QueueIsValid = ck_queue_utils::CanAcceptRequests(InQueue);
    const auto HasAuthority = QueueIsValid && UCk_Utils_Net_UE::Get_HasAuthority(InQueue);
    const auto RequestIsValid = ck::IsValid(InRequest.Get_Member())
        && InRequest.Get_AssignmentRevision() > 0
        && ck_queue_utils::IsValidMovementOutcome(InRequest.Get_Outcome());
    CK_ENSURE_IF_NOT(QueueIsValid && HasAuthority && RequestIsValid,
        TEXT("Cannot enqueue Queue ReportMovementOutcome on [{}]: queue, authority, or request is invalid"), InQueue)
    {}
    if (NOT QueueIsValid || NOT HasAuthority || NOT RequestIsValid)
    {
        InDelegate.ExecuteIfBound(InQueue, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InQueue;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }
    InQueue.AddOrGet<ck::FFragment_Queue_Requests>()._Requests.Emplace(InRequest);
    return InQueue;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Queue_UE::
    BindTo_OnQueueMemberStateChanged(
        FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Queue_OnMemberStateChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Queue
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnQueueMemberStateChanged, InQueue, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InQueue;
}

auto
    UCk_Utils_Queue_UE::
    UnbindFrom_OnQueueMemberStateChanged(
        FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Queue_OnMemberStateChanged& InDelegate)
    -> FCk_Handle_Queue
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnQueueMemberStateChanged, InQueue, InDelegate);
    return InQueue;
}

auto
    UCk_Utils_Queue_UE::
    BindTo_OnQueuePressureChanged(
        FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Queue_OnPressureChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Queue
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnQueuePressureChanged, InQueue, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InQueue;
}

auto
    UCk_Utils_Queue_UE::
    UnbindFrom_OnQueuePressureChanged(
        FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Queue_OnPressureChanged& InDelegate)
    -> FCk_Handle_Queue
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnQueuePressureChanged, InQueue, InDelegate);
    return InQueue;
}

auto
    UCk_Utils_Queue_UE::
    BindTo_OnQueueFormationStateChanged(
        FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Queue_OnFormationStateChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Queue
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnQueueFormationStateChanged, InQueue, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InQueue;
}

auto
    UCk_Utils_Queue_UE::
    UnbindFrom_OnQueueFormationStateChanged(
        FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Queue_OnFormationStateChanged& InDelegate)
    -> FCk_Handle_Queue
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnQueueFormationStateChanged, InQueue, InDelegate);
    return InQueue;
}

auto
    UCk_Utils_Queue_UE::
    BindTo_OnQueueInvalidated(
        FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Queue_OnInvalidated& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Queue
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnQueueInvalidated, InQueue, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InQueue;
}

auto
    UCk_Utils_Queue_UE::
    UnbindFrom_OnQueueInvalidated(
        FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Queue_OnInvalidated& InDelegate)
    -> FCk_Handle_Queue
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnQueueInvalidated, InQueue, InDelegate);
    return InQueue;
}

// --------------------------------------------------------------------------------------------------------------------
