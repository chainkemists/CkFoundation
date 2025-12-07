#pragma once

#include "CkPmg_Fragment.h"
#include "CkPmg_Fragment_FlatShapes.h"

#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPMG_API FProcessor_Pmg_Circle_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Circle_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_Circle_Params,
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
            const FFragment_Pmg_Circle_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_Plane_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Plane_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_Plane_Params,
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
            const FFragment_Pmg_Plane_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_Ring_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Ring_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_Ring_Params,
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
            const FFragment_Pmg_Ring_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
};

// --------------------------------------------------------------------------------------------------------------------

class CKPMG_API FProcessor_Pmg_Cross_Setup : public ck_exp::TProcessor<
        FProcessor_Pmg_Cross_Setup,
        FCk_Handle_Pmg_DebugShape,
        FFragment_Pmg_Cross_Params,
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
        const FFragment_Pmg_Cross_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
        -> void;
};

// --------------------------------------------------------------------------------------------------------------------

class CKPMG_API FProcessor_Pmg_Star_Setup : public ck_exp::TProcessor<
        FProcessor_Pmg_Star_Setup,
        FCk_Handle_Pmg_DebugShape,
        FFragment_Pmg_Star_Params,
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
        const FFragment_Pmg_Star_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
        -> void;
};

// --------------------------------------------------------------------------------------------------------------------

class CKPMG_API FProcessor_Pmg_Checkmark_Setup : public ck_exp::TProcessor<
        FProcessor_Pmg_Checkmark_Setup,
        FCk_Handle_Pmg_DebugShape,
        FFragment_Pmg_Checkmark_Params,
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
        const FFragment_Pmg_Checkmark_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
        -> void;
};

// --------------------------------------------------------------------------------------------------------------------

class CKPMG_API FProcessor_Pmg_Diamond_Setup : public ck_exp::TProcessor<
        FProcessor_Pmg_Diamond_Setup,
        FCk_Handle_Pmg_DebugShape,
        FFragment_Pmg_Diamond_Params,
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
        const FFragment_Pmg_Diamond_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
        -> void;
};
}

// --------------------------------------------------------------------------------------------------------------------
