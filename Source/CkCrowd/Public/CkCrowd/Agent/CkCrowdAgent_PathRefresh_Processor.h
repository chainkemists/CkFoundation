#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_StationaryMarkup_Processor.h"

#include "CkNavigation/Nav/CkNav_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Mid-walk path refresh — the second half of StationaryMarkup; see CkCrowd/CLAUDE.md.
    // One-shot per (path, disc-set): path and discs are both static, so a miss now is a miss
    // forever, and a path that legitimately PAID a disc's cost is never re-planned for it again.
    // Server-only in effect: only the server paints discs, so the client gather is always empty.
    class CKCROWD_API FProcessor_CrowdAgent_PathRefresh : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_PathRefresh,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            FTag_CrowdAgent_Walking,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_Nav_PathResult>,
            ck::TReadWrite<FFragment_CrowdAgent_PathFollow>,
            ck::TReadWrite<FFragment_CrowdAgent_BlockDetect>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_CrowdAgent_StationaryMarkup>;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(FCk_Time InDeltaT) -> void;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_Nav_PathResult& InPathResult,
            FFragment_CrowdAgent_PathFollow& InPathFollow,
            FFragment_CrowdAgent_BlockDetect& InBlockDetect) const -> void;

        // For an agent standing inside a painted cost disc whose goal is outside it, returns a
        // query start just outside the band — planning from its feet re-picks "through", since
        // the toll is per distance crossed. UNSET means "plan from the real location", which is
        // the correct answer for every other case, not a failure.
        static auto
        Get_EscapedQueryStart(
            FCk_Handle InAnyWorldHandle,
            FCk_Entity InSelfEntity,
            const FVector& InSelfLocation,
            const FVector& InGoal,
            float InAgentRadius) -> TOptional<FVector>;

    private:
        struct FSettledDisc
        {
            FCk_Entity _Owner;
            FVector _Center = FVector::ZeroVector;
            float _Radius = 0.0f;
            uint64 _PaintSerial = 0;
        };

        // Rebuilt in DoTick; read by the per-entity pass the same tick.
        TArray<FSettledDisc> _SettledDiscs;
        uint64 _MaxEligibleSerial = 0;
    };
}

// --------------------------------------------------------------------------------------------------------------------
