#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Paints a UCk_NavArea_CrowdAgent COST disc under a physically stationary agent so fresh paths
    // route around standing crowds; see CkCrowd/CLAUDE.md. Cost, never a hole. Server-only:
    // pathfinding is server-authoritative and client worlds skip painting.
    class CKCROWD_API FProcessor_CrowdAgent_StationaryMarkup : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_StationaryMarkup,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadWrite<FFragment_CrowdAgent_NavMarkup>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_NavMarkup& InMarkup) -> void;

        // Shared with the EndPlay processor — lives here because this class is the fragment's friend.
        static auto
        Remove_Markup(
            FFragment_CrowdAgent_NavMarkup& InMarkup) -> void;

        // Monotonic, process-wide. Path installers stamp it onto the path; PathRefresh re-paths
        // only for discs whose serial is NEWER than the path's.
        static auto
        Get_CurrentPaintSerial() -> uint64;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Unregisters the painted area on entity teardown — the fragment's strong ptr alone would
    // keep the UObject alive but leave a stale registration in the nav octree.
    class CKCROWD_API FProcessor_CrowdAgent_NavMarkup_EndPlay : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_NavMarkup_EndPlay,
            FCk_Handle_CrowdAgent,
            ck::TReadWrite<FFragment_CrowdAgent_NavMarkup>,
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
            FFragment_CrowdAgent_NavMarkup& InMarkup) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
