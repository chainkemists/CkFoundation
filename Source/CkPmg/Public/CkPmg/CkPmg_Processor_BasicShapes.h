#pragma once

#include "CkPmg_Fragment.h"
#include "CkPmg_Fragment_BasicShapes.h"

#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPMG_API FProcessor_Pmg_Sphere_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Sphere_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_Sphere_Params,
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
            const FFragment_Pmg_Sphere_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_Box_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Box_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_Box_Params,
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
            const FFragment_Pmg_Box_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_Cone_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Cone_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_Cone_Params,
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
            const FFragment_Pmg_Cone_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_Cylinder_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Cylinder_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_Cylinder_Params,
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
            const FFragment_Pmg_Cylinder_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_Capsule_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Capsule_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_Capsule_Params,
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
            const FFragment_Pmg_Capsule_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_Pyramid_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Pyramid_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_Pyramid_Params,
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
            const FFragment_Pmg_Pyramid_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_Hemisphere_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Hemisphere_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_Hemisphere_Params,
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
            const FFragment_Pmg_Hemisphere_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_Torus_Setup : public ck_exp::TProcessor<
            FProcessor_Pmg_Torus_Setup,
            FCk_Handle_Pmg_DebugShape,
            FFragment_Pmg_Torus_Params,
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
            const FFragment_Pmg_Torus_Params& InParams,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
