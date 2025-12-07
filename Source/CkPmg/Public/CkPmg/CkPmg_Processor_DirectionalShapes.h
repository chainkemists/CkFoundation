#pragma once

#include "CkPmg_Fragment.h"
#include "CkPmg_Fragment_DirectionalShapes.h"

#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPMG_API FProcessor_Pmg_Arrow_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Arrow_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_Arrow_Params,
            FFragment_Pmg_DebugShape_Common,
            FFragment_Pmg_DebugShape_Current,
            FTag_Pmg_DebugShape_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Pmg_DebugShape_NeedsSetup;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_Arrow_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_Pivot_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Pivot_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_Pivot_Params,
            FFragment_Pmg_DebugShape_Common,
            FFragment_Pmg_DebugShape_Current,
            FTag_Pmg_DebugShape_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Pmg_DebugShape_NeedsSetup;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_Pivot_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_DashedLine_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_DashedLine_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_DashedLine_Params,
            FFragment_Pmg_DebugShape_Common,
            FFragment_Pmg_DebugShape_Current,
            FTag_Pmg_DebugShape_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Pmg_DebugShape_NeedsSetup;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_DashedLine_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

} // namespace ck

// --------------------------------------------------------------------------------------------------------------------
