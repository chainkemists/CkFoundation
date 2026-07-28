#pragma once

#include "CkInventory/Query/CkItemQuery_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_ItemQuery_Subsystem_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Deliberately does NOT gate on MarkedDirtyBy: a request whose definition index isn't built
    // yet stays pending and is re-evaluated every tick until the async build finishes.
    class CKINVENTORY_API FProcessor_ItemQuery_HandleRequests : public TProcessor<
            FProcessor_ItemQuery_HandleRequests,
            TReadWrite<FFragment_ItemQuery_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;

    public:
        FProcessor_ItemQuery_HandleRequests(
            const RegistryType& InRegistry,
            UCk_ItemQuery_Subsystem_UE* InSubsystem);

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_ItemQuery_Requests& InRequestsComp) const -> void;

    private:
        TWeakObjectPtr<UCk_ItemQuery_Subsystem_UE> _Subsystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // A query whose definition index never becomes ready stays pending forever; if its request entity
    // is torn down first, this completes each queued request with Failed_Cancelled so a caller awaiting
    // completion terminates instead of hanging.
    class CKINVENTORY_API FProcessor_ItemQuery_CancelPendingRequests : public TProcessor<
            FProcessor_ItemQuery_CancelPendingRequests,
            TReadOnly<FFragment_ItemQuery_Requests>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ItemQuery_Requests& InRequestsComp) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
