#pragma once

#include "CkPmg_Fragment.h"
#include "CkPmg_Fragment_IconShapes.h"

#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPMG_API FProcessor_Pmg_Warning_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Warning_Setup,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadOnly<FFragment_Pmg_Warning_Params>,
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
            const FFragment_Pmg_Warning_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_Prohibition_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Prohibition_Setup,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadOnly<FFragment_Pmg_Prohibition_Params>,
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
            const FFragment_Pmg_Prohibition_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_NoEntry_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_NoEntry_Setup,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadOnly<FFragment_Pmg_NoEntry_Params>,
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
            const FFragment_Pmg_NoEntry_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_InfoCircle_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_InfoCircle_Setup,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadOnly<FFragment_Pmg_InfoCircle_Params>,
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
            const FFragment_Pmg_InfoCircle_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
