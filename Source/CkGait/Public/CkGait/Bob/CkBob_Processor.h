#pragma once

#include "CkGait/Bob/CkBob_Fragment.h"
#include "CkGait/Gait/CkGait_Processor.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

namespace ck
{
    class CKGAIT_API FProcessor_Bob_HandleRequests : public ck_exp::TProcessor<
        FProcessor_Bob_HandleRequests,
        FCk_Handle_Bob,
        TReadWrite<FFragment_Bob_Tunables>,
        TReadWrite<FFragment_Bob>,
        TReadWrite<FFragment_Bob_Requests>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FFragment_Bob_Requests;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Bob_Tunables& InTunables,
            FFragment_Bob& InBob,
            FFragment_Bob_Requests& InRequests) -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Bob_Tunables& InTunables,
            FFragment_Bob& InBob,
            const FCk_Request_Bob_UpdateSpec& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Bob_Tunables& InTunables,
            FFragment_Bob& InBob,
            const FCk_Request_Bob_EnableDisable& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Bob_Tunables& InTunables,
            FFragment_Bob& InBob,
            const FCk_Request_Bob_Reset& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Bob_Tunables& InTunables,
            FFragment_Bob& InBob,
            const FCk_Request_Bob_SetRestOffset& InRequest) -> ECk_Request_OperationResult;
    };

    // Runs EVERY frame (no MarkedDirtyBy): a disabled bob, or one whose gait is gone, still relaxes to rest. Reads only
    // its gait and its own node, never a transform; runs in FGroup_Gameplay so its offset request drains in this frame's
    // FGroup_Transform.
    class CKGAIT_API FProcessor_Bob_Update : public ck_exp::TProcessor<
        FProcessor_Bob_Update,
        FCk_Handle_Bob,
        TReadOnly<FFragment_Bob_Tunables>,
        TReadWrite<FFragment_Bob>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_Gait_Update, FProcessor_Bob_HandleRequests>;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Bob_Tunables& InTunables,
            FFragment_Bob& InBob) -> void;

    private:
        static auto
        DoPublishOffset(
            HandleType InHandle,
            FFragment_Bob& InBob,
            const FTransform& InOffset) -> void;
    };

    class CKGAIT_API FProcessor_Bob_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_Bob_CancelPendingRequests,
        FCk_Handle_Bob,
        TReadWrite<FFragment_Bob_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Bob_Requests& InRequests) -> void;
    };
}
