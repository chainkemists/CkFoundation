#pragma once

#include "CkCue_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKCUE_API FProcessor_Cue_Execute : public TProcessor<FProcessor_Cue_Execute,
            FFragment_Cue_ExecuteRequest, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Cue_ExecuteRequest;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Cue_ExecuteRequest& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKCUE_API FProcessor_Cue_ExecuteLocal : public TProcessor<FProcessor_Cue_ExecuteLocal,
            FFragment_Cue_ExecuteRequestLocal, CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Cue_ExecuteRequestLocal;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Cue_ExecuteRequestLocal& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
