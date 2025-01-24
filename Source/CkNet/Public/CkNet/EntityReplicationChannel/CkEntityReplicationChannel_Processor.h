#pragma once

#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkNet/EntityReplicationChannel/CkEntityReplicationChannel_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKNET_API FProcessor_EntityReplicationChannel_Setup : public ck_exp::TProcessor<
        FProcessor_EntityReplicationChannel_Setup,
        FCk_Handle_EntityReplicationChannel,
        FFragment_OwningActor_Current,
        FTag_EntityReplicationChannel,
        FTag_EntityReplicationChannel_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_EntityReplicationChannel_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        static auto
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            const FFragment_OwningActor_Current& InOwningActor) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------