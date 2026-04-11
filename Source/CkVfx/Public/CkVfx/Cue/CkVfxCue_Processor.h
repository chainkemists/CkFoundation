#pragma once

#include "CkVfxCue_Fragment.h"
#include "CkVfxCue_EntityScript.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKVFX_API FProcessor_VfxCue_Setup : public ck_exp::TProcessor<
            FProcessor_VfxCue_Setup,
            FCk_Handle_VfxCue,
            ck::TReadOnly<FFragment_VfxCue_Params>,
            ck::TReadWrite<FFragment_VfxCue_Current>,
            ck::TReadOnly<FFragment_EntityScript_Current>,
            FTag_VfxCue_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using MarkedDirtyBy = FTag_VfxCue_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VfxCue_Params& InParams,
            FFragment_VfxCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKVFX_API FProcessor_VfxCue_HandleRequests : public ck_exp::TProcessor<
            FProcessor_VfxCue_HandleRequests,
            FCk_Handle_VfxCue,
            ck::TReadWrite<FFragment_VfxCue_Current>,
            ck::TReadOnly<FFragment_EntityScript_Current>,
            ck::TReadOnly<FFragment_VfxCue_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_VfxCue_Setup>;
        using MarkedDirtyBy = FFragment_VfxCue_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VfxCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript,
            const FFragment_VfxCue_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VfxCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript,
            const FCk_Request_VfxCue_Play& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VfxCue_Current& InCurrent,
            const FFragment_EntityScript_Current& InEntityScript,
            const FCk_Request_VfxCue_Stop& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKVFX_API FProcessor_VfxCue_EffectLifetimeMonitor : public ck_exp::TProcessor<
            FProcessor_VfxCue_EffectLifetimeMonitor,
            FCk_Handle_VfxCue,
            ck::TReadWrite<FFragment_VfxCue_Current>,
            FTag_VfxCue_IsPlaying,
            TExclude<FTag_VfxCue_NeedsSetup>,
            TExclude<FFragment_VfxCue_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Audio;
        using RunAfter = TDepList<FProcessor_VfxCue_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VfxCue_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKVFX_API FProcessor_VfxCue_EndPlay : public ck_exp::TProcessor<
            FProcessor_VfxCue_EndPlay,
            FCk_Handle_VfxCue,
            ck::TReadWrite<FFragment_VfxCue_Current>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_PreDestruction;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VfxCue_Current& InCurrent) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
