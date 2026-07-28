#pragma once

#include "CkMontagePlayer_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

class UAnimInstance;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKANIMATION_API FProcessor_MontagePlayer_HandleRequests : public ck_exp::TProcessor<
            FProcessor_MontagePlayer_HandleRequests,
            FCk_Handle_MontagePlayer,
            TReadOnly<FFragment_MontagePlayer_Params>,
            TReadWrite<FFragment_MontagePlayer_Current>,
            TReadWrite<FFragment_MontagePlayer_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FFragment_MontagePlayer_Requests;

        // Catch-up window for late-join playrate adjustment.
        static const FCk_Time _SyncTargetTime;
        static constexpr float _MaxPlayRateAdjustment = 3.0f;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_MontagePlayer_Params& InParams,
            FFragment_MontagePlayer_Current& InCurrent,
            FFragment_MontagePlayer_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_Play& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_Stop& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_Pause& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_Resume& InRequest) -> ECk_Request_OperationResult;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_JumpToSection& InRequest) -> ECk_Request_OperationResult;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKANIMATION_API FProcessor_MontagePlayer_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_MontagePlayer_CancelPendingRequests,
        FCk_Handle_MontagePlayer,
        ck::TReadOnly<FFragment_MontagePlayer_Requests>,
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
            const FFragment_MontagePlayer_Requests& InRequestsComp)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKANIMATION_API FProcessor_MontagePlayer_MonitorAnimInstance : public ck_exp::TProcessor<
            FProcessor_MontagePlayer_MonitorAnimInstance,
            FCk_Handle_MontagePlayer,
            TReadOnly<FFragment_MontagePlayer_Params>,
            TReadWrite<FFragment_MontagePlayer_Current>,
            FTag_MontagePlayer_HasActiveMontage,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_MontagePlayer_HandleRequests>;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_MontagePlayer_Params& InParams,
            FFragment_MontagePlayer_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // (The per-feature restore-replicate processor was removed — restore re-application is now the
    //  v3 hydration Apply path, which re-applies this feature's saved Produce payload.)

    // --------------------------------------------------------------------------------------------------------------------

    class CKANIMATION_API FProcessor_MontagePlayer_Replicate : public ck_exp::TProcessor<
            FProcessor_MontagePlayer_Replicate,
            FCk_Handle_MontagePlayer,
            TReadOnly<FFragment_MontagePlayer_Current>,
            TReadOnly<FFragment_ContainerRef_MontagePlayer>,
            FTag_MontagePlayer_MayRequireReplication,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Replication;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        using MarkedDirtyBy = FTag_MontagePlayer_MayRequireReplication;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_MontagePlayer_Current& InCurrent,
            const FFragment_ContainerRef_MontagePlayer& InRepRef) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
