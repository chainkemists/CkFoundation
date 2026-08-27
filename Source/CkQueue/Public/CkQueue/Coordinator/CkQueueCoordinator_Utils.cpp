#include "CkQueue/Coordinator/CkQueueCoordinator_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkQueue/Queue/CkQueue_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_queue_coordinator_utils
{
    auto
        IsSelectionPolicyValid(
            ECk_QueueCoordinator_SelectionPolicy InPolicy)
        -> bool
    {
        return InPolicy == ECk_QueueCoordinator_SelectionPolicy::LeastMembersThenDistance
            || InPolicy == ECk_QueueCoordinator_SelectionPolicy::NearestThenLeastMembers;
    }

    auto
        IsInSameRegistry(
            const FCk_Handle& InA,
            const FCk_Handle& InB)
        -> bool
    {
        return ck::IsValid(InA)
            && ck::IsValid(InB)
            && InA.Get_RegistryView().Get_RegistryHandle()
                == InB.Get_RegistryView().Get_RegistryHandle();
    }

    auto
        IsCoordinatorValid(
            const FCk_Handle_QueueCoordinator& InCoordinator)
        -> bool
    {
        return ck::IsValid(InCoordinator)
            && UCk_Utils_QueueCoordinator_UE::Has(InCoordinator);
    }

    auto
        CanAcceptRequests(
            const FCk_Handle_QueueCoordinator& InCoordinator)
        -> bool
    {
        return IsCoordinatorValid(InCoordinator)
            && NOT InCoordinator.Has<ck::FTag_DestroyEntity_Initiate>();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_QueueCoordinator_UE::
    Add(
        FCk_Handle& InOwner,
        const FCk_Fragment_QueueCoordinator_ParamsData& InParams)
    -> FCk_Handle_QueueCoordinator
{
    const auto OwnerIsValid = ck::IsValid(InOwner);
    CK_ENSURE_IF_NOT(OwnerIsValid,
        TEXT("Cannot add QueueCoordinator: owner handle is invalid"))
    {}
    if (NOT OwnerIsValid)
    { return {}; }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InOwner);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Cannot add QueueCoordinator to non-authoritative owner [{}]"),
        InOwner)
    {}
    if (NOT HasAuthority)
    { return {}; }

    const auto CoordinatorIsAbsent = NOT Has(InOwner);
    CK_ENSURE_IF_NOT(CoordinatorIsAbsent,
        TEXT("Cannot add QueueCoordinator to [{}]: feature is already present"),
        InOwner)
    {}
    if (NOT CoordinatorIsAbsent)
    { return {}; }

    const auto PolicyIsValid = ck_queue_coordinator_utils::IsSelectionPolicyValid(
        InParams.Get_SelectionPolicy());
    const auto CategoryIsValid = NOT InParams.Get_RequiredQueueCategory().IsValid()
        || InParams.Get_RequiredQueueCategory().MatchesTag(Tag_Queue_CategoryName);
    const auto ParamsAreValid = PolicyIsValid && CategoryIsValid;
    CK_ENSURE_IF_NOT(ParamsAreValid,
        TEXT("Cannot add QueueCoordinator to [{}]: selection policy or required Queue category is invalid"),
        InOwner)
    {}
    if (NOT ParamsAreValid)
    { return {}; }

    InOwner.Add<ck::FFragment_QueueCoordinator_Params>(InParams);
    InOwner.Add<ck::FFragment_QueueCoordinator_Current>();
    InOwner.Add<ck::FTag_QueueCoordinator_NeedsSetup>();
    return CastChecked(InOwner);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_QueueCoordinator_UE,
    FCk_Handle_QueueCoordinator,
    ck::FFragment_QueueCoordinator_Params,
    ck::FFragment_QueueCoordinator_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_QueueCoordinator_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_QueueCoordinator_UE::
    Get_Services(
        const FCk_Handle_QueueCoordinator& InCoordinator)
    -> TArray<FCk_QueueCoordinator_Service>
{
    const auto CoordinatorIsValid = ck_queue_coordinator_utils::IsCoordinatorValid(InCoordinator);
    CK_ENSURE_IF_NOT(CoordinatorIsValid,
        TEXT("Get_Services called with invalid QueueCoordinator [{}]"),
        InCoordinator)
    {}
    if (NOT CoordinatorIsValid)
    { return {}; }

    return InCoordinator.Get<ck::FFragment_QueueCoordinator_Current>().Get_Services();
}

auto
    UCk_Utils_QueueCoordinator_UE::
    Get_Revision(
        const FCk_Handle_QueueCoordinator& InCoordinator)
    -> int32
{
    const auto CoordinatorIsValid = ck_queue_coordinator_utils::IsCoordinatorValid(InCoordinator);
    CK_ENSURE_IF_NOT(CoordinatorIsValid,
        TEXT("Get_Revision called with invalid QueueCoordinator [{}]"),
        InCoordinator)
    {}
    if (NOT CoordinatorIsValid)
    { return 0; }

    return InCoordinator.Get<ck::FFragment_QueueCoordinator_Current>().Get_Revision();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_QueueCoordinator_UE::
    Request_RegisterQueue(
        FCk_Handle_QueueCoordinator& InCoordinator,
        const FCk_Request_QueueCoordinator_RegisterQueue& InRequest,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate)
    -> FCk_Handle_QueueCoordinator
{
    const auto CoordinatorCanAccept = ck_queue_coordinator_utils::CanAcceptRequests(InCoordinator);
    const auto HasAuthority = CoordinatorCanAccept
        && UCk_Utils_Net_UE::Get_HasAuthority(InCoordinator);
    const auto QueueIsValid = CoordinatorCanAccept
        && ck_queue_coordinator_utils::IsInSameRegistry(InRequest.Get_Queue(), InCoordinator)
        && UCk_Utils_Queue_UE::Has(InRequest.Get_Queue());
    const auto QueueHasAuthority = QueueIsValid
        && UCk_Utils_Net_UE::Get_HasAuthority(InRequest.Get_Queue());
    const auto CategoryMatches = QueueIsValid
        && (NOT InCoordinator.Get<ck::FFragment_QueueCoordinator_Params>()
                .Get_RequiredQueueCategory().IsValid()
            || UCk_Utils_Queue_UE::Get_Category(InRequest.Get_Queue())
                == InCoordinator.Get<ck::FFragment_QueueCoordinator_Params>()
                    .Get_RequiredQueueCategory());
    const auto RequestIsValid = CoordinatorCanAccept
        && HasAuthority
        && QueueIsValid
        && QueueHasAuthority
        && CategoryMatches;
    CK_ENSURE_IF_NOT(RequestIsValid,
        TEXT("Cannot enqueue QueueCoordinator RegisterQueue on [{}]: coordinator, Queue, authority, registry, or category is invalid"),
        InCoordinator)
    {}
    if (NOT RequestIsValid)
    {
        InCompletionDelegate.ExecuteIfBound(
            InCoordinator,
            ECk_Request_OperationResult::Failed_NotEnqueued);
        return InCoordinator;
    }

    if (InCompletionDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InCompletionDelegate); }
    CK_CALLSTACK_RECORD(ck::FFragment_QueueCoordinator_Requests, InCoordinator);
    InCoordinator.AddOrGet<ck::FFragment_QueueCoordinator_Requests>()._Requests.Emplace(InRequest);
    return InCoordinator;
}

auto
    UCk_Utils_QueueCoordinator_UE::
    Request_UnregisterQueue(
        FCk_Handle_QueueCoordinator& InCoordinator,
        const FCk_Request_QueueCoordinator_UnregisterQueue& InRequest,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate)
    -> FCk_Handle_QueueCoordinator
{
    const auto CoordinatorCanAccept = ck_queue_coordinator_utils::CanAcceptRequests(InCoordinator);
    const auto HasAuthority = CoordinatorCanAccept
        && UCk_Utils_Net_UE::Get_HasAuthority(InCoordinator);
    const auto QueueIsInSameRegistry = CoordinatorCanAccept
        && ck_queue_coordinator_utils::IsInSameRegistry(InRequest.Get_Queue(), InCoordinator);
    const auto RequestIsValid = CoordinatorCanAccept
        && HasAuthority
        && QueueIsInSameRegistry;
    CK_ENSURE_IF_NOT(RequestIsValid,
        TEXT("Cannot enqueue QueueCoordinator UnregisterQueue on [{}]: coordinator, authority, or Queue registry is invalid"),
        InCoordinator)
    {}
    if (NOT RequestIsValid)
    {
        InCompletionDelegate.ExecuteIfBound(
            InCoordinator,
            ECk_Request_OperationResult::Failed_NotEnqueued);
        return InCoordinator;
    }

    if (InCompletionDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InCompletionDelegate); }
    CK_CALLSTACK_RECORD(ck::FFragment_QueueCoordinator_Requests, InCoordinator);
    InCoordinator.AddOrGet<ck::FFragment_QueueCoordinator_Requests>()._Requests.Emplace(InRequest);
    return InCoordinator;
}

