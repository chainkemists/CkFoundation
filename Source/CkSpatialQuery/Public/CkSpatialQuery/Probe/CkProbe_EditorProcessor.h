#pragma once

#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorDescriptor.h"
#include "CkEcs/Subsystem/CkEcsEditor_Subsystem.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkShapes/Box/CkShapeBox_Fragment.h"
#include "CkShapes/Capsule/CkShapeCapsule_Fragment.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Fragment.h"
#include "CkShapes/Sphere/CkShapeSphere_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR

namespace ck
{
    // Editor-only probe preview. Templated over the shape fragment so one implementation handles
    // all four CkShapes variants (Box/Sphere/Capsule/Cylinder). The actual draw dispatches to an
    // overloaded DrawProbeShape helper per shape type. Runtime probe processors require Jolt
    // physics and can't run in editor, which is why this exists.
    template <typename T_ShapeFragment>
    class TProcessor_Probe_Preview_EditorTime : public ck_exp::TProcessor<
            TProcessor_Probe_Preview_EditorTime<T_ShapeFragment>,
            FCk_Handle_Probe,
            ck::TReadOnly<T_ShapeFragment>,
            ck::TReadOnly<FFragment_Probe_DebugInfo>,
            ck::TReadOnly<FFragment_Transform>,
            CK_IF_EDITOR_ONLY_ENTITY,
            CK_IGNORE_PENDING_KILL>
    {
        using Super = ck_exp::TProcessor<
            TProcessor_Probe_Preview_EditorTime<T_ShapeFragment>,
            FCk_Handle_Probe,
            ck::TReadOnly<T_ShapeFragment>,
            ck::TReadOnly<FFragment_Probe_DebugInfo>,
            ck::TReadOnly<FFragment_Transform>,
            CK_IF_EDITOR_ONLY_ENTITY,
            CK_IGNORE_PENDING_KILL>;

    public:
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::EditorOnly;
        using TimeType = typename Super::TimeType;
        using HandleType = typename Super::HandleType;

    public:
        using Super::Super;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const T_ShapeFragment& InShape,
            const FFragment_Probe_DebugInfo& InDebugInfo,
            const FFragment_Transform& InTransform) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    using FProcessor_Probe_Preview_Box_EditorTime      = TProcessor_Probe_Preview_EditorTime<FFragment_ShapeBox_Current>;
    using FProcessor_Probe_Preview_Sphere_EditorTime   = TProcessor_Probe_Preview_EditorTime<FFragment_ShapeSphere_Current>;
    using FProcessor_Probe_Preview_Capsule_EditorTime  = TProcessor_Probe_Preview_EditorTime<FFragment_ShapeCapsule_Current>;
    using FProcessor_Probe_Preview_Cylinder_EditorTime = TProcessor_Probe_Preview_EditorTime<FFragment_ShapeCylinder_Current>;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
