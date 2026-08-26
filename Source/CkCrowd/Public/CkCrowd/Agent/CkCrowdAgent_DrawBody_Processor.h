#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkCrowd/Agent/CkCrowdAgent_DebugBody_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"   // for FTag_CrowdAgent_HasProbe

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Spawns the agent's body capsule + forward cone as PMG child entities SceneNode-parented to
    // the agent's transform, then stamps FFragment_CrowdAgent_DebugBody and the setup tag. Gated
    // on HasProbe (steering features are composed by then); the setup-tag exclude makes it once-per-agent.
    //
    // While ck.Crowd.Debug.AgentBody is OFF (the default), DoTick skips the whole pass and agents
    // simply stay pending — so a packaged run carries ZERO debug-body PMG entities instead of two
    // per agent (each a SceneNode child that fed the Pmg transform/visibility processors every
    // frame). Flipping the cvar on spawns every pending body on the next tick.
    class CKCROWD_API FProcessor_CrowdAgent_DrawBody_Setup : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DrawBody_Setup,
            FCk_Handle_CrowdAgent,
            FTag_CrowdAgent_HasProbe,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            TExclude<FTag_CrowdAgent_DebugBody_Setup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            FCk_Time InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams) const -> void;
    };

    // Per-tick refresh of each shape's visibility and colour from the DrawAgentBody toggle and the
    // agent's state tags (PathPending → yellow blend, Asleep → desaturate). Mesh topology is never
    // rebuilt.
    class CKCROWD_API FProcessor_CrowdAgent_DrawBody_Update : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DrawBody_Update,
            FCk_Handle_CrowdAgent,
            ck::TReadWrite<FFragment_CrowdAgent_DebugBody>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            FCk_Time InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_CrowdAgent_DebugBody& InDebugBody) const -> void;

    private:
        // Lets the on→off transition fire one final Hidden pass without iterating every tick after.
        bool _LastTickToggleOn = false;
    };
}

// --------------------------------------------------------------------------------------------------------------------
