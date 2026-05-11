#pragma once

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkIskmRenderer/Renderer/CkIskmRenderer_Fragment.h"
#include "CkIskmRenderer/Renderer/CkIskmRenderer_Fragment_Data.h"

namespace ck
{
    class CKISKMRENDERER_API FProcessor_IskmRenderer_Setup : public ck_exp::TProcessor<
        FProcessor_IskmRenderer_Setup,
        FCk_Handle_IskmRenderer,
        TReadOnly<FFragment_IskmRenderer_Params>,
        TReadWrite<FFragment_IskmRenderer_Current>,
        FTag_IskmRenderer_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        // Renderer Setup must run before any IskmProxy_Setup so the proxy can
        // read the freshly-assigned _RendererActor on its first attempt.
        // Both processors live in FGroup_Gameplay_Rendering with no explicit
        // intra-group ordering, so within a single group registration order
        // wins — and proxy ends up registered first. Pulling Renderer Setup
        // up to FGroup_Gameplay_Audio (one phase earlier) guarantees the
        // proxy sees a valid actor by the time it polls the fragment.
        using Group = FGroup_Gameplay_Audio;
        using MarkedDirtyBy = FTag_IskmRenderer_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_IskmRenderer_Params& InParams,
            FFragment_IskmRenderer_Current& InCurrent) const -> void;
    };
}
