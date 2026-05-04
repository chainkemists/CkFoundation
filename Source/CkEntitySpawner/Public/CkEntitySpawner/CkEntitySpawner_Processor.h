#pragma once

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEntitySpawner/CkEntitySpawner_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKENTITYSPAWNER_API FProcessor_EntitySpawner_Spawn : public TProcessor<
            FProcessor_EntitySpawner_Spawn,
            ck::TReadOnly<FFragment_EntitySpawner_PendingSpawn>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FFragment_EntitySpawner_PendingSpawn;

        // PendingSpawn is intentionally sticky: channel-not-ready relies on
        // MarkedDirtyBy re-firing next Tick. 
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_EntitySpawner_PendingSpawn& InPending) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
