#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkParallelProcessor.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkPhysics/Velocity/CkVelocity_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Diag_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_AccelClamp_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_ApplyOffset_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_AvoidanceSample_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_ConstrainToNavmesh_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_PushApart_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Separation_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Steering_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_VelocityBridge_Processor.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // The tap views are tracked-only and copy outputs after each existing behavior stage. They have
    // no behavioral writes: the recorder fragment is diagnostic-only.
    class CKCROWD_API FProcessor_CrowdAgent_DiagSensorTap : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DiagSensorTap, FCk_Handle_CrowdAgent,
            FTag_CrowdDiag_Tracked,
            ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            ck::TReadWrite<FFragment_CrowdAgent_DiagRecorder>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_NeighborSync>;
        using RunBefore = TDepList<FProcessor_CrowdAgent_Separation>;
        using TProcessor::TProcessor;
        static auto ForEachEntity(TimeType, HandleType, const FFragment_CrowdAgent_NeighborCache&, FFragment_CrowdAgent_DiagRecorder&) -> void;
    };

    class CKCROWD_API FProcessor_CrowdAgent_DiagSeparationTap : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DiagSeparationTap, FCk_Handle_CrowdAgent,
            FTag_CrowdDiag_Tracked,
            ck::TReadOnly<FFragment_CrowdAgent_SeparationForce>, ck::TReadWrite<FFragment_CrowdAgent_DiagRecorder>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_Separation>;
        using RunBefore = TDepList<FProcessor_CrowdAgent_Steering>;
        using TProcessor::TProcessor;
        static auto ForEachEntity(TimeType, HandleType, const FFragment_CrowdAgent_SeparationForce&, FFragment_CrowdAgent_DiagRecorder&) -> void;
    };

    class CKCROWD_API FProcessor_CrowdAgent_DiagSteeringTap : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DiagSteeringTap, FCk_Handle_CrowdAgent,
            FTag_CrowdDiag_Tracked, FTag_CrowdAgent_Walking,
            ck::TReadOnly<FFragment_CrowdAgent_PathFollow>, ck::TReadOnly<FFragment_Nav_PathResult>,
            ck::TReadOnly<FFragment_CrowdAgent_DesiredVelocity>, ck::TReadWrite<FFragment_CrowdAgent_DiagRecorder>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_Steering>;
        using RunBefore = TDepList<FProcessor_CrowdAgent_AvoidanceSample>;
        using TProcessor::TProcessor;
        static auto ForEachEntity(TimeType, HandleType, const FFragment_CrowdAgent_PathFollow&, const FFragment_Nav_PathResult&, const FFragment_CrowdAgent_DesiredVelocity&, FFragment_CrowdAgent_DiagRecorder&) -> void;
    };

    class CKCROWD_API FProcessor_CrowdAgent_DiagAvoidanceScoreTap : public TParallelProcessor<
            FProcessor_CrowdAgent_DiagAvoidanceScoreTap, FCk_Handle_CrowdAgent,
            FTag_CrowdDiag_Tracked, FTag_CrowdAgent_Walking, FTag_CrowdAgent_HasProbe,
            ck::TReadOnly<FFragment_CrowdAgent_Params>, ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            ck::TReadOnly<FFragment_CrowdAgent_DesiredVelocity>, ck::TReadWrite<FFragment_CrowdAgent_DiagRecorder>,
            TExclude<FTag_CrowdAgent_Asleep>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_Steering, FProcessor_CrowdAgent_NeighborSync>;
        using RunBefore = TDepList<FProcessor_CrowdAgent_AvoidanceSample>;
        using TParallelProcessor::TParallelProcessor;
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType,
            const FFragment_CrowdAgent_Params&,
            const FFragment_CrowdAgent_NeighborCache&,
            const FFragment_CrowdAgent_DesiredVelocity&,
            FFragment_CrowdAgent_DiagRecorder&) const -> void;
    };

    class CKCROWD_API FProcessor_CrowdAgent_DiagAvoidanceTap : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DiagAvoidanceTap, FCk_Handle_CrowdAgent,
            FTag_CrowdDiag_Tracked, FTag_CrowdAgent_Walking,
            ck::TReadOnly<FFragment_CrowdAgent_DesiredVelocity>, ck::TReadWrite<FFragment_CrowdAgent_DiagRecorder>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_AvoidanceSample>;
        using RunBefore = TDepList<FProcessor_CrowdAgent_AccelClamp>;
        using TProcessor::TProcessor;
        static auto ForEachEntity(TimeType, HandleType, const FFragment_CrowdAgent_DesiredVelocity&, FFragment_CrowdAgent_DiagRecorder&) -> void;
    };

    class CKCROWD_API FProcessor_CrowdAgent_DiagAccelTap : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DiagAccelTap, FCk_Handle_CrowdAgent,
            FTag_CrowdDiag_Tracked,
            ck::TReadOnly<FFragment_CrowdAgent_DesiredVelocity>, ck::TReadWrite<FFragment_CrowdAgent_DiagRecorder>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_AccelClamp>;
        using RunBefore = TDepList<FProcessor_CrowdAgent_VelocityBridge>;
        using TProcessor::TProcessor;
        static auto ForEachEntity(TimeType, HandleType, const FFragment_CrowdAgent_DesiredVelocity&, FFragment_CrowdAgent_DiagRecorder&) -> void;
    };

    class CKCROWD_API FProcessor_CrowdAgent_DiagVelocityBridgeTap : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DiagVelocityBridgeTap, FCk_Handle_CrowdAgent,
            FTag_CrowdDiag_Tracked,
            ck::TReadOnly<FFragment_Velocity_Current>, ck::TReadWrite<FFragment_CrowdAgent_DiagRecorder>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_VelocityBridge>;
        using RunBefore = TDepList<FProcessor_Velocity_Clamp>;
        using TProcessor::TProcessor;
        static auto ForEachEntity(TimeType, HandleType, const FFragment_Velocity_Current&, FFragment_CrowdAgent_DiagRecorder&) -> void;
    };

    class CKCROWD_API FProcessor_CrowdAgent_DiagApplyOffsetTap : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DiagApplyOffsetTap, FCk_Handle_CrowdAgent,
            FTag_CrowdDiag_Tracked,
            ck::TReadOnly<FFragment_CrowdAgent_PendingDisplacement>, ck::TReadWrite<FFragment_CrowdAgent_DiagRecorder>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_ApplyOffset>;
        using RunBefore = TDepList<FProcessor_CrowdAgent_PushApart>;
        using TProcessor::TProcessor;
        static auto ForEachEntity(TimeType, HandleType, const FFragment_CrowdAgent_PendingDisplacement&, FFragment_CrowdAgent_DiagRecorder&) -> void;
    };

    class CKCROWD_API FProcessor_CrowdAgent_DiagPushApartTap : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DiagPushApartTap, FCk_Handle_CrowdAgent,
            FTag_CrowdDiag_Tracked,
            ck::TReadOnly<FFragment_CrowdAgent_PendingDisplacement>, ck::TReadWrite<FFragment_CrowdAgent_DiagRecorder>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_PushApart>;
        using RunBefore = TDepList<FProcessor_CrowdAgent_ConstrainToNavmesh>;
        using TProcessor::TProcessor;
        static auto ForEachEntity(TimeType, HandleType, const FFragment_CrowdAgent_PendingDisplacement&, FFragment_CrowdAgent_DiagRecorder&) -> void;
    };

    // Per-frame recorder for tracked agents. Reads transform + bridged velocity + neighbor cache,
    // appends a sample to the recorder fragment at the cadence set by ck.Crowd.SampleHz
    // (default 20Hz), and updates the per-cycle running metrics (min-sep, dir-reversals, max
    // angular delta, time-to-goal). View-membership-keyed on FTag_CrowdDiag_Tracked so untagged
    // agents pay no overhead — the recorder is opt-in per agent.
    //
    // Physics, after ConstrainToNavmesh. It samples tracked agents only and captures the observed
    // Transform after the constraint stage has queued its offset request.
    class CKCROWD_API FProcessor_CrowdAgent_DiagRecorder : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DiagRecorder,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            FTag_CrowdDiag_Tracked,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_Velocity_Current>,
            ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            ck::TReadWrite<FFragment_CrowdAgent_DiagRecorder>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_ConstrainToNavmesh>;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_Velocity_Current& InCurrentVelocity,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_DiagRecorder& InRecorder) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
