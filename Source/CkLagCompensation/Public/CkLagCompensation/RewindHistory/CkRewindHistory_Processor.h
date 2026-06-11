#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkLagCompensation/RewindHistory/CkRewindHistory_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Snapshots the frame-final poses of every RewindHistory entity's hit shapes into its ring
    // buffer. Runs in FGroup_PostTransform so the recorded poses are exactly what this frame
    // presents to the world (and to remote clients). Records on the authority only — clients
    // never rewind, they get rewound against
    class CKLAGCOMPENSATION_API FProcessor_RewindHistory_Record : public ck_exp::TProcessor<
            FProcessor_RewindHistory_Record,
            FCk_Handle_RewindHistory,
            ck::TReadOnly<FFragment_RewindHistory_Params>,
            ck::TReadWrite<FFragment_RewindHistory_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_RewindHistory_Params& InParams,
            FFragment_RewindHistory_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
