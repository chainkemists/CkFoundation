#pragma once

#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorDescriptor.h"
#include "CkEcs/Subsystem/CkEcsEditor_Subsystem.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Editor-only preview for Probe entities. Draws a uniform sphere indicator at the probe's
    // transform so designers can see where probes sit in the level viewport without entering PIE.
    // The real probe shape (from CkShapes) is NOT rendered — see Decision D8 in the plan for the
    // richer per-shape wireframe follow-up.
    class CKSPATIALQUERY_API FProcessor_Probe_Preview_EditorTime : public ck_exp::TProcessor<
            FProcessor_Probe_Preview_EditorTime,
            FCk_Handle_Probe,
            ck::TReadOnly<FFragment_Transform>,
            CK_IF_EDITOR_ONLY_ENTITY,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::EditorOnly;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
