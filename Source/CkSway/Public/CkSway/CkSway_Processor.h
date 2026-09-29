#pragma once

#include "CkSway/CkSway_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

namespace ck
{
    class CKSWAY_API FProcessor_Sway_Setup : public ck_exp::TProcessor<
        FProcessor_Sway_Setup,
        FCk_Handle_Sway,
        TReadOnly<FFragment_Sway_Tunables>,
        TReadWrite<FFragment_Sway>,
        FTag_Sway_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Derived;
        using MarkedDirtyBy = FTag_Sway_NeedsSetup;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sway_Tunables& InTunables,
            FFragment_Sway& InSway) -> void;
    };

    class CKSWAY_API FProcessor_Sway_HandleRequests : public ck_exp::TProcessor<
        FProcessor_Sway_HandleRequests,
        FCk_Handle_Sway,
        TReadWrite<FFragment_Sway_Tunables>,
        TReadWrite<FFragment_Sway>,
        TReadWrite<FFragment_Sway_Requests>,
        TExclude<FTag_Sway_NeedsSetup>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Derived;
        using MarkedDirtyBy = FFragment_Sway_Requests;
        using RunAfter = TDepList<FProcessor_Sway_Setup>;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Sway_Tunables& InTunables,
            FFragment_Sway& InSway,
            FFragment_Sway_Requests& InRequests) -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Sway_Tunables& InTunables,
            FFragment_Sway& InSway,
            const FCk_Request_Sway_UpdateSpec& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Sway_Tunables& InTunables,
            FFragment_Sway& InSway,
            const FCk_Request_Sway_EnableDisable& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Sway_Tunables& InTunables,
            FFragment_Sway& InSway,
            const FCk_Request_Sway_Reset& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Sway_Tunables& InTunables,
            FFragment_Sway& InSway,
            const FCk_Request_Sway_SetRestOffset& InRequest) -> ECk_Request_OperationResult;
    };

    // Runs EVERY frame (no MarkedDirtyBy): a disabled sway still relaxes to rest. Stimulus source: the node's driver
    // frame (UCk_Utils_SceneNode_UE::Get_DriverWorldTransform).
    class CKSWAY_API FProcessor_Sway_Update : public ck_exp::TProcessor<
        FProcessor_Sway_Update,
        FCk_Handle_Sway,
        TReadOnly<FFragment_Sway_Tunables>,
        TReadWrite<FFragment_Sway>,
        TExclude<FTag_Sway_NeedsSetup>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Derived;
        using RunAfter = TDepList<FProcessor_Sway_HandleRequests>;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sway_Tunables& InTunables,
            FFragment_Sway& InSway) -> void;

    private:
        static auto
        DoPublishOffset(
            HandleType InHandle,
            FFragment_Sway& InSway,
            const FTransform& InOffset) -> void;
    };

    class CKSWAY_API FProcessor_Sway_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_Sway_CancelPendingRequests,
        FCk_Handle_Sway,
        TReadWrite<FFragment_Sway_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Sway_Requests& InRequests) -> void;
    };
}
