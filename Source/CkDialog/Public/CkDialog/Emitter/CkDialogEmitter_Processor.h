#pragma once

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkDialog/Emitter/CkDialogEmitter_Fragment.h"
#include "CkDialog/Line/CkDialogLine_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UWorld;
class UCk_DialogRegistry_Subsystem_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Drains the emitter's request queue: enqueues queries (adding the PendingQueries marker) and mutates the
    // always-present cooldown fragment.
    class CKDIALOG_API FProcessor_DialogEmitter_HandleRequests : public ck_exp::TProcessor<
        FProcessor_DialogEmitter_HandleRequests,
        FCk_Handle_DialogEmitter,
        ck::TReadOnly<FFragment_DialogEmitter_Params>,
        ck::TReadWrite<FFragment_DialogEmitter_Requests>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FFragment_DialogEmitter_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InEmitter,
            const FFragment_DialogEmitter_Params& InParams,
            FFragment_DialogEmitter_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InEmitter,
            FCk_Time InNow,
            const FCk_Request_DialogEmitter_Query& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InEmitter,
            FCk_Time InNow,
            const FCk_Request_DialogEmitter_StartCooldown& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InEmitter,
            FCk_Time InNow,
            const FCk_Request_DialogEmitter_ClearCooldown& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InEmitter,
            FCk_Time InNow,
            const FCk_Request_DialogEmitter_ClearAllCooldowns& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Evaluates every pending query against the world's Dialog registry and broadcasts OnDialogQueryCompleted.
    class CKDIALOG_API FProcessor_DialogEmitter_EvaluateQueries : public ck_exp::TProcessor<
        FProcessor_DialogEmitter_EvaluateQueries,
        FCk_Handle_DialogEmitter,
        ck::TReadOnly<FFragment_DialogEmitter_Params>,
        ck::TReadWrite<FFragment_DialogEmitter_PendingQueries>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_DialogEmitter_HandleRequests>;
        using MarkedDirtyBy = FFragment_DialogEmitter_PendingQueries;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InEmitter,
            const FFragment_DialogEmitter_Params& InParams,
            FFragment_DialogEmitter_PendingQueries& InPending) const -> void;

    private:
        static auto
        DoEvaluateQuery(
            HandleType InEmitter,
            const FFragment_DialogEmitter_Params& InParams,
            const FFragment_DialogEmitter_Cooldowns& InCooldowns,
            const UCk_DialogRegistry_Subsystem_UE& InRegistry,
            const FCk_Request_DialogEmitter_Query& InQuery,
            FCk_Time InNow) -> FCk_DialogEmitter_QueryResult;

        static auto
        DoClassifyLine(
            HandleType InEmitter,
            const FFragment_DialogEmitter_Cooldowns& InCooldowns,
            const FCk_Handle_DialogLine& InLine) -> ECk_DialogLine_QueryResult;

        // These two touch private fragment members (cooldown map / debug ring) — members so the friend grant on
        // FFragment_DialogEmitter_Cooldowns / _Debug / DebugEntry applies.
        static auto
        DoPruneCooldowns(
            FFragment_DialogEmitter_Cooldowns& InCooldowns,
            FCk_Time InNow) -> void;

        static auto
        DoRecordDebug(
            HandleType InEmitter,
            const FCk_DialogEmitter_QueryResult& InResult,
            FCk_Time InNow) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
