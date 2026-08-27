#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkVisualLod/CkVisualLod_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKVISUALLOD_API FProcessor_VisualLod_Setup
        : public ck_exp::TProcessor<FProcessor_VisualLod_Setup, FCk_Handle_VisualLod,
            ck::TReadOnly<FFragment_VisualLod_Params>, ck::TReadWrite<FFragment_VisualLod_Current>,
            FTag_VisualLod_NeedsSetup, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FTag_VisualLod_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VisualLod_Params& InParams,
            FFragment_VisualLod_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKVISUALLOD_API FProcessor_VisualLod_HandleRequests
        : public ck_exp::TProcessor<FProcessor_VisualLod_HandleRequests, FCk_Handle_VisualLod,
            ck::TReadWrite<FFragment_VisualLod_Current>, ck::TReadWrite<FFragment_VisualLod_Requests>,
            TExclude<FTag_VisualLod_NeedsSetup>, TExclude<FTag_DestroyEntity_Initiate>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_VisualLod_Setup>;
        using MarkedDirtyBy = FFragment_VisualLod_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VisualLod_Current& InCurrent,
            FFragment_VisualLod_Requests& InRequests) const
            -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod_Current& InCurrent,
            const FCk_Request_VisualLod_SetArbiter& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod_Current& InCurrent,
            const FCk_Request_VisualLod_SetVisibility& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod_Current& InCurrent,
            const FCk_Request_VisualLod_SetFarAnim& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod_Current& InCurrent,
            const FCk_Request_VisualLod_SetRenderer& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod_Current& InCurrent,
            const FCk_Request_VisualLod_Suspend& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisualLod_Current& InCurrent,
            const FCk_Request_VisualLod_Resume& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Deterministic slot release + budget refund on entity destruction. The arbiter's amortized
    // sweep remains the reconciliation net for anything this misses (e.g. the arbiter dying first)
    class CKVISUALLOD_API FProcessor_VisualLod_EndPlay
        : public ck_exp::TProcessor<FProcessor_VisualLod_EndPlay, FCk_Handle_VisualLod,
            ck::TReadWrite<FFragment_VisualLod_Current>, CK_IF_END_PLAY>
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
            FFragment_VisualLod_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKVISUALLOD_API FProcessor_VisualLod_CancelPendingRequests
        : public ck_exp::TProcessor<FProcessor_VisualLod_CancelPendingRequests, FCk_Handle_VisualLod,
            ck::TReadOnly<FFragment_VisualLod_Requests>, CK_IF_END_PLAY>
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
            const FFragment_VisualLod_Requests& InRequests)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
