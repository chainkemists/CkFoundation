#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkPathNetwork/Network/CkPathNetwork_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Gated on ck.PathNetwork.DebugDraw > 0 (>= 2 also draws chunk-grid bounds). Server world only.
    class CKPATHNETWORK_API FProcessor_PathNetwork_DebugDraw : public ck_exp::TProcessor<
        FProcessor_PathNetwork_DebugDraw,
        FCk_Handle_PathNetwork,
        ck::TReadOnly<FFragment_PathNetwork_Graph>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetwork_Graph& InGraph) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Gated on ck.PathNetwork.DebugDraw > 0. Legs are colored by type: on-ribbon orange, off-path cyan.
    class CKPATHNETWORK_API FProcessor_PathNetworkFollower_DebugDrawCorridor : public ck_exp::TProcessor<
        FProcessor_PathNetworkFollower_DebugDrawCorridor,
        FCk_Handle_PathNetworkFollower,
        ck::TReadOnly<FFragment_PathNetworkFollower_Corridor>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetworkFollower_Corridor& InCorridor) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
