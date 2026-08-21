#pragma once

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkQueue/Queue/CkQueue_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Queue_Formation;

    class CKQUEUE_API FProcessor_Queue_DebugDraw : public ck_exp::TProcessor<
        FProcessor_Queue_DebugDraw,
        FCk_Handle_Queue,
        ck::TReadOnly<FFragment_Transform>,
        ck::TReadOnly<FFragment_Queue_Params>,
        ck::TReadOnly<FFragment_Queue_Current>,
        TExclude<FTag_Queue_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_Queue_Formation>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InQueue,
            const FFragment_Transform& InTransform,
            const FFragment_Queue_Params& InParams,
            const FFragment_Queue_Current& InCurrent)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
