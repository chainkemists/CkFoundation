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

#include "CkIsmRenderer/Public/CkIsmRenderer/Proxy/CkIsmProxy_Processor.h"

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
        // Shares MarkedDirtyBy (FTag_Transform_Updated) with the IsmProxy PostTransform processors;
        // traces read instance transforms after they are pushed. Declared to order the cross-module
        // pairs and silence the scheduler's dirty-marker-conflict advisory.
        using RunAfter = TDepList<FProcessor_IsmProxy_TransformInstance, FProcessor_IsmProxy_EnsureStaticNotMoved_DEBUG>;
        // Performs a line trace from cached transforms and broadcasts UUtils_Signal_OnRaySenseTraceHit on hit.
        // Pump would re-trace and re-broadcast the same hit within the same frame; FTag_Transform_Updated is sticky here.
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;

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
        // Shares MarkedDirtyBy (FTag_Transform_Updated) with the IsmProxy PostTransform processors;
        // traces read instance transforms after they are pushed. Declared to order the cross-module
        // pairs and silence the scheduler's dirty-marker-conflict advisory.
        using RunAfter = TDepList<FProcessor_IsmProxy_TransformInstance, FProcessor_IsmProxy_EnsureStaticNotMoved_DEBUG, FProcessor_RaySense_LineTrace_Update>;
        // Sweeps a box from cached transforms and broadcasts UUtils_Signal_OnRaySenseTraceHit on hit (via DoSweepTrace helper).
        // Pump would re-sweep and re-broadcast the same hit within the same frame; FTag_Transform_Updated is sticky here.
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;

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
        // Shares MarkedDirtyBy (FTag_Transform_Updated) with the IsmProxy PostTransform processors;
        // traces read instance transforms after they are pushed. Declared to order the cross-module
        // pairs and silence the scheduler's dirty-marker-conflict advisory.
        using RunAfter = TDepList<FProcessor_IsmProxy_TransformInstance, FProcessor_IsmProxy_EnsureStaticNotMoved_DEBUG, FProcessor_RaySense_BoxSweep_Update>;
        // Sweeps a sphere from cached transforms and broadcasts UUtils_Signal_OnRaySenseTraceHit on hit (via DoSweepTrace helper).
        // Pump would re-sweep and re-broadcast the same hit within the same frame; FTag_Transform_Updated is sticky here.
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;

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
        // Shares MarkedDirtyBy (FTag_Transform_Updated) with the IsmProxy PostTransform processors;
        // traces read instance transforms after they are pushed. Declared to order the cross-module
        // pairs and silence the scheduler's dirty-marker-conflict advisory.
        using RunAfter = TDepList<FProcessor_IsmProxy_TransformInstance, FProcessor_IsmProxy_EnsureStaticNotMoved_DEBUG, FProcessor_RaySense_SphereSweep_Update>;
        // Sweeps a capsule from cached transforms and broadcasts UUtils_Signal_OnRaySenseTraceHit on hit (via DoSweepTrace helper).
        // Pump would re-sweep and re-broadcast the same hit within the same frame; FTag_Transform_Updated is sticky here.
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;

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
        // Shares MarkedDirtyBy (FTag_Transform_Updated) with the IsmProxy PostTransform processors;
        // traces read instance transforms after they are pushed. Declared to order the cross-module
        // pairs and silence the scheduler's dirty-marker-conflict advisory.
        using RunAfter = TDepList<FProcessor_IsmProxy_TransformInstance, FProcessor_IsmProxy_EnsureStaticNotMoved_DEBUG, FProcessor_RaySense_CapsuleSweep_Update>;

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
        // Also writes FFragment_RaySense_Current (as do the 5 *_Update processors). Declaration order
        // puts the Updates first; RunAfter the chain tail (Cylinder) makes that write-ordering explicit
        // — transitively orders HandleRequests after all 5 Updates via their chain.
        using RunAfter = TDepList<FProcessor_RaySense_CylinderSweep_Update>;

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