#pragma once

#include "CkGait/Gait/CkGait_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

namespace ck
{
    class CKGAIT_API FProcessor_Gait_HandleRequests : public ck_exp::TProcessor<
        FProcessor_Gait_HandleRequests,
        FCk_Handle_Gait,
        TReadWrite<FFragment_Gait_Tunables>,
        TReadWrite<FFragment_Gait>,
        TReadWrite<FFragment_Gait_Requests>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FFragment_Gait_Requests;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Gait_Tunables& InTunables,
            FFragment_Gait& InGait,
            FFragment_Gait_Requests& InRequests) -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Gait_Tunables& InTunables,
            FFragment_Gait& InGait,
            const FCk_Request_Gait_UpdateSpec& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Gait_Tunables& InTunables,
            FFragment_Gait& InGait,
            const FCk_Request_Gait_EnableDisable& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Gait_Tunables& InTunables,
            FFragment_Gait& InGait,
            const FCk_Request_Gait_Reset& InRequest) -> ECk_Request_OperationResult;
    };

    // Runs EVERY frame (no MarkedDirtyBy): a disabled gait still relaxes its Amount and idles its phase. Motion source:
    // the spec's movement component (Tunables); never a transform, camera, controller or input.
    class CKGAIT_API FProcessor_Gait_Update : public ck_exp::TProcessor<
        FProcessor_Gait_Update,
        FCk_Handle_Gait,
        TReadOnly<FFragment_Gait_Tunables>,
        TReadWrite<FFragment_Gait>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_Gait_HandleRequests>;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Gait_Tunables& InTunables,
            FFragment_Gait& InGait) -> void;
    };

    class CKGAIT_API FProcessor_Gait_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_Gait_CancelPendingRequests,
        FCk_Handle_Gait,
        TReadWrite<FFragment_Gait_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Gait_Requests& InRequests) -> void;
    };
}
