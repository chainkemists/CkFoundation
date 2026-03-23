#pragma once

#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Fragment.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

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
    // ================================================================================================================
    // Traits: TProbeShapeFactory
    // ================================================================================================================

    template <typename T_ShapeFragment>
    struct TProbeShapeFactory;

    template <> struct TProbeShapeFactory<FFragment_ShapeBox_Current>;
    template <> struct TProbeShapeFactory<FFragment_ShapeSphere_Current>;
    template <> struct TProbeShapeFactory<FFragment_ShapeCapsule_Current>;
    template <> struct TProbeShapeFactory<FFragment_ShapeCylinder_Current>;

    // ================================================================================================================
    // TProcessor_ProbeSetup
    // ================================================================================================================

    template <typename T_ShapeFragment>
    class TProcessor_ProbeSetup : public ck_exp::TProcessor<
            TProcessor_ProbeSetup<T_ShapeFragment>,
            FCk_Handle_Probe,
            T_ShapeFragment,
            FFragment_Probe_Params,
            FFragment_Probe_Current,
            FFragment_Transform,
            FTag_Probe_NeedsSetup,
            TExclude<FTag_Transform_Updated>,
            TExclude<FTag_SceneNode_RelativeTransformUpdated>,
            CK_IGNORE_PENDING_KILL>
    {
        using Super = ck_exp::TProcessor<
            TProcessor_ProbeSetup<T_ShapeFragment>,
            FCk_Handle_Probe,
            T_ShapeFragment,
            FFragment_Probe_Params,
            FFragment_Probe_Current,
            FFragment_Transform,
            FTag_Probe_NeedsSetup,
            TExclude<FTag_Transform_Updated>,
            TExclude<FTag_SceneNode_RelativeTransformUpdated>,
            CK_IGNORE_PENDING_KILL>;

    public:
        using MarkedDirtyBy = FTag_Probe_NeedsSetup;
        using RegistryType = typename Super::RegistryType;
        using TimeType = typename Super::TimeType;
        using HandleType = typename Super::HandleType;

    public:
        TProcessor_ProbeSetup(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const T_ShapeFragment& InShape,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent,
            const FFragment_Transform& InTransform) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    // ================================================================================================================
    // TProcessor_ProbeUpdateShape
    // ================================================================================================================

    template <typename T_ShapeFragment>
    class TProcessor_ProbeUpdateShape : public ck_exp::TProcessor<
        TProcessor_ProbeUpdateShape<T_ShapeFragment>,
        FCk_Handle_Probe,
        T_ShapeFragment,
        FFragment_Probe_Current,
        FTag_Probe_ShapeUpdated,
        TExclude<FTag_Probe_NeedsSetup>,
        TExclude<FTag_Probe_Disabled>,
        CK_IGNORE_PENDING_KILL>
    {
        using Super = ck_exp::TProcessor<
            TProcessor_ProbeUpdateShape<T_ShapeFragment>,
            FCk_Handle_Probe,
            T_ShapeFragment,
            FFragment_Probe_Current,
            FTag_Probe_ShapeUpdated,
            TExclude<FTag_Probe_NeedsSetup>,
            TExclude<FTag_Probe_Disabled>,
            CK_IGNORE_PENDING_KILL>;

    public:
        using MarkedDirtyBy = FTag_Probe_ShapeUpdated;
        using RegistryType = typename Super::RegistryType;
        using TimeType = typename Super::TimeType;
        using HandleType = typename Super::HandleType;

    public:
        TProcessor_ProbeUpdateShape(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const T_ShapeFragment& InShape,
            FFragment_Probe_Current& InCurrent) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    // ================================================================================================================
    // Backwards-compatible type aliases
    // ================================================================================================================

    using FProcessor_BoxProbe_Setup = TProcessor_ProbeSetup<FFragment_ShapeBox_Current>;
    using FProcessor_SphereProbe_Setup = TProcessor_ProbeSetup<FFragment_ShapeSphere_Current>;
    using FProcessor_CapsuleProbe_Setup = TProcessor_ProbeSetup<FFragment_ShapeCapsule_Current>;
    using FProcessor_CylinderProbe_Setup = TProcessor_ProbeSetup<FFragment_ShapeCylinder_Current>;

    using FProcessor_BoxProbe_UpdateShape = TProcessor_ProbeUpdateShape<FFragment_ShapeBox_Current>;
    using FProcessor_SphereProbe_UpdateShape = TProcessor_ProbeUpdateShape<FFragment_ShapeSphere_Current>;
    using FProcessor_CapsuleProbe_UpdateShape = TProcessor_ProbeUpdateShape<FFragment_ShapeCapsule_Current>;
    using FProcessor_CylinderProbe_UpdateShape = TProcessor_ProbeUpdateShape<FFragment_ShapeCylinder_Current>;
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
            TExclude<FTag_Probe_NeedsSetup>,
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
            TExclude<FTag_Probe_NeedsSetup>,
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
            TExclude<FTag_Probe_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_DebugInfo& InDebugInfo,
            const FFragment_Transform& InTransform) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_Probe_DebugDrawAll : public ck_exp::TProcessor<
            FProcessor_Probe_DebugDrawAll,
            FCk_Handle_Probe,
            FFragment_Probe_DebugInfo,
            FFragment_Transform,
            TExclude<FTag_Probe_NeedsSetup>,
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
            TExclude<FTag_Probe_NeedsSetup>,
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

    class CKSPATIALQUERY_API FProcessor_Probe_EndPlay : public ck_exp::TProcessor<
            FProcessor_Probe_EndPlay,
            FCk_Handle_Probe,
            FFragment_Probe_Params,
            FFragment_Probe_Current,
            CK_IF_END_PLAY>
    {
    public:
        FProcessor_Probe_EndPlay(
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

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_Probe_UpdateShape
    {
    public:
        using TimeType = FCk_Time;
        using RegistryType = FCk_Registry;

    public:
        FProcessor_Probe_UpdateShape(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

    public:
        auto
            Tick(
                TimeType InDeltaT) -> void;

    private:
        details::FProcessor_BoxProbe_UpdateShape _Processor_BoxProbe;
        details::FProcessor_SphereProbe_UpdateShape _Processor_SphereProbe;
        details::FProcessor_CapsuleProbe_UpdateShape _Processor_CapsuleProbe;
        details::FProcessor_CylinderProbe_UpdateShape _Processor_CylinderProbe;
    };
}

// --------------------------------------------------------------------------------------------------------------------
