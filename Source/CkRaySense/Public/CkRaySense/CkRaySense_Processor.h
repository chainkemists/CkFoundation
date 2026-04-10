#pragma once

#include "CkRaySense_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkShapes/Public/CkShapes/Box/CkShapeBox_Fragment.h"
#include "CkShapes/Public/CkShapes/Capsule/CkShapeCapsule_Fragment.h"
#include "CkShapes/Public/CkShapes/Cylinder/CkShapeCylinder_Fragment.h"
#include "CkShapes/Public/CkShapes/Sphere/CkShapeSphere_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKRAYSENSE_API FProcessor_RaySense_LineTrace_Update : public ck_exp::TProcessor<
            FProcessor_RaySense_LineTrace_Update,
            FCk_Handle_RaySense,
            ck::TReadOnly<FFragment_RaySense_Params>,
            ck::TReadWrite<FFragment_RaySense_Current>,
            ck::TReadOnly<FFragment_Transform_Previous>,
            ck::TReadOnly<FFragment_Transform>,
            FTag_Transform_Updated,
            TExclude<FFragment_ShapeBox_Current>,
            TExclude<FFragment_ShapeCapsule_Current>,
            TExclude<FFragment_ShapeSphere_Current>,
            TExclude<FFragment_ShapeCylinder_Current>,
            TExclude<FTag_RaySense_Disabled>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using MarkedDirtyBy = FTag_Transform_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_RaySense_Params& InParams,
            FFragment_RaySense_Current& InCurrent,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRAYSENSE_API FProcessor_RaySense_BoxSweep_Update : public ck_exp::TProcessor<
            FProcessor_RaySense_BoxSweep_Update,
            FCk_Handle_RaySense,
            ck::TReadOnly<FFragment_ShapeBox_Current>,
            ck::TReadOnly<FFragment_RaySense_Params>,
            ck::TReadWrite<FFragment_RaySense_Current>,
            ck::TReadOnly<FFragment_Transform_Previous>,
            ck::TReadOnly<FFragment_Transform>,
            FTag_Transform_Updated,
            TExclude<FTag_RaySense_Disabled>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using MarkedDirtyBy = FTag_Transform_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeBox_Current& InShape,
            const FFragment_RaySense_Params& InParams,
            FFragment_RaySense_Current& InCurrent,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRAYSENSE_API FProcessor_RaySense_SphereSweep_Update : public ck_exp::TProcessor<
            FProcessor_RaySense_SphereSweep_Update,
            FCk_Handle_RaySense,
            ck::TReadOnly<FFragment_ShapeSphere_Current>,
            ck::TReadOnly<FFragment_RaySense_Params>,
            ck::TReadWrite<FFragment_RaySense_Current>,
            ck::TReadOnly<FFragment_Transform_Previous>,
            ck::TReadOnly<FFragment_Transform>,
            FTag_Transform_Updated,
            TExclude<FTag_RaySense_Disabled>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using MarkedDirtyBy = FTag_Transform_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeSphere_Current& InShape,
            const FFragment_RaySense_Params& InParams,
            FFragment_RaySense_Current& InCurrent,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRAYSENSE_API FProcessor_RaySense_CapsuleSweep_Update : public ck_exp::TProcessor<
            FProcessor_RaySense_CapsuleSweep_Update,
            FCk_Handle_RaySense,
            ck::TReadOnly<FFragment_ShapeCapsule_Current>,
            ck::TReadOnly<FFragment_RaySense_Params>,
            ck::TReadWrite<FFragment_RaySense_Current>,
            ck::TReadOnly<FFragment_Transform_Previous>,
            ck::TReadOnly<FFragment_Transform>,
            FTag_Transform_Updated,
            TExclude<FTag_RaySense_Disabled>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using MarkedDirtyBy = FTag_Transform_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCapsule_Current& InShape,
            const FFragment_RaySense_Params& InParams,
            FFragment_RaySense_Current& InCurrent,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRAYSENSE_API FProcessor_RaySense_CylinderSweep_Update : public ck_exp::TProcessor<
            FProcessor_RaySense_CylinderSweep_Update,
            FCk_Handle_RaySense,
            ck::TReadOnly<FFragment_ShapeCylinder_Current>,
            ck::TReadOnly<FFragment_RaySense_Params>,
            ck::TReadWrite<FFragment_RaySense_Current>,
            ck::TReadOnly<FFragment_Transform_Previous>,
            ck::TReadOnly<FFragment_Transform>,
            FTag_Transform_Updated,
            TExclude<FTag_RaySense_Disabled>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_PostTransform;
        using MarkedDirtyBy = FTag_Transform_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCylinder_Current& InShape,
            const FFragment_RaySense_Params& InParams,
            FFragment_RaySense_Current& InCurrent,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKRAYSENSE_API FProcessor_RaySense_HandleRequests : public ck_exp::TProcessor<
        FProcessor_RaySense_HandleRequests,
        FCk_Handle_RaySense,
        ck::TReadOnly<FFragment_RaySense_Params>,
        ck::TReadWrite<FFragment_RaySense_Current>,
        ck::TReadOnly<FFragment_RaySense_Requests>,
        CK_IGNORE_PENDING_KILL>

    {
    public:
        using Group = FGroup_PostTransform;
        using MarkedDirtyBy = FFragment_RaySense_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
            ForEachEntity(
                TimeType InDeltaT,
                HandleType InHandle,
                const FFragment_RaySense_Params& InParams,
                FFragment_RaySense_Current& InCurrent,
                const FFragment_RaySense_Requests& InRaySenseComp) const -> void;

    private:
        static auto
            DoHandleRequest(
                HandleType InHandle,
                FFragment_RaySense_Current& InCurrent,
                const FCk_Request_RaySense_EnableDisable& InRequestsComp) -> void;
    };

}

// --------------------------------------------------------------------------------------------------------------------