#pragma once

#include "CkRaySense_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkShapes/Public/CkShapes/Box/CkShapeBox_Fragment.h"
#include "CkShapes/Public/CkShapes/Capsule/CkShapeCapsule_Fragment.h"
#include "CkShapes/Public/CkShapes/Cylinder/CkShapeCylinder_Fragment.h"
#include "CkShapes/Public/CkShapes/Sphere/CkShapeSphere_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKRAYSENSE_API FProcessor_RaySense_LineTrace_Update : public ck_exp::TProcessor<
            FProcessor_RaySense_LineTrace_Update,
            FCk_Handle_RaySense,
            FFragment_RaySense_Params,
            FFragment_RaySense_Current,
            FFragment_Transform_Previous,
            FFragment_Transform,
            FTag_Transform_Updated,
            TExclude<FFragment_ShapeBox_Current>,
            TExclude<FFragment_ShapeCapsule_Current>,
            TExclude<FFragment_ShapeSphere_Current>,
            TExclude<FFragment_ShapeCylinder_Current>,
            CK_IGNORE_PENDING_KILL>
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
            const FFragment_RaySense_Params& InParams,
            FFragment_RaySense_Current& InCurrent,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRAYSENSE_API FProcessor_RaySense_BoxSweep_Update : public ck_exp::TProcessor<
            FProcessor_RaySense_BoxSweep_Update,
            FCk_Handle_RaySense,
            FFragment_ShapeBox_Current,
            FFragment_RaySense_Params,
            FFragment_RaySense_Current,
            FFragment_Transform_Previous,
            FFragment_Transform,
            FTag_Transform_Updated,
            CK_IGNORE_PENDING_KILL>
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
            const FFragment_ShapeBox_Current& InShape,
            const FFragment_RaySense_Params& InParams,
            FFragment_RaySense_Current& InCurrent,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRAYSENSE_API FProcessor_RaySense_SphereSweep_Update : public ck_exp::TProcessor<
            FProcessor_RaySense_SphereSweep_Update,
            FCk_Handle_RaySense,
            FFragment_ShapeSphere_Current,
            FFragment_RaySense_Params,
            FFragment_RaySense_Current,
            FFragment_Transform_Previous,
            FFragment_Transform,
            FTag_Transform_Updated,
            CK_IGNORE_PENDING_KILL>
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
            const FFragment_ShapeSphere_Current& InShape,
            const FFragment_RaySense_Params& InParams,
            FFragment_RaySense_Current& InCurrent,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRAYSENSE_API FProcessor_RaySense_CapsuleSweep_Update : public ck_exp::TProcessor<
            FProcessor_RaySense_CapsuleSweep_Update,
            FCk_Handle_RaySense,
            FFragment_ShapeCapsule_Current,
            FFragment_RaySense_Params,
            FFragment_RaySense_Current,
            FFragment_Transform_Previous,
            FFragment_Transform,
            FTag_Transform_Updated,
            CK_IGNORE_PENDING_KILL>
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
            const FFragment_ShapeCapsule_Current& InShape,
            const FFragment_RaySense_Params& InParams,
            FFragment_RaySense_Current& InCurrent,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRAYSENSE_API FProcessor_RaySense_CylinderSweep_Update : public ck_exp::TProcessor<
            FProcessor_RaySense_CylinderSweep_Update,
            FCk_Handle_RaySense,
            FFragment_ShapeCylinder_Current,
            FFragment_RaySense_Params,
            FFragment_RaySense_Current,
            FFragment_Transform_Previous,
            FFragment_Transform,
            FTag_Transform_Updated,
            CK_IGNORE_PENDING_KILL>
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
            const FFragment_ShapeCylinder_Current& InShape,
            const FFragment_RaySense_Params& InParams,
            FFragment_RaySense_Current& InCurrent,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------