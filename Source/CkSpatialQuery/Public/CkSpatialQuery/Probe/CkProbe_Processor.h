#pragma once

#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcs/Transform/CkTransform_Fragment.h"

#include "CkShapes/Box/CkShapeBox_Fragment.h"
#include "CkShapes/Capsule/CkShapeCapsule_Fragment.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Fragment.h"
#include "CkShapes/Sphere/CkShapeSphere_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

// ReSharper disable once CppInconsistentNaming
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
            FFragment_Transform,
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
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeBox_Current& InShape,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent,
            const FFragment_Transform& InTransform) const -> void;

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
            FFragment_Transform,
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
            FFragment_Probe_Current& InCurrent,
            const FFragment_Transform& InTransform) const -> void;

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
            FFragment_Transform,
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
            FFragment_Probe_Current& InCurrent,
            const FFragment_Transform& InTransform) -> void;

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
            FFragment_Transform,
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
            FFragment_Probe_Current& InCurrent,
            const FFragment_Transform& InTransform) const -> void;

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
            FFragment_Transform,
            FTag_Transform_Updated,
            TExclude<FTag_Probe_LinearCast>,
            TExclude<FTag_Probe_MotionType_Static>,
            TExclude<FTag_Probe_Disabled>,
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
            const FFragment_Probe_Current& InCurrent,
            const FFragment_Transform& InTransform) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_Probe_UpdateTransform_LinearCast : public ck_exp::TProcessor<
            FProcessor_Probe_UpdateTransform_LinearCast,
            FCk_Handle_Probe,
            FFragment_Probe_Current,
            FFragment_Transform_Previous,
            FFragment_Transform,
            FTag_Probe_LinearCast,
            TExclude<FTag_Probe_Disabled>,
            TExclude<FTag_Probe_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        FProcessor_Probe_UpdateTransform_LinearCast(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Probe_Current& InCurrent,
            const FFragment_Transform_Previous& InPreviousTransform,
            const FFragment_Transform& InTransform) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_ProbeTrace : public ck_exp::TProcessor<
        FProcessor_ProbeTrace,
        FCk_Handle_ProbeTrace,
        FFragment_Probe_Request_RayCast,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Probe_Requests;

        FProcessor_ProbeTrace(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_Request_RayCast& InRequest) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_Probe_EnsureStaticNotMoved_DEBUG : public ck_exp::TProcessor<
            FProcessor_Probe_EnsureStaticNotMoved_DEBUG,
            FCk_Handle_Probe,
            FFragment_Probe_Params,
            FFragment_Probe_Current,
            FTag_Probe_MotionType_Static,
            FTag_Transform_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_Probe_DebugDraw : public ck_exp::TProcessor<
            FProcessor_Probe_DebugDraw,
            FCk_Handle_Probe,
            FTag_Probe_DebugDraw,
            FFragment_Probe_DebugInfo,
            FFragment_Transform,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_DebugInfo& InDebugInfo,
            const FFragment_Transform& InTransform) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_Probe_DebugDrawAll : public ck_exp::TProcessor<
            FProcessor_Probe_DebugDrawAll,
            FCk_Handle_Probe,
            FFragment_Probe_DebugInfo,
            FFragment_Transform,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            FCk_Time InDeltaT) -> void;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_DebugInfo& InDebugInfo,
            const FFragment_Transform& InTransform) -> void;
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
        FProcessor_Probe_HandleRequests(
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
            const FCk_Request_Probe_OverlapUpdated& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Probe_Current& InCurrent,
            const FCk_Request_Probe_EndOverlap& InRequest) -> void;

        auto
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Probe_Current& InCurrent,
            const FCk_Request_Probe_EnableDisable& InRequest) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
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
            const FFragment_Probe_Current& InCurrent) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };
}

// --------------------------------------------------------------------------------------------------------------------
