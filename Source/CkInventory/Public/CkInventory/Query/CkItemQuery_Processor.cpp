#include "CkItemQuery_Processor.h"

#include "CkInventory/Query/CkItemQuery_Subsystem.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR_WITH_FACTORY(ck::FProcessor_ItemQuery_HandleRequests,
    [](const FCk_Registry& InRegistry) -> ck::concepts::FTickableType
    {
        return ck::FProcessor_ItemQuery_HandleRequests{InRegistry, UCk_ItemQuery_Subsystem_UE::Get()};
    });

CK_REGISTER_PROCESSOR(ck::FProcessor_ItemQuery_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FProcessor_ItemQuery_HandleRequests::
    FProcessor_ItemQuery_HandleRequests(
        const RegistryType& InRegistry,
        UCk_ItemQuery_Subsystem_UE* InSubsystem)
        : TProcessor(InRegistry)
        , _Subsystem(InSubsystem) {}

    auto
        FProcessor_ItemQuery_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ItemQuery_Requests& InRequestsComp) const
        -> void
    {
        auto* Subsystem = _Subsystem.Get();

        if (ck::Is_NOT_Valid(Subsystem) || NOT Subsystem->Get_IsIndexReady())
        {
            // Index not ready — kick the build and leave the request pending;
            // we re-evaluate next tick (no MarkedDirtyBy gating).
            if (ck::IsValid(Subsystem))
            { Subsystem->Request_BuildIndex(); }
            return;
        }

        for (const auto& Request : InRequestsComp.Get_Requests())
        {
            const auto Result = FCk_ItemQuery_Result{}
                .Set_Definitions(Subsystem->Query_Definitions(Request.Get_Filter()));

            UUtils_Signal_OnItemDefinitionsQueried::Broadcast(InHandle, MakePayload(Result));

            Request.TryFireCompletion(InHandle, ECk_Request_OperationResult::Succeeded);
        }

        InHandle.Remove<FFragment_ItemQuery_Requests>();
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ItemQuery_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ItemQuery_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }
}

// --------------------------------------------------------------------------------------------------------------------
