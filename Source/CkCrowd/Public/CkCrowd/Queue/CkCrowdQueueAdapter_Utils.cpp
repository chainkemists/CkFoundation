#include "CkCrowd/Queue/CkCrowdQueueAdapter_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkQueue/Queue/CkQueue_Utils.h"

auto
    UCk_Utils_CrowdQueueAdapter_UE::
    Request_JoinQueue(
        FCk_Handle_CrowdAgent& InAgent,
        FCk_Handle_Queue& InQueue,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_CrowdAgent
{
    const auto AgentIsValid = ck::IsValid(InAgent);
    const auto QueueIsValid = ck::IsValid(InQueue) && UCk_Utils_Queue_UE::Has(InQueue);
    const auto HasAuthority = AgentIsValid && QueueIsValid && UCk_Utils_Net_UE::Get_HasAuthority(InAgent);
    auto HasCompatibleAdapter = NOT AgentIsValid || NOT InAgent.Has<ck::FFragment_CrowdQueueAdapter>();
    if (AgentIsValid && InAgent.Has<ck::FFragment_CrowdQueueAdapter>())
    {
        const auto& Existing = InAgent.Get<ck::FFragment_CrowdQueueAdapter>();
        auto ExistingMember = FCk_Queue_MemberSnapshot{};
        const auto ExistingQueueIsValid = ck::IsValid(Existing.Get_Queue())
            && UCk_Utils_Queue_UE::Has(Existing.Get_Queue());
        const auto ExistingMembership = ExistingQueueIsValid
            && UCk_Utils_Queue_UE::TryGet_MemberSnapshot(
                Existing.Get_Queue(),
                FCk_Handle{InAgent},
                ExistingMember);
        const auto PriorRejectionIsObservable = ExistingQueueIsValid
            && Existing.Get_JoinPending()
            && NOT ExistingMembership
            && UCk_Utils_Queue_UE::Get_Revision(Existing.Get_Queue())
                > Existing.Get_PendingQueueRevision();
        const auto CompletedLeaveIsObservable = InAgent.Has<ck::FTag_CrowdQueueAdapter_LeaveRequested>()
            && NOT ExistingMembership;
        const auto EstablishedMembershipIsAbsent = ExistingQueueIsValid
            && NOT Existing.Get_JoinPending()
            && NOT ExistingMembership;
        HasCompatibleAdapter = Existing.Get_Queue() == InQueue
            || NOT ExistingQueueIsValid
            || PriorRejectionIsObservable
            || CompletedLeaveIsObservable
            || EstablishedMembershipIsAbsent;
    }
    CK_ENSURE_IF_NOT(AgentIsValid && QueueIsValid && HasAuthority && HasCompatibleAdapter,
        TEXT("Cannot join CrowdAgent [{}] to Queue [{}]: invalid agent, queue, authority, or conflicting adapter"), InAgent, InQueue)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    auto& Adapter = InAgent.AddOrGet<ck::FFragment_CrowdQueueAdapter>();
    if (Adapter._Queue != InQueue)
    {
        const auto ActiveCorrelation = UCk_Utils_CrowdAgent_UE::Get_ActiveMoveCorrelationId(InAgent);
        if (Adapter._IssuedCrowdCorrelationId != 0
            && ActiveCorrelation == Adapter._IssuedCrowdCorrelationId)
        { UCk_Utils_CrowdAgent_UE::Request_Stop(InAgent, {}); }

        Adapter._IssuedQueueAssignmentRevision = 0;
        Adapter._IssuedCrowdCorrelationId = 0;
        Adapter._ReportedQueueAssignmentRevision = 0;
    }
    Adapter._Queue = InQueue;
    Adapter._JoinPending = true;
    Adapter._PendingQueueRevision = UCk_Utils_Queue_UE::Get_Revision(InQueue);
    InAgent.Try_Remove<ck::FTag_CrowdQueueAdapter_LeaveRequested>();

    auto Request = FCk_Request_Queue_Join{FCk_Handle{InAgent}};
    Request.Set_Mover(FCk_Handle{InAgent});
    UCk_Utils_Queue_UE::Request_Join(InQueue, Request, InDelegate);
    return InAgent;
}

auto
    UCk_Utils_CrowdQueueAdapter_UE::
    Request_RestoreJoinQueue(
        FCk_Handle_CrowdAgent& InAgent,
        FCk_Handle_Queue& InQueue,
        int64 InRestoredTicket,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_CrowdAgent
{
    const auto AgentIsValid = ck::IsValid(InAgent);
    const auto QueueIsValid = ck::IsValid(InQueue) && UCk_Utils_Queue_UE::Has(InQueue);
    const auto HasAuthority = AgentIsValid && QueueIsValid && UCk_Utils_Net_UE::Get_HasAuthority(InAgent);
    const auto TicketIsValid = InRestoredTicket > 0 && InRestoredTicket < MAX_int64;
    auto HasCompatibleAdapter = NOT AgentIsValid || NOT InAgent.Has<ck::FFragment_CrowdQueueAdapter>();
    if (AgentIsValid && InAgent.Has<ck::FFragment_CrowdQueueAdapter>())
    {
        const auto& Existing = InAgent.Get<ck::FFragment_CrowdQueueAdapter>();
        auto ExistingMember = FCk_Queue_MemberSnapshot{};
        const auto ExistingQueueIsValid = ck::IsValid(Existing.Get_Queue())
            && UCk_Utils_Queue_UE::Has(Existing.Get_Queue());
        const auto ExistingMembership = ExistingQueueIsValid
            && UCk_Utils_Queue_UE::TryGet_MemberSnapshot(
                Existing.Get_Queue(), FCk_Handle{InAgent}, ExistingMember);
        const auto PriorRejectionIsObservable = ExistingQueueIsValid
            && Existing.Get_JoinPending()
            && NOT ExistingMembership
            && UCk_Utils_Queue_UE::Get_Revision(Existing.Get_Queue())
                > Existing.Get_PendingQueueRevision();
        const auto CompletedLeaveIsObservable = InAgent.Has<ck::FTag_CrowdQueueAdapter_LeaveRequested>()
            && NOT ExistingMembership;
        const auto EstablishedMembershipIsAbsent = ExistingQueueIsValid
            && NOT Existing.Get_JoinPending()
            && NOT ExistingMembership;
        HasCompatibleAdapter = Existing.Get_Queue() == InQueue
            || NOT ExistingQueueIsValid
            || PriorRejectionIsObservable
            || CompletedLeaveIsObservable
            || EstablishedMembershipIsAbsent;
    }
    CK_ENSURE_IF_NOT(AgentIsValid && QueueIsValid && HasAuthority && TicketIsValid && HasCompatibleAdapter,
        TEXT("Cannot restore join CrowdAgent [{}] to Queue [{}]: invalid agent, queue, authority, ticket, or adapter"),
        InAgent, InQueue)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    auto& Adapter = InAgent.AddOrGet<ck::FFragment_CrowdQueueAdapter>();
    if (Adapter._Queue != InQueue)
    {
        const auto ActiveCorrelation = UCk_Utils_CrowdAgent_UE::Get_ActiveMoveCorrelationId(InAgent);
        if (Adapter._IssuedCrowdCorrelationId != 0
            && ActiveCorrelation == Adapter._IssuedCrowdCorrelationId)
        { UCk_Utils_CrowdAgent_UE::Request_Stop(InAgent, {}); }

        Adapter._IssuedQueueAssignmentRevision = 0;
        Adapter._IssuedCrowdCorrelationId = 0;
        Adapter._ReportedQueueAssignmentRevision = 0;
    }
    Adapter._Queue = InQueue;
    Adapter._JoinPending = true;
    Adapter._PendingQueueRevision = UCk_Utils_Queue_UE::Get_Revision(InQueue);
    InAgent.Try_Remove<ck::FTag_CrowdQueueAdapter_LeaveRequested>();

    auto Request = FCk_Request_Queue_RestoreJoin{FCk_Handle{InAgent}, InRestoredTicket};
    Request.Set_Mover(FCk_Handle{InAgent});
    UCk_Utils_Queue_UE::Request_RestoreJoin(InQueue, Request, InDelegate);
    return InAgent;
}

auto
    UCk_Utils_CrowdQueueAdapter_UE::
    Request_LeaveQueue(
        FCk_Handle_CrowdAgent& InAgent,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_CrowdAgent
{
    const auto AgentIsValid = ck::IsValid(InAgent);
    const auto HasAdapter = AgentIsValid && InAgent.Has<ck::FFragment_CrowdQueueAdapter>();
    CK_ENSURE_IF_NOT(HasAdapter,
        TEXT("Cannot leave Queue: CrowdAgent [{}] has no Queue adapter"), InAgent)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    auto& Adapter = InAgent.Get<ck::FFragment_CrowdQueueAdapter>();
    auto Queue = Adapter.Get_Queue();
    const auto QueueIsValid = ck::IsValid(Queue) && UCk_Utils_Queue_UE::Has(Queue);
    const auto HasAuthority = QueueIsValid && UCk_Utils_Net_UE::Get_HasAuthority(InAgent);
    CK_ENSURE_IF_NOT(QueueIsValid && HasAuthority,
        TEXT("Cannot leave Queue from CrowdAgent [{}]: queue is invalid or non-authoritative"), InAgent)
    {
        InDelegate.ExecuteIfBound(InAgent, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InAgent;
    }

    InAgent.AddOrGet<ck::FTag_CrowdQueueAdapter_LeaveRequested>();
    Adapter._JoinPending = false;
    const auto ActiveCorrelation = UCk_Utils_CrowdAgent_UE::Get_ActiveMoveCorrelationId(InAgent);
    if (Adapter.Get_IssuedCrowdCorrelationId() != 0 && ActiveCorrelation == Adapter.Get_IssuedCrowdCorrelationId())
    { UCk_Utils_CrowdAgent_UE::Request_Stop(InAgent, {}); }

    UCk_Utils_Queue_UE::Request_Leave(Queue, FCk_Request_Queue_Leave{FCk_Handle{InAgent}}, InDelegate);
    return InAgent;
}
