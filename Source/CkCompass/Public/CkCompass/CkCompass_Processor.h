#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCompass/CkCompass_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKCOMPASS_API FProcessor_Compass_Setup : public ck_exp::TProcessor<
        FProcessor_Compass_Setup,
        FCk_Handle_Compass,
        ck::TReadOnly<FFragment_Compass_Params>,
        ck::TReadWrite<FFragment_Compass_Current>,
        FTag_Compass_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using MarkedDirtyBy = FTag_Compass_NeedsSetup;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InCompassEntity,
            const FFragment_Compass_Params& InParams,
            FFragment_Compass_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKCOMPASS_API FProcessor_Compass_HandleRequests
        : public ck_exp::TProcessor<FProcessor_Compass_HandleRequests, FCk_Handle_Compass,
            ck::TReadWrite<FFragment_Compass_Current>, ck::TReadWrite<FFragment_Compass_Params>, ck::TReadWrite<FFragment_Compass_Requests>,
            TExclude<FTag_Compass_NeedsSetup>, TExclude<FTag_DestroyEntity_Initiate>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using RunAfter = TDepList<FProcessor_Compass_Setup>;
        using MarkedDirtyBy = FFragment_Compass_Requests;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InCompassEntity,
            FFragment_Compass_Current& InCurrent,
            FFragment_Compass_Params& InParams,
            FFragment_Compass_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InCompassEntity,
            FFragment_Compass_Current& InCurrent,
            FFragment_Compass_Params& InParams,
            const FCk_Request_Compass_SetCategoryFilter& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InCompassEntity,
            FFragment_Compass_Current& InCurrent,
            FFragment_Compass_Params& InParams,
            const FCk_Request_Compass_SetManualHeading& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InCompassEntity,
            FFragment_Compass_Current& InCurrent,
            FFragment_Compass_Params& InParams,
            const FCk_Request_Compass_SetObserver& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests excludes owners already tagged for destruction, so a destroyed compass' still-queued
    // requests are never drained. This fires each pending request's completion delegate with
    // Failed_Cancelled so a caller awaiting completion terminates instead of hanging.
    class CKCOMPASS_API FProcessor_Compass_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_Compass_CancelPendingRequests,
        FCk_Handle_Compass,
        ck::TReadOnly<FFragment_Compass_Requests>,
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
            HandleType InCompassEntity,
            const FFragment_Compass_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKCOMPASS_API FProcessor_Compass_Update : public ck_exp::TProcessor<
        FProcessor_Compass_Update,
        FCk_Handle_Compass,
        ck::TReadOnly<FFragment_Compass_Params>,
        ck::TReadWrite<FFragment_Compass_Current>,
        ck::TReadWrite<FFragment_Compass_Scratch>,
        TExclude<FTag_Compass_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        // Reads final frame transforms AND the camera's composed POV (FGroup_Gameplay_Camera runs earlier
        // in the frame; FGroup_PostTransform group-RunAfters it — same slot as the WorldSpaceWidget projector)
        using Group = FGroup_PostTransform;
        using RunAfter = TDepList<FProcessor_Compass_Setup, FProcessor_Compass_HandleRequests>;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::CosmeticOnly;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InCompassEntity,
            const FFragment_Compass_Params& InParams,
            FFragment_Compass_Current& InCurrent,
            FFragment_Compass_Scratch& InScratch) const -> void;

    private:
        static auto
        DoResolveHeading(
            HandleType InCompassEntity,
            const FFragment_Compass_Params& InParams,
            const FFragment_Compass_Current& InCurrent,
            const FCk_Handle& InObserver) -> float;

        static auto
        DoProjectPois(
            HandleType InCompassEntity,
            const FFragment_Compass_Params& InParams,
            FFragment_Compass_Current& InCurrent,
            FFragment_Compass_Scratch& InScratch,
            const FVector& InObserverLocation) -> void;

        static auto
        DoDiffAndPublishEntries(
            HandleType InCompassEntity,
            FFragment_Compass_Current& InCurrent,
            FFragment_Compass_Scratch& InScratch) -> void;

        static auto
        DoClearAllEntries(
            HandleType InCompassEntity,
            FFragment_Compass_Current& InCurrent,
            FFragment_Compass_Scratch& InScratch) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKCOMPASS_API FProcessor_Compass_EndPlay : public ck_exp::TProcessor<
        FProcessor_Compass_EndPlay,
        FCk_Handle_Compass,
        ck::TReadOnly<FFragment_Compass_Params>,
        ck::TReadWrite<FFragment_Compass_Current>,
        ck::TReadWrite<FFragment_Compass_Scratch>,
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
            HandleType InCompassEntity,
            const FFragment_Compass_Params& InParams,
            FFragment_Compass_Current& InCurrent,
            FFragment_Compass_Scratch& InScratch)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------