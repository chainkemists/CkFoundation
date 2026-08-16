#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_ConstrainToNavmesh_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_PushApart_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Answers "is the navmesh clamp eating this agent's movement, and why" without changing it.
    // Every production fragment is read-only here and the clamp's math is replicated rather than
    // shared: the same NavData resolution, the same projection extents, the same
    // FindMoveAlongSurface walk. Gated entirely on ck.Crowd.DiagNavClip (default 0), which is
    // checked before any nav query or state write, so an off diagnostic costs one CVar read.
    //
    // It must run BEFORE FProcessor_CrowdAgent_ConstrainToNavmesh, which zeroes
    // FFragment_CrowdAgent_PendingDisplacement as it consumes it — the same slot
    // FProcessor_CrowdAgent_DiagPushApartTap occupies.
    //
    // The view deliberately does NOT require FTag_CrowdAgent_Walking: a PathPending agent keeps
    // integrating and can be clipped exactly like a walking one. Idle agents are skipped by
    // movement state instead, so the tag combination stays out of the view filter.
    class CKCROWD_API FProcessor_CrowdAgent_DiagNavClip : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DiagNavClip,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_CrowdAgent_PendingDisplacement>,
            ck::TReadOnly<FFragment_CrowdAgent_DesiredVelocity>,
            ck::TReadOnly<FFragment_CrowdAgent_PathFollow>,
            TExclude<FTag_CrowdAgent_Asleep>,
            TExclude<FTag_CrowdAgent_Flying>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_PushApart>;
        using RunBefore = TDepList<FProcessor_CrowdAgent_ConstrainToNavmesh>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_PendingDisplacement& InPending,
            const FFragment_CrowdAgent_DesiredVelocity& InDesiredVelocity,
            const FFragment_CrowdAgent_PathFollow& InPathFollow) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
