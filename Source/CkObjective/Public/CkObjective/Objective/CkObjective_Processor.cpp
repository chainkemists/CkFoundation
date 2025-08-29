#include "CkObjective_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Objective_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            const FFragment_Objective_Params& InParams,
            const FFragment_Objective_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_Objective_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InCurrent, InParams, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_Objective_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            const FFragment_Objective_Params& InParams,
            const FCk_Request_Objective_Start& InRequest)
        -> void
    {
        if (InCurrent._Status == ECk_ObjectiveStatus::NotStarted)
        {
            DoSetStatus(InHandle, InCurrent, ECk_ObjectiveStatus::Active);
        }
    }

    auto
        FProcessor_Objective_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            const FFragment_Objective_Params& InParams,
            const FCk_Request_Objective_Complete& InRequest)
        -> void
    {
        if (InCurrent._Status != ECk_ObjectiveStatus::Active)
        { return; }

        InCurrent._CompletionTag = InRequest.Get_MetaData();
        DoSetStatus(InHandle, InCurrent, ECk_ObjectiveStatus::Completed);

        UUtils_Signal_OnObjective_Completed::Broadcast(InHandle,
            MakePayload(InHandle, InRequest.Get_MetaData()));
    }

    auto
        FProcessor_Objective_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            const FFragment_Objective_Params& InParams,
            const FCk_Request_Objective_Fail& InRequest)
        -> void
    {
        if (InCurrent._Status != ECk_ObjectiveStatus::Active)
        { return; }

        InCurrent._FailureTag = InRequest.Get_MetaData();
        DoSetStatus(InHandle, InCurrent, ECk_ObjectiveStatus::Failed);

        UUtils_Signal_OnObjective_Failed::Broadcast(InHandle,
            MakePayload(InHandle, InRequest.Get_MetaData()));
    }

    auto
        FProcessor_Objective_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            const FFragment_Objective_Params& InParams,
            const FCk_Request_Objective_UpdateProgress& InRequest)
        -> void
    {
        if (InCurrent._Status != ECk_ObjectiveStatus::Active)
        { return; }

        DoSetProgress(InHandle, InCurrent, InRequest.Get_MetaData(), InRequest.Get_NewProgress());
    }

    auto
        FProcessor_Objective_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            const FFragment_Objective_Params& InParams,
            const FCk_Request_Objective_AddProgress& InRequest)
        -> void
    {
        if (InCurrent._Status != ECk_ObjectiveStatus::Active)
        { return; }

        if (InRequest.Get_ProgressDelta() == 0)
        { return; }

        const auto NewProgress = InCurrent._Progress + InRequest.Get_ProgressDelta();
        DoSetProgress(InHandle, InCurrent, InRequest.Get_MetaData(), NewProgress);
    }

    auto
        FProcessor_Objective_HandleRequests::
        DoSetStatus(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            ECk_ObjectiveStatus NewStatus)
        -> void
    {
        if (InCurrent._Status == NewStatus)
        { return; }

        InCurrent._Status = NewStatus;

        UUtils_Signal_OnObjective_StatusChanged::Broadcast(InHandle,
            MakePayload(InHandle, InCurrent.Get_Status()));
    }

    auto
        FProcessor_Objective_HandleRequests::
        DoSetProgress(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            FGameplayTag InMetaData,
            int32 NewProgress)
        -> void
    {
        if (InCurrent._Progress == NewProgress)
        { return; }

        const auto OldProgress = InCurrent._Progress;
        InCurrent._Progress = NewProgress;

        UUtils_Signal_OnObjective_ProgressChanged::Broadcast(InHandle,
            MakePayload(InHandle, InMetaData, OldProgress, NewProgress));
    }
}

// --------------------------------------------------------------------------------------------------------------------