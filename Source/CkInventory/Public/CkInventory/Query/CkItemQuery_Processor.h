#pragma once

#include "CkInventory/Query/CkItemQuery_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_ItemQuery_Subsystem_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Drains item-definition query requests. Unlike most request processors this
    // does NOT gate on MarkedDirtyBy: a request whose definition index isn't
    // built yet is left pending and re-evaluated every tick until the async
    // build finishes, at which point the result signal is broadcast and the
    // request entity destroyed.
    //
    // The definition-index subsystem is cached at construction (resolved by the
    // registration factory) rather than looked up per entity — mirrors how
    // FProcessor_Probe_* cache the JPH::PhysicsSystem.
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
}

// --------------------------------------------------------------------------------------------------------------------
