#pragma once

#include "CkPmg_Fragment.h"
#include "CkPmg_ProcessorGroups.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPMG_API FProcessor_Pmg_DebugShape_UpdateTransform : public ck_exp::TProcessor<
            FProcessor_Pmg_DebugShape_UpdateTransform,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadWrite<FFragment_Pmg_DebugShape_Current>,
            ck::TReadOnly<FFragment_Transform>,
            FTag_Transform_Updated,
            TExclude<FTag_Pmg_DebugShape_NeedsSetup>,
            TExclude<FTag_Pmg_DebugShape_Composite>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FGroup_Pmg_DebugShape_Setup>;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FFragment_Transform& InTransform)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_DebugShape_BakeLines : public ck_exp::TProcessor<
            FProcessor_Pmg_DebugShape_BakeLines,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadOnly<FFragment_Pmg_DebugShape_Common>,
            ck::TReadOnly<FFragment_Pmg_DebugShape_Lines>,
            ck::TReadOnly<FFragment_Pmg_DebugShape_Current>,
            FTag_Pmg_DebugShape_LinesNeedBaking,
            TExclude<FTag_Pmg_DebugShape_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using MarkedDirtyBy = FTag_Pmg_DebugShape_LinesNeedBaking;
        using RunAfter = TDepList<FProcessor_Pmg_DebugShape_UpdateTransform>;
        using TProcessor::TProcessor;

    public:
        // Current is TReadOnly deliberately: only the pointed-at mesh component is mutated, never
        // the fragment — so no write conflict against HandleRequests / CheckDuration, which do.
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            const FFragment_Pmg_DebugShape_Lines& InLines,
            const FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Applies the global diagnostic-visibility policy without changing the shape's local render
    // mode. A live streamer-mode CVar therefore hides retained debug shapes and restores their
    // caller-selected visibility when the override is removed.
    class CKPMG_API FProcessor_Pmg_DebugShape_ApplyRuntimeVisibility : public ck_exp::TProcessor<
            FProcessor_Pmg_DebugShape_ApplyRuntimeVisibility,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadOnly<FFragment_Pmg_DebugShape_Common>,
            ck::TReadOnly<FFragment_Pmg_DebugShape_Current>,
            TExclude<FTag_Pmg_DebugShape_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FProcessor_Pmg_DebugShape_UpdateTransform, FProcessor_Pmg_DebugShape_BakeLines>;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            const FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_DebugShape_CheckDuration : public ck_exp::TProcessor<
            FProcessor_Pmg_DebugShape_CheckDuration,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadOnly<FFragment_Pmg_DebugShape_Common>,
            ck::TReadWrite<FFragment_Pmg_DebugShape_Current>,
            TExclude<FTag_Pmg_DebugShape_NeedsSetup>,
            TExclude<FTag_Pmg_DebugShape_PersistentDuration>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        // Chained so the three writers of FFragment_Pmg_DebugShape_Current run in a fixed order.
        using RunAfter = TDepList<FGroup_Pmg_DebugShape_Setup, FProcessor_Pmg_DebugShape_UpdateTransform>;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------


    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_DebugShape_EndPlay : public ck_exp::TProcessor<
            FProcessor_Pmg_DebugShape_EndPlay,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadWrite<FFragment_Pmg_DebugShape_Current>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_DebugShape_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Pmg_DebugShape_HandleRequests,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadWrite<FFragment_Pmg_DebugShape_Common>,
            ck::TReadWrite<FFragment_Pmg_DebugShape_Current>,
            ck::TReadWrite<FFragment_Pmg_DebugShape_Requests>,
            TExclude<FTag_Pmg_DebugShape_NeedsSetup>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FGroup_Pmg_DebugShape_Setup, FProcessor_Pmg_DebugShape_CheckDuration>;
        using MarkedDirtyBy = FFragment_Pmg_DebugShape_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            FFragment_Pmg_DebugShape_Requests& InRequestsComp) const
            -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetColor& InRequest)
            -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetLineThickness& InRequest)
            -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetDrawLines& InRequest)
            -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetDuration& InRequest)
            -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetRenderMode& InRequest)
            -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetEnableCollision& InRequest)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_DebugShape_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_Pmg_DebugShape_CancelPendingRequests,
        FCk_Handle_Pmg_DebugShape,
        ck::TReadOnly<FFragment_Pmg_DebugShape_Requests>,
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
            HandleType InHandle,
            const FFragment_Pmg_DebugShape_Requests& InRequestsComp)
            -> void;
    };
}
