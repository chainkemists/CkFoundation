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
        FFragment_IskmRenderer_Current,
        FTag_IskmRenderer_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
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
