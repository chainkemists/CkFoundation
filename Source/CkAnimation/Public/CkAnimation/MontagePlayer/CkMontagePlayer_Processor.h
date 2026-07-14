#pragma once

#include "CkMontagePlayer_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
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
            const FCk_Request_MontagePlayer_Play& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_Stop& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_Pause& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_Resume& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            UAnimInstance* InAnimInstance,
            FFragment_MontagePlayer_Current& InCurrent,
            const FCk_Request_MontagePlayer_JumpToSection& InRequest) -> void;
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

    // (FProcessor_MontagePlayer_ReplicateOnRestore removed — restore re-seed is now the generic
    //  FProcessor_Persistence_ReDriveOnRestore driven by this feature's Produce/SeedContainer.)

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
