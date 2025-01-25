#pragma once

#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

#include "CkShapes/Box/CkShapeBox_Fragment.h"
#include "CkShapes/Capsule/CkShapeCapsule_Fragment.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Fragment.h"
#include "CkShapes/Sphere/CkShapeSphere_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace JPH
{
    class PhysicsSystem;
}

namespace ck::details
{
    class CKSPATIALQUERY_API FProcessor_BoxProbe_Setup : public ck_exp::TProcessor<
            FProcessor_BoxProbe_Setup,
            FCk_Handle_Probe,
            FFragment_ShapeBox_Current,
            FFragment_Probe_Params,
            FFragment_Probe_Current,
            FTag_Probe_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Probe_NeedsSetup;

    public:
        FProcessor_BoxProbe_Setup(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeBox_Current& InShape,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent) -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_SphereProbe_Setup : public ck_exp::TProcessor<
            FProcessor_SphereProbe_Setup,
            FCk_Handle_Probe,
            FFragment_ShapeSphere_Current,
            FFragment_Probe_Params,
            FFragment_Probe_Current,
            FTag_Probe_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Probe_NeedsSetup;

    public:
        FProcessor_SphereProbe_Setup(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeSphere_Current& InShape,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent) -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_CylinderProbe_Setup : public ck_exp::TProcessor<
            FProcessor_CylinderProbe_Setup,
            FCk_Handle_Probe,
            FFragment_ShapeCylinder_Current,
            FFragment_Probe_Params,
            FFragment_Probe_Current,
            FTag_Probe_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Probe_NeedsSetup;

    public:
        FProcessor_CylinderProbe_Setup(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCylinder_Current& InShape,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent) -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_CapsuleProbe_Setup : public ck_exp::TProcessor<
            FProcessor_CapsuleProbe_Setup,
            FCk_Handle_Probe,
            FFragment_ShapeCapsule_Current,
            FFragment_Probe_Params,
            FFragment_Probe_Current,
            FTag_Probe_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_Probe_NeedsSetup;

    public:
        FProcessor_CapsuleProbe_Setup(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCapsule_Current& InShape,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent) -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };
}

namespace ck
{
    class CKSPATIALQUERY_API FProcessor_Probe_Setup
    {
    public:
        using TimeType = FCk_Time;
        using RegistryType = FCk_Registry;

    public:
        FProcessor_Probe_Setup(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

    public:
        auto
        Tick(
            TimeType InDeltaT) -> void;

    private:
        details::FProcessor_BoxProbe_Setup _Processor_BoxProbe;
        details::FProcessor_SphereProbe_Setup _Processor_SphereProbe;
        details::FProcessor_CapsuleProbe_Setup _Processor_CapsuleProbe;
        details::FProcessor_CylinderProbe_Setup _Processor_CylinderProbe;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_Probe_UpdateTransform : public ck_exp::TProcessor<
            FProcessor_Probe_UpdateTransform,
            FCk_Handle_Probe,
            FFragment_Probe_Params,
            FFragment_Probe_Current,
            TExclude<FTag_Probe_MotionType_Static>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        FProcessor_Probe_UpdateTransform(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent) -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_Probe_DebugDraw : public ck_exp::TProcessor<
            FProcessor_Probe_DebugDraw,
            FCk_Handle_Probe,
            FFragment_Probe_DebugInfo,
            CK_IGNORE_PENDING_KILL>
    {
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_DebugInfo& InDebugInfo) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_Probe_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Probe_HandleRequests,
            FCk_Handle_Probe,
            FFragment_Probe_Current,
            FFragment_Probe_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Probe_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Probe_Current& InCurrent,
            const FFragment_Probe_Requests& InRequestsComp) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Probe_Current& InCurrent,
            const FCk_Request_Probe_BeginOverlap& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Probe_Current& InCurrent,
            const FCk_Request_Probe_OverlapPersisted& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Probe_Current& InCurrent,
            const FCk_Request_Probe_EndOverlap& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_Probe_Teardown : public ck_exp::TProcessor<
            FProcessor_Probe_Teardown,
            FCk_Handle_Probe,
            FFragment_Probe_Params,
            FFragment_Probe_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        FProcessor_Probe_Teardown(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };
}

// --------------------------------------------------------------------------------------------------------------------
