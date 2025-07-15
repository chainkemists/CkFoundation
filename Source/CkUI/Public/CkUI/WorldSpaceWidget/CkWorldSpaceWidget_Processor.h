#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcs/Transform/CkTransform_Fragment.h"

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
        using TProcessor::TProcessor;

    public:
        auto
    	ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform InTransform,
            FFragment_WorldSpaceWidget_Params& InParams,
            FFragment_WorldSpaceWidget_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKUI_API FProcessor_WorldSpaceWidget_Teardown : public ck_exp::TProcessor<
            FProcessor_WorldSpaceWidget_Teardown,
            FCk_Handle_WorldSpaceWidget,
            FFragment_WorldSpaceWidget_Params,
            FFragment_WorldSpaceWidget_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Transform_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto
    	ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_WorldSpaceWidget_Params& InParams,
            FFragment_WorldSpaceWidget_Current& InCurrent) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
