#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkCrowd/Agent/CkCrowdAgent_DebugBody_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"   // for FTag_CrowdAgent_HasProbe

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Setup processor — runs ONCE per agent, gated on FTag_CrowdAgent_HasProbe (the agent has
    // composed its probe child, so steering features are stable). Excludes
    // FTag_CrowdAgent_DebugBody_Setup so it never re-fires on the same agent.
    //
    // Spawns two PMG child entities owned by the agent: a filled body capsule and a forward-
    // facing cone (using the new ECk_Pmg_ConeOrientation::Forward orientation so the apex is
    // baked along +X). Both are SceneNode-parented to the agent's transform. Both use
    // PMG `InDuration = -1.0f` so they persist until the agent (their parent) is destroyed —
    // the framework's parent-child cascade handles cleanup automatically.
    //
    // Stamps FFragment_CrowdAgent_DebugBody (with the child handles) and the setup tag.
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
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams) const -> void;
    };

    // Update processor — per-tick. Reads UCk_Crowd_DebugSettings's DrawAgentBody toggle and the
    // agent's state tags (PathPending → yellow blend, Asleep → desaturate), updates each shape's
    // RenderMode (DoubleSided / Hidden) and color via the Pmg utils. Mesh topology is never
    // rebuilt; only the lightweight properties get updated.
    //
    // DoTick gates: when the toggle is OFF and was OFF last tick too, skip the view iteration
    // entirely. The on→off transition still iterates one final time to flip every shape Hidden.
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
        // Tracks the previous tick's toggle state so an on→off transition can fire one final
        // pass to flip every shape Hidden, without iterating every subsequent tick when the
        // toggle stays off.
        bool _LastTickToggleOn = false;
    };
}

// --------------------------------------------------------------------------------------------------------------------
