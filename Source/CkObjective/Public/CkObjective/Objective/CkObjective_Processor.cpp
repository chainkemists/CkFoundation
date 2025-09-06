#include "CkObjective_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkAttribute/ByteAttribute/CkByteAttribute_Utils.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"
#include "CkObjective/CkObjective_Log.h"
#include "CkObjective/Objective/CkObjective_Utils.h"
#include "CkObjective/ObjectiveOwner/CkObjectiveOwner_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    // Helper function to convert status enum to byte value
    constexpr uint8 StatusEnumToByte(ECk_ObjectiveStatus InStatus)
    {
        return static_cast<uint8>(InStatus);
    }
}

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
        if (const auto& CurrentStatus = UCk_Utils_Objective_UE::Get_Status(InHandle);
            CurrentStatus == ECk_ObjectiveStatus::NotStarted)
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
        if (const auto& CurrentStatus = UCk_Utils_Objective_UE::Get_Status(InHandle);
            CurrentStatus != ECk_ObjectiveStatus::Active)
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
        if (const auto& CurrentStatus = UCk_Utils_Objective_UE::Get_Status(InHandle);
            CurrentStatus != ECk_ObjectiveStatus::Active)
        { return; }

        InCurrent._FailureTag = InRequest.Get_MetaData();
        DoSetStatus(InHandle, InCurrent, ECk_ObjectiveStatus::Failed);

        UUtils_Signal_OnObjective_Failed::Broadcast(InHandle,
            MakePayload(InHandle, InRequest.Get_MetaData()));
    }

    auto
        FProcessor_Objective_HandleRequests::
        DoSetStatus(
            HandleType InHandle,
            FFragment_Objective_Current& InCurrent,
            ECk_ObjectiveStatus NewStatus)
        -> void
    {
        if (const auto& CurrentStatus = UCk_Utils_Objective_UE::Get_Status(InHandle);
            CurrentStatus == NewStatus)
        { return; }

        auto StatusAttribute = InCurrent.Get_StatusAttribute();
        UCk_Utils_ByteAttribute_UE::Request_Override(
            StatusAttribute,
            StatusEnumToByte(NewStatus),
            ECk_MinMaxCurrent::Current);

        // TODO: have the attribute signal drive the objective signal
        UUtils_Signal_OnObjective_StatusChanged::Broadcast(InHandle,
            MakePayload(InHandle, NewStatus));

        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        auto ObjectiveOwner = UCk_Utils_ObjectiveOwner_UE::CastChecked(LifetimeOwner);

        UUtils_Signal_OnObjectiveOwner_ObjectiveStatusChanged::Broadcast(ObjectiveOwner, MakePayload(ObjectiveOwner, InHandle, NewStatus));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Objective_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Objective_Current& InCurrent)
            -> void
    {
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        auto ObjectiveOwner = UCk_Utils_ObjectiveOwner_UE::CastChecked(LifetimeOwner);

        UCk_Utils_ObjectiveOwner_UE::Request_RemoveObjective(ObjectiveOwner, FCk_Request_ObjectiveOwner_RemoveObjective{InHandle});
    }
}

// --------------------------------------------------------------------------------------------------------------------