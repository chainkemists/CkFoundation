#pragma once

#include "CkRotateTowards/CkRotateTowards_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

namespace ck
{
    class CKROTATETOWARDS_API FProcessor_RotateTowards_HandleRequests : public ck_exp::TProcessor<
        FProcessor_RotateTowards_HandleRequests,
        FCk_Handle_RotateTowards,
        TReadWrite<FFragment_RotateTowards_Tunables>,
        TReadWrite<FFragment_RotateTowards>,
        TReadWrite<FFragment_RotateTowards_Requests>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Derived;
        using MarkedDirtyBy = FFragment_RotateTowards_Requests;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_RotateTowards_Tunables& InTunables,
            FFragment_RotateTowards& InRotateTowards,
            FFragment_RotateTowards_Requests& InRequests) -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_RotateTowards_Tunables& InTunables,
            FFragment_RotateTowards& InRotateTowards,
            const FCk_Request_RotateTowards_SetTarget& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_RotateTowards_Tunables& InTunables,
            FFragment_RotateTowards& InRotateTowards,
            const FCk_Request_RotateTowards_ClearTarget& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_RotateTowards_Tunables& InTunables,
            FFragment_RotateTowards& InRotateTowards,
            const FCk_Request_RotateTowards_UpdateTunables& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_RotateTowards_Tunables& InTunables,
            FFragment_RotateTowards& InRotateTowards,
            const FCk_Request_RotateTowards_SetRangeClamp& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_RotateTowards_Tunables& InTunables,
            FFragment_RotateTowards& InRotateTowards,
            const FCk_Request_RotateTowards_ClearRangeClamp& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_RotateTowards_Tunables& InTunables,
            FFragment_RotateTowards& InRotateTowards,
            const FCk_Request_RotateTowards_EnableDisable& InRequest) -> ECk_Request_OperationResult;

    public:
        // Shared with Update (target lost): clears the target, removes both tags and the Result fragment, then broadcasts OnTargetCleared.
        static auto
        DoClearTarget(
            HandleType InHandle,
            FFragment_RotateTowards& InRotateTowards,
            ECk_RotateTowards_ClearReason InReason) -> void;
    };

    // Runs every frame while the entity has a target (no MarkedDirtyBy); a disabled entity holds its rotation.
    class CKROTATETOWARDS_API FProcessor_RotateTowards_Update : public ck_exp::TProcessor<
        FProcessor_RotateTowards_Update,
        FCk_Handle_RotateTowards,
        TReadOnly<FFragment_RotateTowards_Tunables>,
        TReadWrite<FFragment_RotateTowards>,
        TReadWrite<FFragment_RotateTowards_SolveResult>,
        FTag_RotateTowards_HasTarget,
        TExclude<FTag_RotateTowards_Disabled>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Derived;
        using RunAfter = TDepList<FProcessor_RotateTowards_HandleRequests>;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_RotateTowards_Tunables& InTunables,
            FFragment_RotateTowards& InRotateTowards,
            FFragment_RotateTowards_SolveResult& InResult) -> void;

    private:
        static auto
        DoApplyRotation(
            HandleType InHandle,
            FCk_Handle_Transform& InTransform,
            const FRotator& InCurrentWorld,
            const FRotator& InNewWorld) -> void;
    };

    class CKROTATETOWARDS_API FProcessor_RotateTowards_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_RotateTowards_CancelPendingRequests,
        FCk_Handle_RotateTowards,
        TReadWrite<FFragment_RotateTowards_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_RotateTowards_Requests& InRequests) -> void;
    };
}
