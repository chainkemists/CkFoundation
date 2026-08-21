#pragma once

#include "CkQueue/Queue/CkQueue_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKQUEUE_API FProcessor_Queue_Formation : public ck_exp::TProcessor<
        FProcessor_Queue_Formation,
        FCk_Handle_Queue,
        ck::TReadOnly<FFragment_Queue_Params>,
        ck::TReadWrite<FFragment_Queue_Current>,
        FTag_Queue_NeedsFormation,
        TExclude<FTag_Queue_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_Queue_Reconcile>;
        using MarkedDirtyBy = FTag_Queue_NeedsFormation;
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue_Current& InCurrent)
            -> void;

    private:
        static auto
        RecordRetryableFailure(
            HandleType InQueue,
            const FFragment_Queue_Params& InParams,
            FFragment_Queue_Current& InCurrent,
            ECk_Queue_EventReason InReason,
            double InWorldTimeSeconds,
            int32 InNavigationRevision)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
