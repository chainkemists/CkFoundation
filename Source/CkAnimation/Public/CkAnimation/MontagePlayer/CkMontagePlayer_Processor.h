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

    // ---- ReplicateOnRestore (Server-side, post-snapshot-load) ----

    // Re-drives replication of a RESTORED MontagePlayer to clients after a snapshot load: once this
    // entity's replication driver is re-established (snapshot respawn pass), re-create the
    // self-resident container entry SEEDED with the restored State (Construct is abstained during
    // reconstitution) and re-arm FTag_MontagePlayer_MayRequireReplication. Clients receive the State
    // and replay it through the normal dispatch (DoDispatchReplicatedState), so client-side playback
    // resumes; SERVER-side playback stays inert (the Params mesh component does not survive a save —
    // see FFragment_MontagePlayer_Params::SerializeSnapshot). Pairs ck::FTag_Snapshot_JustRestored
    // (shared per-entity, never removed here) with the per-feature done tag. The view iterates the
    // clean FFragment_MontagePlayer_Current and POINT-QUERIES the in_place marker (listing it in the
    // view would surface tombstones).
    class CKANIMATION_API FProcessor_MontagePlayer_ReplicateOnRestore : public ck_exp::TProcessor<
            FProcessor_MontagePlayer_ReplicateOnRestore,
            FCk_Handle_MontagePlayer,
            TReadOnly<FFragment_MontagePlayer_Current>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_MontagePlayer_Current& InCurrent) const -> void;
    };

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
