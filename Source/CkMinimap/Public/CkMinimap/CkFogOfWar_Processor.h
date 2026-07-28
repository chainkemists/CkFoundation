#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkMinimap/CkFogOfWar_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // The FGroup_Gameplay placement and the explicit NetModeRequirement::All below are both load-bearing —
    // rationale (and the one-frame-stale revealer-transform consequence) in CkMinimap/CLAUDE.md.

    class CKMINIMAP_API FProcessor_FogOfWar_Setup : public ck_exp::TProcessor<
        FProcessor_FogOfWar_Setup,
        FCk_Handle_FogOfWar,
        ck::TReadOnly<FFragment_FogOfWar_Params>,
        ck::TReadWrite<FFragment_FogOfWar_Current>,
        FTag_FogOfWar_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FTag_FogOfWar_NeedsSetup;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::All;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InFogEntity,
            const FFragment_FogOfWar_Params& InParams,
            FFragment_FogOfWar_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKMINIMAP_API FProcessor_FogOfWar_HandleRequests
        : public ck_exp::TProcessor<FProcessor_FogOfWar_HandleRequests, FCk_Handle_FogOfWar,
            ck::TReadWrite<FFragment_FogOfWar_Current>, ck::TReadOnly<FFragment_FogOfWar_Params>, ck::TReadWrite<FFragment_FogOfWar_Requests>,
            TExclude<FTag_FogOfWar_NeedsSetup>, TExclude<FTag_DestroyEntity_Initiate>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_FogOfWar_Setup>;
        using MarkedDirtyBy = FFragment_FogOfWar_Requests;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::All;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InFogEntity,
            FFragment_FogOfWar_Current& InCurrent,
            const FFragment_FogOfWar_Params& InParams,
            FFragment_FogOfWar_Requests& InRequests) const -> void;

    private:
        // Genuine failure path (malformed input, ensure-gated) — returns whether the request could
        // be honoured, so the drain can report Failed instead of the default Succeeded.
        static auto
        DoHandleRequest(
            HandleType InFogEntity,
            FFragment_FogOfWar_Current& InCurrent,
            const FFragment_FogOfWar_Params& InParams,
            const FCk_Request_FogOfWar_AddRevealer& InRequest) -> bool;

        static auto
        DoHandleRequest(
            HandleType InFogEntity,
            FFragment_FogOfWar_Current& InCurrent,
            const FFragment_FogOfWar_Params& InParams,
            const FCk_Request_FogOfWar_RemoveRevealer& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InFogEntity,
            FFragment_FogOfWar_Current& InCurrent,
            const FFragment_FogOfWar_Params& InParams,
            const FCk_Request_FogOfWar_RevealLocation& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InFogEntity,
            FFragment_FogOfWar_Current& InCurrent,
            const FFragment_FogOfWar_Params& InParams,
            const FCk_Request_FogOfWar_RevealAll& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InFogEntity,
            FFragment_FogOfWar_Current& InCurrent,
            const FFragment_FogOfWar_Params& InParams,
            const FCk_Request_FogOfWar_Reset& InRequest) -> void;

        // Genuine failure path (cell-count mismatch, ensure-gated) — see AddRevealer above.
        static auto
        DoHandleRequest(
            HandleType InFogEntity,
            FFragment_FogOfWar_Current& InCurrent,
            const FFragment_FogOfWar_Params& InParams,
            const FCk_Request_FogOfWar_SetExplored& InRequest) -> bool;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKMINIMAP_API FProcessor_FogOfWar_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_FogOfWar_CancelPendingRequests,
        FCk_Handle_FogOfWar,
        ck::TReadOnly<FFragment_FogOfWar_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InFogEntity,
            const FFragment_FogOfWar_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKMINIMAP_API FProcessor_FogOfWar_Update : public ck_exp::TProcessor<
        FProcessor_FogOfWar_Update,
        FCk_Handle_FogOfWar,
        ck::TReadOnly<FFragment_FogOfWar_Params>,
        ck::TReadWrite<FFragment_FogOfWar_Current>,
        TExclude<FTag_FogOfWar_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_FogOfWar_Setup, FProcessor_FogOfWar_HandleRequests>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::All;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InFogEntity,
            const FFragment_FogOfWar_Params& InParams,
            FFragment_FogOfWar_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
