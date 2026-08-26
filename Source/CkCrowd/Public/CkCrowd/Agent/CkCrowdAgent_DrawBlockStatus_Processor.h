#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkCrowd/Agent/CkCrowdAgent_DebugBlockMarker_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Draws WHERE a held agent is stuck and WHO is holding it.
    //
    // This exists because the block tier was the one first-class agent state with no visualization:
    // the cause enum, the blocker handle and the crowd depth were all readable through
    // FFragment_CrowdAgent_BlockDetect and the OnGoalBlocked signal, but nothing rendered them, so
    // telling a HELD agent from a merely-slow one meant reading a log. DrawNavStatus is not that
    // overlay and never was — it reports PATH TROUBLE (the nav query layer,
    // FFragment_CrowdAgent_PathTrouble). An agent held by another body has no path trouble at all
    // and draws nothing there, which is precisely the case that was invisible.
    //
    // The TEXT half deliberately does NOT live here. Per-agent state belongs in the entity debug
    // overlay's Crowd provider (`FCk_DebugOverlay_Provider_Crowd`), which already owns the
    // field/severity/compact-token presentation for this feature; a second, differently-styled
    // world-space text layer competing with it was the presentation problem. This processor owns
    // only the world-space GEOMETRY that overlay cannot express: where the body is standing and
    // which other body it is waiting on.

    // Lazily creates the floor disc for an agent the first time it is blocked. Excluded once the
    // marker fragment exists, so it is once-per-agent, and gated on GoalBlocked, so an agent that
    // is never held never allocates a mesh entity at all.
    class CKCROWD_API FProcessor_CrowdAgent_DrawBlockStatus_Setup : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DrawBlockStatus_Setup,
            FCk_Handle_CrowdAgent,
            FTag_CrowdAgent_GoalBlocked,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            TExclude<FFragment_CrowdAgent_DebugBlockMarker>,
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

    // Per-tick refresh: disc visibility follows (toggle AND currently-blocked), disc colour follows
    // the block cause, and the blocker link is drawn immediate-mode. Mesh topology is never rebuilt.
    //
    // The link is deliberately NOT a retained PMG shape: its two endpoints are two independently
    // moving entities, so a retained line would need its transform rewritten every tick anyway —
    // which is exactly the per-frame cost retained geometry exists to avoid. The disc, whose
    // transform is fixed in the agent's own local space, is the half that benefits from being
    // retained.
    class CKCROWD_API FProcessor_CrowdAgent_DrawBlockStatus_Update : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DrawBlockStatus_Update,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_Transform>,
            ck::TReadOnly<FFragment_CrowdAgent_BlockDetect>,
            ck::TReadWrite<FFragment_CrowdAgent_DebugBlockMarker>,
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
            const FFragment_Transform& InTransform,
            const FFragment_CrowdAgent_BlockDetect& InBlockDetect,
            FFragment_CrowdAgent_DebugBlockMarker& InMarker) const -> void;

    private:
        // Lets the on -> off transition run one final pass to hide every disc without iterating
        // every tick afterwards. Mirrors DrawBody_Update.
        bool _LastTickToggleOn = false;
    };

    // A PMG shape is an ENTITY: dropping the handle leaks it. The parent cascade covers the normal
    // agent-destroyed path, but this makes the ownership explicit and covers a marker whose agent
    // outlives it.
    class CKCROWD_API FProcessor_CrowdAgent_DrawBlockStatus_EndPlay : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_DrawBlockStatus_EndPlay,
            FCk_Handle_CrowdAgent,
            ck::TReadWrite<FFragment_CrowdAgent_DebugBlockMarker>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_CrowdAgent_DebugBlockMarker& InMarker) const -> void;
    };
}
