#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkRelationship/Player/CkPlayer_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKRELATIONSHIP_API FProcessor_Player_RetryPendingReplication : public ck_exp::TProcessor<
        FProcessor_Player_RetryPendingReplication,
        FCk_Handle,
        ck::TReadWrite<FFragment_Player_PendingReplication>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Player_PendingReplication& InPending) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
