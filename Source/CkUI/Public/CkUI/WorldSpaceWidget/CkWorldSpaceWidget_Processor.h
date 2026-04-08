#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkUI/WorldSpaceWidget/CkWorldSpaceWidget_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKUI_API FProcessor_WorldSpaceWidget_UpdateLocation : public ck_exp::TProcessor<
            FProcessor_WorldSpaceWidget_UpdateLocation,
            FCk_Handle_WorldSpaceWidget,
            FFragment_Transform,
            FFragment_WorldSpaceWidget_Params,
            FFragment_WorldSpaceWidget_Current,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_WorldSpaceWidget_Params& InParams,
            const FFragment_WorldSpaceWidget_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKUI_API FProcessor_WorldSpaceWidget_UpdateScaling : public ck_exp::TProcessor<
            FProcessor_WorldSpaceWidget_UpdateScaling,
            FCk_Handle_WorldSpaceWidget,
            FFragment_Transform,
            FFragment_WorldSpaceWidget_Params,
            FFragment_WorldSpaceWidget_Current,
            FTag_WorldSpaceWidget_NeedsUpdateScaling,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using RunAfter = TDepList<FProcessor_WorldSpaceWidget_UpdateLocation>;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_WorldSpaceWidget_Params& InParams,
            const FFragment_WorldSpaceWidget_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKUI_API FProcessor_WorldSpaceWidget_EndPlay : public ck_exp::TProcessor<
            FProcessor_WorldSpaceWidget_EndPlay,
            FCk_Handle_WorldSpaceWidget,
            FFragment_WorldSpaceWidget_Params,
            FFragment_WorldSpaceWidget_Current,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_PreDestruction;
        using MarkedDirtyBy = FTag_Transform_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_WorldSpaceWidget_Params& InParams,
            FFragment_WorldSpaceWidget_Current& InCurrent) -> void;
    };
}
