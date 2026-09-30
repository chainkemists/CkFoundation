#pragma once

#include "CkChain/CkChain_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

namespace ck
{
    class CKCHAIN_API FProcessor_Chain_Setup : public ck_exp::TProcessor<
        FProcessor_Chain_Setup,
        FCk_Handle_Chain,
        TReadOnly<FFragment_Chain_Params>,
        TReadWrite<FFragment_Chain>,
        FTag_Chain_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Derived;
        using MarkedDirtyBy = FTag_Chain_NeedsSetup;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Chain_Params& InParams,
            FFragment_Chain& InCurrent) -> void;
    };

    class CKCHAIN_API FProcessor_Chain_HandleRequests : public ck_exp::TProcessor<
        FProcessor_Chain_HandleRequests,
        FCk_Handle_Chain,
        TReadOnly<FFragment_Chain_Params>,
        TReadWrite<FFragment_Chain>,
        TReadWrite<FFragment_Chain_Requests>,
        TExclude<FTag_Chain_NeedsSetup>,
        TExclude<FTag_Chain_SplitPending>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Derived;
        using MarkedDirtyBy = FFragment_Chain_Requests;
        using RunAfter = TDepList<FProcessor_Chain_Setup>;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Chain_Params& InParams,
            FFragment_Chain& InCurrent,
            FFragment_Chain_Requests& InRequests) -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Chain_Params& InParams,
            FFragment_Chain& InCurrent,
            const FCk_Request_Chain_AttachLink& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Chain_Params& InParams,
            FFragment_Chain& InCurrent,
            const FCk_Request_Chain_DetachLink& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Chain_Params& InParams,
            FFragment_Chain& InCurrent,
            const FCk_Request_Chain_SetLinkDistance& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Chain_Params& InParams,
            FFragment_Chain& InCurrent,
            const FCk_Request_Chain_Split& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Chain_Params& InParams,
            FFragment_Chain& InCurrent,
            const FCk_Request_Chain_ReseedHistory& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Chain_Params& InParams,
            FFragment_Chain& InCurrent,
            const FCk_Request_Chain_EnableDisable& InRequest) -> ECk_Request_OperationResult;

    };

    class CKCHAIN_API FProcessor_Chain_Update : public ck_exp::TProcessor<
        FProcessor_Chain_Update,
        FCk_Handle_Chain,
        TReadOnly<FFragment_Chain_Params>,
        TReadWrite<FFragment_Chain>,
        TExclude<FTag_Chain_NeedsSetup>,
        TExclude<FTag_Chain_SplitPending>,
        TExclude<FTag_Chain_Disabled>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_Derived;
        using RunAfter = TDepList<FProcessor_Chain_HandleRequests>;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Chain_Params& InParams,
            FFragment_Chain& InCurrent) -> void;

    private:
        static auto
        DoPublishPose(
            FCk_Handle_ChainLink InLink,
            const FFragment_ChainLink_Params& InParams,
            const FTransform& InTargetPose) -> void;
    };

    class CKCHAIN_API FProcessor_Chain_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_Chain_CancelPendingRequests,
        FCk_Handle_Chain,
        TReadWrite<FFragment_Chain_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Chain_Requests& InRequests) -> void;
    };

    class CKCHAIN_API FProcessor_Chain_EndPlay : public ck_exp::TProcessor<
        FProcessor_Chain_EndPlay,
        FCk_Handle_Chain,
        TReadWrite<FFragment_Chain>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        using RunAfter = TDepList<FProcessor_Chain_CancelPendingRequests>;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Chain& InCurrent) -> void;
    };

    class CKCHAIN_API FProcessor_ChainLink_EndPlay : public ck_exp::TProcessor<
        FProcessor_ChainLink_EndPlay,
        FCk_Handle_ChainLink,
        TReadOnly<FFragment_ChainLink>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

        using TProcessor::TProcessor;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ChainLink& InCurrent) -> void;
    };

}

