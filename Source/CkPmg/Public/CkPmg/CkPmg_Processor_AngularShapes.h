#pragma once

#include "CkPmg_Fragment.h"
#include "CkPmg_Fragment_AngularShapes.h"

#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPMG_API FProcessor_Pmg_Wedge_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Wedge_Setup,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadOnly<FFragment_Pmg_Wedge_Params>,
            ck::TReadOnly<FFragment_Pmg_DebugShape_Common>,
            ck::TReadWrite<FFragment_Pmg_DebugShape_Current>,
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
            const FFragment_Pmg_Wedge_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_Arc_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Arc_Setup,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadOnly<FFragment_Pmg_Arc_Params>,
            ck::TReadOnly<FFragment_Pmg_DebugShape_Common>,
            ck::TReadWrite<FFragment_Pmg_DebugShape_Current>,
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
            const FFragment_Pmg_Arc_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_WedgeCone_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_WedgeCone_Setup,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadOnly<FFragment_Pmg_WedgeCone_Params>,
            ck::TReadOnly<FFragment_Pmg_DebugShape_Common>,
            ck::TReadWrite<FFragment_Pmg_DebugShape_Current>,
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
            const FFragment_Pmg_WedgeCone_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
