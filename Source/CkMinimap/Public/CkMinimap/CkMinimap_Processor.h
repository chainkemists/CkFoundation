#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkMinimap/CkMinimap_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKMINIMAP_API FProcessor_Minimap_Setup : public ck_exp::TProcessor<
        FProcessor_Minimap_Setup,
        FCk_Handle_Minimap,
        ck::TReadOnly<FFragment_Minimap_Params>,
        ck::TReadWrite<FFragment_Minimap_Current>,
        FTag_Minimap_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using MarkedDirtyBy = FTag_Minimap_NeedsSetup;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMinimapEntity,
            const FFragment_Minimap_Params& InParams,
            FFragment_Minimap_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKMINIMAP_API FProcessor_Minimap_HandleRequests
        : public ck_exp::TProcessor<FProcessor_Minimap_HandleRequests, FCk_Handle_Minimap,
            ck::TReadWrite<FFragment_Minimap_Current>, ck::TReadWrite<FFragment_Minimap_Params>, ck::TReadWrite<FFragment_Minimap_Requests>,
            TExclude<FTag_Minimap_NeedsSetup>, TExclude<FTag_DestroyEntity_Initiate>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using RunAfter = TDepList<FProcessor_Minimap_Setup>;
        using MarkedDirtyBy = FFragment_Minimap_Requests;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMinimapEntity,
            FFragment_Minimap_Current& InCurrent,
            FFragment_Minimap_Params& InParams,
            FFragment_Minimap_Requests& InRequests) const -> void;

    private:
        // Genuine failure path (ViewExtent must be > 0, ensure-gated) — returns whether the request
        // could be honoured, so the drain can report Failed instead of the default Succeeded.
        static auto
        DoHandleRequest(
            HandleType InMinimapEntity,
            FFragment_Minimap_Current& InCurrent,
            FFragment_Minimap_Params& InParams,
            const FCk_Request_Minimap_SetViewExtent& InRequest) -> bool;

        static auto
        DoHandleRequest(
            HandleType InMinimapEntity,
            FFragment_Minimap_Current& InCurrent,
            FFragment_Minimap_Params& InParams,
            const FCk_Request_Minimap_SetCategoryFilter& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InMinimapEntity,
            FFragment_Minimap_Current& InCurrent,
            FFragment_Minimap_Params& InParams,
            const FCk_Request_Minimap_SetObserver& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InMinimapEntity,
            FFragment_Minimap_Current& InCurrent,
            FFragment_Minimap_Params& InParams,
            const FCk_Request_Minimap_SetRotationMode& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InMinimapEntity,
            FFragment_Minimap_Current& InCurrent,
            FFragment_Minimap_Params& InParams,
            const FCk_Request_Minimap_SetFogOfWar& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKMINIMAP_API FProcessor_Minimap_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_Minimap_CancelPendingRequests,
        FCk_Handle_Minimap,
        ck::TReadOnly<FFragment_Minimap_Requests>,
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
            HandleType InMinimapEntity,
            const FFragment_Minimap_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKMINIMAP_API FProcessor_Minimap_Update : public ck_exp::TProcessor<
        FProcessor_Minimap_Update,
        FCk_Handle_Minimap,
        ck::TReadOnly<FFragment_Minimap_Params>,
        ck::TReadWrite<FFragment_Minimap_Current>,
        TExclude<FTag_Minimap_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        // Reads final frame transforms AND the camera's composed POV (FGroup_Gameplay_Camera runs earlier
        // in the frame; FGroup_PostTransform group-RunAfters it) — same slot as the compass projector.
        // FogOfWar processors run in an EARLIER group so the fog cull below never reads a same-frame
        // half-written grid (same-group ordering is registration order — unsafe by house rule).
        using Group = FGroup_PostTransform;
        using RunAfter = TDepList<FProcessor_Minimap_Setup, FProcessor_Minimap_HandleRequests>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMinimapEntity,
            const FFragment_Minimap_Params& InParams,
            FFragment_Minimap_Current& InCurrent) const -> void;

    private:
        static auto
        DoResolveViewYaw(
            const FCk_Handle& InObserver,
            const FFragment_Minimap_Current& InCurrent) -> float;

        static auto
        DoProjectPois(
            HandleType InMinimapEntity,
            const FFragment_Minimap_Params& InParams,
            FFragment_Minimap_Current& InCurrent) -> void;

        static auto
        DoDiffAndPublishEntries(
            HandleType InMinimapEntity,
            FFragment_Minimap_Current& InCurrent) -> void;

        static auto
        DoClearAllEntries(
            HandleType InMinimapEntity,
            FFragment_Minimap_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKMINIMAP_API FProcessor_Minimap_EndPlay : public ck_exp::TProcessor<
        FProcessor_Minimap_EndPlay,
        FCk_Handle_Minimap,
        ck::TReadOnly<FFragment_Minimap_Params>,
        ck::TReadWrite<FFragment_Minimap_Current>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InMinimapEntity,
            const FFragment_Minimap_Params& InParams,
            FFragment_Minimap_Current& InCurrent)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
