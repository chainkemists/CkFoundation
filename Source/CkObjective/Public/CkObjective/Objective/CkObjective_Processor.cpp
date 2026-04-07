#include "CkObjective_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkAttribute/ByteAttribute/CkByteAttribute_Utils.h"
#include "CkObjective/Objective/CkObjective_Utils.h"
#include "CkObjective/ObjectiveOwner/CkObjectiveOwner_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Objective_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Objective_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Objective_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_objective
{
    constexpr uint8 StatusEnumToByte(ECk_ObjectiveStatus InStatus)
    {
        return static_cast<uint8>(InStatus);
    }

    constexpr ECk_ObjectiveStatus ByteToStatusEnum(uint8 InStatus)
    {
        return static_cast<ECk_ObjectiveStatus>(InStatus);
    }

    // ----
    auto
        OnObjectiveStatusAttributeChanged(
            const FCk_Handle& InAttributeOwnerEntity,
            const ck::TPayload_Attribute_OnValueChanged<ck::FFragment_ByteAttribute_Current>& InPayload)
        -> void
    {
        const auto& Objective = UCk_Utils_Objective_UE::Cast(InAttributeOwnerEntity);
        const auto& ObjectiveOwner = UCk_Utils_ObjectiveOwner_UE::Cast(UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Objective));
        const auto& NewStatus = ByteToStatusEnum(InPayload.Get_FinalValue());

        ck::UUtils_Signal_OnObjective_StatusChanged::Broadcast(Objective, ck::MakePayload(Objective, NewStatus));
        ck::UUtils_Signal_OnObjectiveOwner_ObjectiveStatusChanged::Broadcast(ObjectiveOwner, ck::MakePayload(ObjectiveOwner, Objective, NewStatus));
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Objective_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Objective_Params& InParams,
            FFragment_Objective_Current& InCurrent)
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        const auto& StatusAttribute = InCurrent.Get_StatusAttribute();
        UUtils_Signal_OnByteAttributeValueChanged_Current::Bind<&ck_objective::OnObjectiveStatusAttributeChanged>(
            StatusAttribute, ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame, ECk_Signal_PostFireBehavior::DoNothing);
    }

    // --------------------------------------------------------------------------------------------------------------------

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

        // TODO: The tag does not carry over through OnObjectiveStatusAttributeChanged on the client
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

        // TODO: The tag does not carry over through OnObjectiveStatusAttributeChanged on the client
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
            ck_objective::StatusEnumToByte(NewStatus),
            ECk_MinMaxCurrent::Current);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Objective_EndPlay::
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