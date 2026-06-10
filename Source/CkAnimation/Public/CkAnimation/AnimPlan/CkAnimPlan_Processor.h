#pragma once

#include "CkAnimPlan_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKANIMATION_API FProcessor_AnimPlan_HandleRequests : public ck_exp::TProcessor<
            FProcessor_AnimPlan_HandleRequests,
            FCk_Handle_AnimPlan,
            TReadOnly<FFragment_AnimPlan_Params>,
            TReadWrite<FFragment_AnimPlan_Current>,
            TReadWrite<FFragment_AnimPlan_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using MarkedDirtyBy = FFragment_AnimPlan_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AnimPlan_Params& InParams,
            FFragment_AnimPlan_Current& InCurrent,
            FFragment_AnimPlan_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AnimPlan_Current& InCurrent,
            const FCk_Request_AnimPlan_UpdateAnimCluster& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_AnimPlan_Current& InCurrent,
            const FCk_Request_AnimPlan_UpdateAnimState& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKANIMATION_API FProcessor_AnimPlan_Replicate : public ck_exp::TProcessor<
            FProcessor_AnimPlan_Replicate,
            FCk_Handle_AnimPlan,
            TReadOnly<FFragment_AnimPlan_Params>,
            TReadWrite<FFragment_AnimPlan_Current>,
            FTag_AnimPlan_MayRequireReplication,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Replication;
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        using MarkedDirtyBy = FTag_AnimPlan_MayRequireReplication;

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
            const FFragment_AnimPlan_Params& InParams,
            FFragment_AnimPlan_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Re-drives replication of a RESTORED AnimPlan to clients after a snapshot load: once the OWNER's
    // replication driver is re-established (snapshot respawn pass), re-create the owner-resident
    // container entry (Construct is abstained during reconstitution) and re-arm
    // FTag_AnimPlan_MayRequireReplication so FProcessor_AnimPlan_Replicate re-pushes the restored
    // cluster/state. Pairs the shared ck::FTag_Snapshot_JustRestored with the per-feature
    // FTag_AnimPlan_RestoreReplicated done tag; POINT-QUERIES the in_place marker (tombstone trap).
    class CKANIMATION_API FProcessor_AnimPlan_ReplicateOnRestore : public ck_exp::TProcessor<
            FProcessor_AnimPlan_ReplicateOnRestore,
            FCk_Handle_AnimPlan,
            TReadOnly<FFragment_AnimPlan_Params>,
            TReadOnly<FFragment_AnimPlan_Current>,
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
            const FFragment_AnimPlan_Params& InParams,
            const FFragment_AnimPlan_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