auto
    UCk_Utils_QueueCoordinator_UE::
    Request_SelectQueue(
        FCk_Handle_QueueCoordinator& InCoordinator,
        const FCk_Request_QueueCoordinator_SelectQueue& InRequest,
        const FCk_Delegate_QueueCoordinator_OnSelected& InResultDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate)
    -> FCk_Handle_QueueCoordinator
{
    const auto CoordinatorCanAccept = ck_queue_coordinator_utils::CanAcceptRequests(InCoordinator);
    const auto HasAuthority = CoordinatorCanAccept
        && UCk_Utils_Net_UE::Get_HasAuthority(InCoordinator);
    const auto MemberIsValid = CoordinatorCanAccept
        && ck_queue_coordinator_utils::IsInSameRegistry(InRequest.Get_Member(), InCoordinator);
    const auto LocationIsValid = NOT InRequest.Get_WorldLocation().ContainsNaN();
    const auto DelegateIsBound = InResultDelegate.IsBound();
    const auto ExclusionsAreValid = CoordinatorCanAccept
        && ck::algo::AllOf(
            InRequest.Get_ExcludedQueues(),
            [&InCoordinator](const FCk_Handle_Queue& InQueue)
            {
                return ck_queue_coordinator_utils::IsInSameRegistry(
                    InQueue,
                    InCoordinator);
            });
    const auto RequestIsValid = CoordinatorCanAccept
        && HasAuthority
        && MemberIsValid
        && LocationIsValid
        && DelegateIsBound
        && ExclusionsAreValid;
    CK_ENSURE_IF_NOT(RequestIsValid,
        TEXT("Cannot enqueue QueueCoordinator SelectQueue on [{}]: coordinator, authority, member, location, delegate, or exclusions are invalid"),
        InCoordinator)
    {}
    if (NOT RequestIsValid)
    {
        InCompletionDelegate.ExecuteIfBound(
            InCoordinator,
            ECk_Request_OperationResult::Failed_NotEnqueued);
        return InCoordinator;
    }

    auto Request = InRequest;
    Request.Set_ResultDelegate(InResultDelegate);
    if (InCompletionDelegate.IsBound())
    { Request.Set_CompletionDelegate(InCompletionDelegate); }
    CK_CALLSTACK_RECORD(ck::FFragment_QueueCoordinator_Requests, InCoordinator);
    InCoordinator.AddOrGet<ck::FFragment_QueueCoordinator_Requests>()._Requests.Emplace(
        MoveTemp(Request));
    return InCoordinator;
}

// --------------------------------------------------------------------------------------------------------------------
