#pragma once

#include "CkPmg_Fragment.h"
#include "CkPmg_Fragment_SymbolShapes.h"

#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPMG_API FProcessor_Pmg_MagnifyingGlass_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_MagnifyingGlass_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_MagnifyingGlass_Params,
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
            const FFragment_Pmg_MagnifyingGlass_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_QuestionMark_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_QuestionMark_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_QuestionMark_Params,
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
            const FFragment_Pmg_QuestionMark_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_ExclamationMark_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_ExclamationMark_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_ExclamationMark_Params,
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
            const FFragment_Pmg_ExclamationMark_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_Flag_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Flag_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_Flag_Params,
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
            const FFragment_Pmg_Flag_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_Pin_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Pin_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_Pin_Params,
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
            const FFragment_Pmg_Pin_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
