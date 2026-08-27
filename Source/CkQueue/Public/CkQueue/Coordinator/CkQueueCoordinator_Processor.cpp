#include "CkQueue/Coordinator/CkQueueCoordinator_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkQueue/Queue/CkQueue_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_QueueCoordinator_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_QueueCoordinator_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_QueueCoordinator_Reconcile);
CK_REGISTER_PROCESSOR(ck::FProcessor_QueueCoordinator_EndPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_QueueCoordinator_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_queue_coordinator_processor
{
    struct FCandidate
    {
        FCk_Handle_Queue Queue;
        int32 RegistrationOrdinal = 0;
        int32 CurrentMemberCount = 0;
        int32 ProjectedMemberCount = 0;
        float DistanceUu = 0.0f;
    };

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
        IsQueueValidForCoordinator(
            const FCk_Handle_Queue& InQueue,
            const FCk_Handle_QueueCoordinator& InCoordinator)
        -> bool
    {
        return IsInSameRegistry(InQueue, InCoordinator)
            && UCk_Utils_Queue_UE::Has(InQueue);
    }

    auto
        GetPendingAdmissionCount(
            const FCk_Handle_Queue& InQueue)
        -> int32
    {
        if (NOT InQueue.Has<ck::FFragment_Queue_Requests>())
        { return 0; }

        auto Result = int32{0};
        for (const auto& Request : InQueue.Get<ck::FFragment_Queue_Requests>().Get_Requests())
        {
            if (std::holds_alternative<FCk_Request_Queue_Join>(Request)
                || std::holds_alternative<FCk_Request_Queue_RestoreJoin>(Request))
            { ++Result; }
        }
        return Result;
    }

    auto
        GetRegistrationOrdinal(
            const TArray<FCk_QueueCoordinator_Service>& InServices,
            const FCk_Handle_Queue& InQueue)
        -> int32
    {
        for (const auto& Service : InServices)
        {
            if (Service.Get_Queue() == InQueue)
            { return Service.Get_RegistrationOrdinal(); }
        }
        return 0;
    }

    auto
        IsCandidateBefore(
            const FCandidate& InA,
            const FCandidate& InB,
            ECk_QueueCoordinator_SelectionPolicy InPolicy)
        -> bool
    {
        if (InPolicy == ECk_QueueCoordinator_SelectionPolicy::NearestThenLeastMembers)
        {
            if (NOT FMath::IsNearlyEqual(InA.DistanceUu, InB.DistanceUu))
            { return InA.DistanceUu < InB.DistanceUu; }
            if (InA.ProjectedMemberCount != InB.ProjectedMemberCount)
            { return InA.ProjectedMemberCount < InB.ProjectedMemberCount; }
        }
        else
        {
            if (InA.ProjectedMemberCount != InB.ProjectedMemberCount)
            { return InA.ProjectedMemberCount < InB.ProjectedMemberCount; }
            if (NOT FMath::IsNearlyEqual(InA.DistanceUu, InB.DistanceUu))
            { return InA.DistanceUu < InB.DistanceUu; }
        }

        return InA.RegistrationOrdinal < InB.RegistrationOrdinal;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_QueueCoordinator_Setup::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InCoordinator,
            FFragment_QueueCoordinator_Current& InCurrent)
        -> void
    {
        InCurrent._Revision = 1;
        if (InCurrent._Services.IsEmpty())
        { InCoordinator.Remove<MarkedDirtyBy>(); }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_QueueCoordinator_HandleRequests::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InCoordinator,
            const FFragment_QueueCoordinator_Params& InParams,
            FFragment_QueueCoordinator_Current& InCurrent,
            FFragment_QueueCoordinator_Requests& InRequests)
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();
        auto ProjectedAdmissions = TMap<FCk_Handle_Queue, int32>{};
        for (const auto& Service : InCurrent._Services)
        {
            const auto Queue = Service.Get_Queue();
            if (ck_queue_coordinator_processor::IsQueueValidForCoordinator(Queue, InCoordinator))
            {
                ProjectedAdmissions.Add(
                    Queue,
                    ck_queue_coordinator_processor::GetPendingAdmissionCount(Queue));
            }
        }

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            auto Completion = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InCoordinator, Completion);

            if (DoHandleRequest(
                InCoordinator,
                InParams,
                InCurrent,
                ProjectedAdmissions,
                InRequest))
            { Completion = ECk_Request_OperationResult::Succeeded; }
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        { InCoordinator.Remove<MarkedDirtyBy>(); }
    }

    auto
        FProcessor_QueueCoordinator_HandleRequests::
        DoHandleRequest(
            HandleType InCoordinator,
            const FFragment_QueueCoordinator_Params& InParams,
            FFragment_QueueCoordinator_Current& InCurrent,
            TMap<FCk_Handle_Queue, int32>& /*InOutProjectedAdmissions*/,
            const FCk_Request_QueueCoordinator_RegisterQueue& InRequest)
        -> bool
    {
        const auto Queue = InRequest.Get_Queue();
        const auto QueueIsValid = ck_queue_coordinator_processor::IsQueueValidForCoordinator(
            Queue,
            InCoordinator);
        const auto QueueHasAuthority = QueueIsValid
            && UCk_Utils_Net_UE::Get_HasAuthority(Queue);
        const auto CategoryMatches = QueueIsValid
            && (NOT InParams.Get_RequiredQueueCategory().IsValid()
                || UCk_Utils_Queue_UE::Get_Category(Queue) == InParams.Get_RequiredQueueCategory());
        const auto RequestIsValid = QueueIsValid
            && QueueHasAuthority
            && CategoryMatches;
        CK_ENSURE_IF_NOT(RequestIsValid,
            TEXT("QueueCoordinator [{}] cannot register Queue [{}]: registry, authority, feature, or category is invalid"),
            InCoordinator,
            Queue)
        {}
        if (NOT RequestIsValid)
        { return false; }

        const auto AlreadyRegistered = InCurrent._Services.ContainsByPredicate(
            [&Queue](const FCk_QueueCoordinator_Service& InService)
            { return InService.Get_Queue() == Queue; });
        if (AlreadyRegistered)
        { return true; }

        InCurrent._Services.Emplace(Queue, InCurrent._NextRegistrationOrdinal++);
        ++InCurrent._Revision;
        InCoordinator.AddOrGet<FTag_QueueCoordinator_NeedsReconcile>();
        return true;
    }

    auto
        FProcessor_QueueCoordinator_HandleRequests::
        DoHandleRequest(
            HandleType InCoordinator,
            const FFragment_QueueCoordinator_Params& /*InParams*/,
            FFragment_QueueCoordinator_Current& InCurrent,
            TMap<FCk_Handle_Queue, int32>& /*InOutProjectedAdmissions*/,
            const FCk_Request_QueueCoordinator_UnregisterQueue& InRequest)
        -> bool
    {
        const auto Queue = InRequest.Get_Queue();
        const auto QueueIsInSameRegistry = ck_queue_coordinator_processor::IsInSameRegistry(
            Queue,
            InCoordinator);
        CK_ENSURE_IF_NOT(QueueIsInSameRegistry,
            TEXT("QueueCoordinator [{}] cannot unregister foreign or invalid Queue [{}]"),
            InCoordinator,
            Queue)
        {}
        if (NOT QueueIsInSameRegistry)
        { return false; }

        const auto RemovedCount = InCurrent._Services.RemoveAll(
            [&Queue](const FCk_QueueCoordinator_Service& InService)
            { return InService.Get_Queue() == Queue; });
        if (RemovedCount > 0)
        { ++InCurrent._Revision; }
        return true;
    }

    auto
        FProcessor_QueueCoordinator_HandleRequests::
        DoHandleRequest(
            HandleType InCoordinator,
            const FFragment_QueueCoordinator_Params& InParams,
            FFragment_QueueCoordinator_Current& InCurrent,
            TMap<FCk_Handle_Queue, int32>& InOutProjectedAdmissions,
            const FCk_Request_QueueCoordinator_SelectQueue& InRequest)
        -> bool
    {
        auto ResultDelegate = InRequest.Get_ResultDelegate();
        const auto RequestIsValid = ck_queue_coordinator_processor::IsInSameRegistry(
                InRequest.Get_Member(),
                InCoordinator)
            && NOT InRequest.Get_WorldLocation().ContainsNaN()
            && ResultDelegate.IsBound();
        CK_ENSURE_IF_NOT(RequestIsValid,
            TEXT("QueueCoordinator [{}] cannot select for member [{}]: member, location, or result delegate is invalid"),
            InCoordinator,
            InRequest.Get_Member())
        {}
        if (NOT RequestIsValid)
        {
            ResultDelegate.ExecuteIfBound(FCk_QueueCoordinator_SelectResult{
                ECk_QueueCoordinator_SelectOutcome::NoEligibleQueue,
                InRequest.Get_Member(),
                {},
                InCurrent._Revision,
                0,
                0,
                0.0f,
                {}});
            return false;
        }

        const auto RemovedInvalidServices = InCurrent._Services.RemoveAll(
            [&InCoordinator](const FCk_QueueCoordinator_Service& InService)
            {
                return NOT ck_queue_coordinator_processor::IsQueueValidForCoordinator(
                    InService.Get_Queue(),
                    InCoordinator)
                    || NOT UCk_Utils_Queue_UE::Get_CanAcceptRequests(InService.Get_Queue());
            });
        if (RemovedInvalidServices > 0)
        { ++InCurrent._Revision; }

        auto ExistingQueues = TArray<FCk_Handle_Queue>{};
        for (const auto& Service : InCurrent._Services)
        {
            const auto Queue = Service.Get_Queue();
            if (ck_queue_coordinator_processor::IsQueueValidForCoordinator(Queue, InCoordinator)
                && UCk_Utils_Queue_UE::Get_CanAcceptRequests(Queue)
                && UCk_Utils_Queue_UE::Get_IsMember(Queue, InRequest.Get_Member()))
            { ExistingQueues.Add(Queue); }
        }

        const auto MemberIsInMultipleQueues = ExistingQueues.Num() > 1;
        CK_ENSURE_IF_NOT(NOT MemberIsInMultipleQueues,
            TEXT("QueueCoordinator [{}] member [{}] is in [{}] registered Queues"),
            InCoordinator,
            InRequest.Get_Member(),
            ExistingQueues.Num())
        {}
        if (MemberIsInMultipleQueues)
        {
            ResultDelegate.Execute(FCk_QueueCoordinator_SelectResult{
                ECk_QueueCoordinator_SelectOutcome::MemberInMultipleQueues,
                InRequest.Get_Member(),
                {},
                InCurrent._Revision,
                0,
                0,
                0.0f,
                {}});
            return false;
        }

        if (ExistingQueues.Num() == 1)
        {
            const auto Queue = ExistingQueues[0];
            const auto Pressure = UCk_Utils_Queue_UE::Get_Pressure(Queue);
            const auto QueueTransform = UCk_Utils_Transform_UE::CastChecked(Queue);
            const auto DistanceUu = static_cast<float>(FVector::Dist(
                InRequest.Get_WorldLocation(),
                UCk_Utils_Transform_UE::Get_EntityCurrentLocation(QueueTransform)));
            ResultDelegate.Execute(FCk_QueueCoordinator_SelectResult{
                ECk_QueueCoordinator_SelectOutcome::AlreadyQueued,
                InRequest.Get_Member(),
                Queue,
                InCurrent._Revision,
                ck_queue_coordinator_processor::GetRegistrationOrdinal(InCurrent._Services, Queue),
                Pressure.Get_MemberCount(),
                DistanceUu,
                {Queue}});
            return true;
        }

        auto Candidates = TArray<ck_queue_coordinator_processor::FCandidate>{};
        Candidates.Reserve(InCurrent._Services.Num());
        for (const auto& Service : InCurrent._Services)
        {
            const auto Queue = Service.Get_Queue();
            if (InRequest.Get_ExcludedQueues().Contains(Queue)
                || NOT ck_queue_coordinator_processor::IsQueueValidForCoordinator(Queue, InCoordinator)
                || NOT UCk_Utils_Queue_UE::Get_CanAcceptRequests(Queue))
            { continue; }

            const auto QueueHasTransform = UCk_Utils_Transform_UE::Has(Queue);
            CK_ENSURE_IF_NOT(QueueHasTransform,
                TEXT("QueueCoordinator [{}] registered Queue [{}] lost its required Transform"),
                InCoordinator,
                Queue)
            {}
            if (NOT QueueHasTransform)
            { continue; }

            const auto Pressure = UCk_Utils_Queue_UE::Get_Pressure(Queue);
            const auto CurrentMemberCount = Pressure.Get_MemberCount();
            const auto ProjectedMemberCount = CurrentMemberCount
                + InOutProjectedAdmissions.FindRef(Queue)
                + 1;
            const auto WouldExceedHardLimit = Pressure.Get_HardLimit() > 0
                && ProjectedMemberCount > Pressure.Get_HardLimit();
            if (WouldExceedHardLimit)
            { continue; }

            const auto QueueLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(
                UCk_Utils_Transform_UE::CastChecked(Queue));
            Candidates.Emplace(ck_queue_coordinator_processor::FCandidate{
                Queue,
                Service.Get_RegistrationOrdinal(),
                CurrentMemberCount,
                ProjectedMemberCount,
                static_cast<float>(FVector::Dist(InRequest.Get_WorldLocation(), QueueLocation))});
        }

        Candidates.Sort(
            [&InParams](const auto& InA, const auto& InB)
            {
                return ck_queue_coordinator_processor::IsCandidateBefore(
                    InA,
                    InB,
                    InParams.Get_SelectionPolicy());
            });

        auto FallbackQueues = TArray<FCk_Handle_Queue>{};
        FallbackQueues.Reserve(Candidates.Num());
        for (const auto& Candidate : Candidates)
        { FallbackQueues.Add(Candidate.Queue); }

        if (Candidates.IsEmpty())
        {
            ResultDelegate.Execute(FCk_QueueCoordinator_SelectResult{
                InCurrent._Services.IsEmpty()
                    ? ECk_QueueCoordinator_SelectOutcome::NoRegisteredQueues
                    : ECk_QueueCoordinator_SelectOutcome::NoEligibleQueue,
                InRequest.Get_Member(),
                {},
                InCurrent._Revision,
                0,
                0,
                0.0f,
                MoveTemp(FallbackQueues)});
            return false;
        }

        const auto& Selected = Candidates[0];
        ++InOutProjectedAdmissions.FindOrAdd(Selected.Queue);
        ResultDelegate.Execute(FCk_QueueCoordinator_SelectResult{
            ECk_QueueCoordinator_SelectOutcome::Selected,
            InRequest.Get_Member(),
            Selected.Queue,
            InCurrent._Revision,
            Selected.RegistrationOrdinal,
            Selected.ProjectedMemberCount,
            Selected.DistanceUu,
            MoveTemp(FallbackQueues)});
        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_QueueCoordinator_Reconcile::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InCoordinator,
            FFragment_QueueCoordinator_Current& InCurrent)
        -> void
    {
        const auto RemovedCount = InCurrent._Services.RemoveAll(
            [&InCoordinator](const FCk_QueueCoordinator_Service& InService)
            {
                return NOT ck_queue_coordinator_processor::IsQueueValidForCoordinator(
                    InService.Get_Queue(),
                    InCoordinator)
                    || NOT UCk_Utils_Queue_UE::Get_CanAcceptRequests(InService.Get_Queue());
            });
        if (RemovedCount > 0)
        { ++InCurrent._Revision; }
        InCoordinator.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_QueueCoordinator_EndPlay::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType /*InCoordinator*/,
            FFragment_QueueCoordinator_Current& InCurrent)
        -> void
    {
        ++InCurrent._Revision;
        InCurrent._Services.Reset();
    }

    auto
        FProcessor_QueueCoordinator_CancelPendingRequests::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InCoordinator,
            const FFragment_QueueCoordinator_Requests& InRequests)
        -> void
    {
        algo::ForEachRequest(
            InRequests.Get_Requests(),
            ck::Visitor(
            [&InCoordinator](const auto& InRequest) -> void
            {
                using RequestType = std::decay_t<decltype(InRequest)>;
                if constexpr (std::is_same_v<RequestType, FCk_Request_QueueCoordinator_SelectQueue>)
                {
                    auto ResultDelegate = InRequest.Get_ResultDelegate();
                    ResultDelegate.ExecuteIfBound(FCk_QueueCoordinator_SelectResult{
                        ECk_QueueCoordinator_SelectOutcome::Cancelled,
                        InRequest.Get_Member(),
                        {},
                        0,
                        0,
                        0,
                        0.0f,
                        {}});
                }
            }),
            policy::DontResetContainer{});
        request::FireCancelledForPending(
            InCoordinator,
            InRequests.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
