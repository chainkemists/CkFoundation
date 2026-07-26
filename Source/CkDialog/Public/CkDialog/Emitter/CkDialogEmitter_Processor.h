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

    // Advances every active cooldown and retires the ones that finish, broadcasting OnDialogCooldownEnded for each.
    //
    // This exists as its own processor because expiry has to be noticed even when nobody is querying. Pruning used to
    // ride along inside EvaluateQueries, whose view is gated on pending queries — so on a silent emitter a lapsed
    // cooldown simply sat in the map. That was invisible while the only consumers compared against a deadline; the
    // moment an "ended" signal exists, something has to actually observe the transition.
    //
    // Declared before EvaluateQueries because that processor names this one in its RunAfter list.
    //
    // The HasCooldowns marker keeps the view to emitters that are genuinely cooling, and is dropped with the last
    // entry so a quiet emitter costs nothing.
    class CKDIALOG_API FProcessor_DialogEmitter_TickCooldowns : public ck_exp::TProcessor<
        FProcessor_DialogEmitter_TickCooldowns,
        FCk_Handle_DialogEmitter,
        FTag_DialogEmitter_HasCooldowns,
        ck::TReadWrite<FFragment_DialogEmitter_Cooldowns>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_DialogEmitter_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InEmitter,
            FFragment_DialogEmitter_Cooldowns& InCooldowns) const -> void;
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
        // TickCooldowns must land first. Cooldown state is now advanced by that processor rather than compared
        // against an absolute deadline, so classification reads whatever it last wrote — evaluate before it and a
        // line that expired this frame is still treated as cooling.
        using RunAfter = TDepList<FProcessor_DialogEmitter_HandleRequests, FProcessor_DialogEmitter_TickCooldowns>;
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

        static auto
        DoRecordDebug(
            HandleType InEmitter,
            const FCk_DialogEmitter_QueryResult& InResult,
            FCk_Time InNow) -> void;
    };

}

// --------------------------------------------------------------------------------------------------------------------
