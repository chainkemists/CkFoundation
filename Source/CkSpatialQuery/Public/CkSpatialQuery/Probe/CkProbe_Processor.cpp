#include "CkProbe_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkEcs/Net/CkNet_Utils.h"

#include "CkShapes/Capsule/CkShapeCapsule_Utils.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Utils.h"
#include "CkShapes/Public/CkShapes/Box/CkShapeBox_Fragment_Data.h"
#include "CkShapes/Public/CkShapes/Box/CkShapeBox_Utils.h"
#include "CkShapes/Sphere/CkShapeSphere_Utils.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"
#include "CkSpatialQuery/CkSpatialQuery_Utils.h"
#include "CkSpatialQuery/Probe/CkProbe_Utils.h"
#include "CkSpatialQuery/Settings/CkSpatialQuery_Settings.h"

#include "Jolt/Jolt.h"
#include "Jolt/Core/Reference.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/Body/Body.h"
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Body/BodyInterface.h"
#include "Jolt/Physics/Collision/ActiveEdgeMode.h"
#include "Jolt/Physics/Collision/ShapeCast.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/CylinderShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::details
{
    // --------------------------------------------------------------------------------------------------------------------

    class ContactCastCollector : public JPH::CastShapeCollector
    {
    public:
        CK_GENERATED_BODY(ContactCastCollector);

    public:
        struct FCk_ProbeBeginOverlaps
        {
            CK_GENERATED_BODY(FCk_ProbeBeginOverlaps);

            FCk_Handle_Probe _Probe;
            TOptional<FCk_Request_Probe_BeginOverlap> _BeginOverlap;
            TOptional<FCk_Request_Probe_OverlapUpdated> _UpdateOverlap;

            CK_DEFINE_CONSTRUCTORS(FCk_ProbeBeginOverlaps, _Probe, _BeginOverlap, _UpdateOverlap);

            CK_PROPERTY_GET(_Probe);
            CK_PROPERTY_GET(_BeginOverlap);
            CK_PROPERTY_GET(_UpdateOverlap);
        };

    public:
        auto
            AddHit(
                const JPH::ShapeCastResult& inResult)
            -> void override
        {
            const auto Entity = static_cast<FCk_Entity::IdType>(_BodyInterface->GetUserData(inResult.mBodyID2));

            if (_ProbeHandle.Get_Entity().Get_ID() == Entity)
            { return; }

            const auto OtherProbe = UCk_Utils_Probe_UE::Cast(_ProbeHandle.Get_ValidHandle(Entity));

            {
                auto ContactPoints = TArray<FVector>{};
                ContactPoints.Emplace(ck::jolt::Conv(inResult.mContactPointOn1));

                _OverlappingProbes.Emplace(FCk_ProbeBeginOverlaps{
                    _ProbeHandle,
                    FCk_Request_Probe_BeginOverlap{
                        OtherProbe,
                        ContactPoints,
                        jolt::Conv(-inResult.mPenetrationAxis.Normalized()),
                        UCk_Utils_Probe_UE::Get_SurfaceInfo(OtherProbe).Get_PhysicalMaterial()
                    },
                    {}
                });

                _BeginOverlaps.Emplace(OtherProbe);
            }
            {
                auto ContactPoints = TArray<FVector>{};
                ContactPoints.Emplace(ck::jolt::Conv(inResult.mContactPointOn2));

                _OverlappingProbes.Emplace(FCk_ProbeBeginOverlaps{
                    OtherProbe,
                    FCk_Request_Probe_BeginOverlap{
                        _ProbeHandle,
                        ContactPoints,
                        jolt::Conv(-inResult.mPenetrationAxis.Normalized()),
                        UCk_Utils_Probe_UE::Get_SurfaceInfo(_ProbeHandle).Get_PhysicalMaterial()
                    },
                    {}
                });

                _BeginOverlaps.Emplace(_ProbeHandle);

            }
        }

    private:
        FCk_Handle_Probe _ProbeHandle;
        const JPH::BodyInterface* _BodyInterface;

        TSet<FCk_Handle_Probe> _BeginOverlaps;
        TArray<FCk_ProbeBeginOverlaps> _OverlappingProbes;

    public:
        CK_PROPERTY_GET(_OverlappingProbes);
        CK_PROPERTY_GET(_BeginOverlaps);

        CK_DEFINE_CONSTRUCTOR(ContactCastCollector, _ProbeHandle, _BodyInterface);
    };

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_BoxProbe_Setup::
    FProcessor_BoxProbe_Setup(
        const RegistryType& InRegistry,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem) {}

    auto
        FProcessor_BoxProbe_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeBox_Current& InShape,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent,
            const FFragment_Transform& InTransform) const
            -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        using namespace JPH;
        const auto& EntityPosition = InTransform.Get_Transform().GetLocation();

        const auto BoxParams = UCk_Utils_ShapeBox_UE::Get_Dimensions(UCk_Utils_ShapeBox_UE::Cast(InHandle));

        const auto& HalfExtents = BoxParams.Get_HalfExtents();

        const auto Settings = BoxShapeSettings{jolt::Conv(HalfExtents), BoxParams.Get_ConvexRadius()};
        Settings.SetEmbedded();

        auto ShapeResult = Settings.Create();
        auto Shape = ShapeResult.Get();

        InHandle.Add<JPH::Ref<JPH::Shape>>(Shape);

        auto ShapeSettings = BodyCreationSettings{
            Shape,
            jolt::Conv(EntityPosition),
            Quat::sIdentity(),
            EMotionType::Kinematic,
            ObjectLayer{1}
        };
        ShapeSettings.mIsSensor = true;

        switch (InParams.Get_MotionType())
        {
            case ECk_MotionType::Static: ShapeSettings.mMotionType = EMotionType::Static;
                InHandle.Add<FTag_Probe_MotionType_Static>();
                break;
            case ECk_MotionType::Kinematic: ShapeSettings.mMotionType = EMotionType::Kinematic;
                break;
            case ECk_MotionType::Dynamic: ShapeSettings.mMotionType = EMotionType::Dynamic;
                break;
        }

        switch (InParams.Get_MotionQuality())
        {
            case ECk_MotionQuality::Discrete: ShapeSettings.mMotionQuality = EMotionQuality::Discrete;
                break;
            case ECk_MotionQuality::LinearCast: ShapeSettings.mMotionQuality = EMotionQuality::LinearCast;
                break;
        }

        const auto& PhysicsSystem = _PhysicsSystem.Pin();
        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        const auto Body = BodyInterface.CreateBody(ShapeSettings);
        Body->SetUserData(static_cast<uint64>(InHandle.Get_Entity().Get_ID()));
        Body->SetCollideKinematicVsNonDynamic(true);

        InCurrent._BodyId = Body->GetID();

        if (InHandle.Has<FTag_Probe_LinearCast>())
        { return; }

        // Deactivate the body for LinearCast because we use ShapeCasts for LinearCast, since Jolt does
        // NOT support LinearCast for sensors.
        BodyInterface.AddBody(Body->GetID(),
            InHandle.Has<FTag_Probe_LinearCast>()
            ? EActivation::Activate
            : EActivation::DontActivate);
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_SphereProbe_Setup::
    FProcessor_SphereProbe_Setup(
        const RegistryType& InRegistry,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem) {}

    auto
        FProcessor_SphereProbe_Setup::
        DoTick(
            TimeType InDeltaT)
            -> void
    {
        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_SphereProbe_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeSphere_Current& InShape,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent,
            const FFragment_Transform& InTransform) const
            -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        using namespace JPH;
        const auto& EntityPosition = InTransform.Get_Transform().GetLocation();

        const auto Params = UCk_Utils_ShapeSphere_UE::Get_Dimensions(UCk_Utils_ShapeSphere_UE::Cast(InHandle));

        const auto& Radius = Params.Get_Radius();

        const auto Settings = SphereShapeSettings{Radius};
        Settings.SetEmbedded();

        auto ShapeResult = Settings.Create();
        auto Shape = ShapeResult.Get();

        InHandle.Add<JPH::Ref<JPH::Shape>>(Shape);

        auto ShapeSettings = BodyCreationSettings{
            Shape,
            jolt::Conv(EntityPosition),
            Quat::sIdentity(),
            EMotionType::Kinematic,
            ObjectLayer{1}
        };
        ShapeSettings.mIsSensor = true;

        switch (InParams.Get_MotionType())
        {
            case ECk_MotionType::Static:
            {
                ShapeSettings.mMotionType = EMotionType::Static;
                InHandle.Add<FTag_Probe_MotionType_Static>();
                break;
            }
            case ECk_MotionType::Kinematic:
            {
                ShapeSettings.mMotionType = EMotionType::Kinematic;
                break;
            }
            case ECk_MotionType::Dynamic:
            {
                ShapeSettings.mMotionType = EMotionType::Dynamic;
                break;
            }
        }

        switch (InParams.Get_MotionQuality())
        {
            case ECk_MotionQuality::Discrete:
            {
                ShapeSettings.mMotionQuality = EMotionQuality::Discrete;
                break;
            }
            case ECk_MotionQuality::LinearCast:
            {
                ShapeSettings.mMotionQuality = EMotionQuality::LinearCast;
                break;
            }
        }

        const auto& PhysicsSystem = _PhysicsSystem.Pin();
        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        const auto Body = BodyInterface.CreateBody(ShapeSettings);
        Body->SetUserData(static_cast<uint64>(InHandle.Get_Entity().Get_ID()));
        Body->SetCollideKinematicVsNonDynamic(true);

        InCurrent._BodyId = Body->GetID();

        if (InHandle.Has<FTag_Probe_LinearCast>())
        { return; }

        // Deactivate the body for LinearCast because we use ShapeCasts for LinearCast, since Jolt does
        // NOT support LinearCast for sensors.
        BodyInterface.AddBody(Body->GetID(),
            InHandle.Has<FTag_Probe_LinearCast>()
            ? EActivation::Activate
            : EActivation::DontActivate);
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_CylinderProbe_Setup::
    FProcessor_CylinderProbe_Setup(
        const RegistryType& InRegistry,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem) {}

    auto
        FProcessor_CylinderProbe_Setup::
        DoTick(
            TimeType InDeltaT)
            -> void
    {
        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_CylinderProbe_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCylinder_Current& InShape,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent,
            const FFragment_Transform& InTransform)
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        using namespace JPH;
        const auto& EntityPosition = InTransform.Get_Transform().GetLocation();

        const auto Params = UCk_Utils_ShapeCylinder_UE::Get_Dimensions(UCk_Utils_ShapeCylinder_UE::Cast(InHandle));

        const auto& HalfHeight = Params.Get_HalfHeight();
        const auto& Radius = Params.Get_Radius();
        const auto& ConvexRadius = Params.Get_ConvexRadius();

        const auto Settings = CylinderShapeSettings{HalfHeight, Radius, ConvexRadius};
        Settings.SetEmbedded();

        auto ShapeResult = Settings.Create();
        auto Shape = ShapeResult.Get();

        InHandle.Add<JPH::Ref<JPH::Shape>>(Shape);

        auto ShapeSettings = BodyCreationSettings{
            Shape,
            jolt::Conv(EntityPosition),
            Quat::sIdentity(),
            EMotionType::Kinematic,
            ObjectLayer{1}
        };
        ShapeSettings.mIsSensor = true;

        switch (InParams.Get_MotionType())
        {
            case ECk_MotionType::Static: ShapeSettings.mMotionType = EMotionType::Static;
                InHandle.Add<FTag_Probe_MotionType_Static>();
                break;
            case ECk_MotionType::Kinematic: ShapeSettings.mMotionType = EMotionType::Kinematic;
                break;
            case ECk_MotionType::Dynamic: ShapeSettings.mMotionType = EMotionType::Dynamic;
                break;
        }

        switch (InParams.Get_MotionQuality())
        {
            case ECk_MotionQuality::Discrete: ShapeSettings.mMotionQuality = EMotionQuality::Discrete;
                break;
            case ECk_MotionQuality::LinearCast: ShapeSettings.mMotionQuality = EMotionQuality::LinearCast;
                break;
        }

        const auto& PhysicsSystem = _PhysicsSystem.Pin();
        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        const auto Body = BodyInterface.CreateBody(ShapeSettings);
        Body->SetUserData(static_cast<uint64>(InHandle.Get_Entity().Get_ID()));
        Body->SetCollideKinematicVsNonDynamic(true);

        InCurrent._BodyId = Body->GetID();

        if (InHandle.Has<FTag_Probe_LinearCast>())
        { return; }

        // Deactivate the body for LinearCast because we use ShapeCasts for LinearCast, since Jolt does
        // NOT support LinearCast for sensors.
        BodyInterface.AddBody(Body->GetID(),
            InHandle.Has<FTag_Probe_LinearCast>()
            ? EActivation::Activate
            : EActivation::DontActivate);
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_CapsuleProbe_Setup::
    FProcessor_CapsuleProbe_Setup(
        const RegistryType& InRegistry,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem) {}

    auto
        FProcessor_CapsuleProbe_Setup::
        DoTick(
            TimeType InDeltaT)
            -> void
    {
        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_CapsuleProbe_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCapsule_Current& InShape,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent,
            const FFragment_Transform& InTransform) const
            -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        using namespace JPH;
        const auto& EntityPosition = InTransform.Get_Transform().GetLocation();

        const auto Params = UCk_Utils_ShapeCapsule_UE::Get_Dimensions(UCk_Utils_ShapeCapsule_UE::Cast(InHandle));

        const auto HalfHeight = Params.Get_HalfHeight();
        const auto Radius = Params.Get_Radius();

        const auto Settings = CapsuleShapeSettings{HalfHeight, Radius};
        Settings.SetEmbedded();

        auto ShapeResult = Settings.Create();
        auto Shape = ShapeResult.Get();

        InHandle.Add<JPH::Ref<JPH::Shape>>(Shape);

        auto ShapeSettings = BodyCreationSettings{
            Shape,
            jolt::Conv(EntityPosition),
            Quat::sIdentity(),
            EMotionType::Kinematic,
            ObjectLayer{1}
        };
        ShapeSettings.mIsSensor = true;

        switch (InParams.Get_MotionType())
        {
            case ECk_MotionType::Static: ShapeSettings.mMotionType = EMotionType::Static;
                InHandle.Add<FTag_Probe_MotionType_Static>();
                break;
            case ECk_MotionType::Kinematic: ShapeSettings.mMotionType = EMotionType::Kinematic;
                break;
            case ECk_MotionType::Dynamic: ShapeSettings.mMotionType = EMotionType::Dynamic;
                break;
        }

        switch (InParams.Get_MotionQuality())
        {
            case ECk_MotionQuality::Discrete: ShapeSettings.mMotionQuality = EMotionQuality::Discrete;
                break;
            case ECk_MotionQuality::LinearCast: ShapeSettings.mMotionQuality = EMotionQuality::LinearCast;
                break;
        }

        const auto& PhysicsSystem = _PhysicsSystem.Pin();
        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        const auto Body = BodyInterface.CreateBody(ShapeSettings);
        Body->SetUserData(static_cast<uint64>(InHandle.Get_Entity().Get_ID()));
        Body->SetCollideKinematicVsNonDynamic(true);

        InCurrent._BodyId = Body->GetID();

        if (InHandle.Has<FTag_Probe_LinearCast>())
        { return; }

        // Deactivate the body for LinearCast because we use ShapeCasts for LinearCast, since Jolt does
        // NOT support LinearCast for sensors.
        BodyInterface.AddBody(Body->GetID(),
            InHandle.Has<FTag_Probe_LinearCast>()
            ? EActivation::Activate
            : EActivation::DontActivate);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FProcessor_Probe_Setup::
    FProcessor_Probe_Setup(
        const RegistryType& InRegistry,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : _Processor_BoxProbe(InRegistry, InPhysicsSystem)
        , _Processor_SphereProbe(InRegistry, InPhysicsSystem)
        , _Processor_CapsuleProbe(InRegistry, InPhysicsSystem)
        , _Processor_CylinderProbe(InRegistry, InPhysicsSystem) {}

    auto
        FProcessor_Probe_Setup::
        Tick(
            TimeType InDeltaT)
            -> void
    {
        _Processor_BoxProbe.Tick(InDeltaT);
        _Processor_SphereProbe.Tick(InDeltaT);
        _Processor_CapsuleProbe.Tick(InDeltaT);
        _Processor_CylinderProbe.Tick(InDeltaT);
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_Probe_UpdateTransform::
    FProcessor_Probe_UpdateTransform(
        const RegistryType& InRegistry,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem) {}

    auto
        FProcessor_Probe_UpdateTransform::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_Params& InParams,
            const FFragment_Probe_Current& InCurrent,
            const FFragment_Transform& InTransform) const
            -> void
    {
        const auto EntityPosition = InTransform.Get_Transform().GetLocation();
        const auto EntityRotation = InTransform.Get_Transform().GetRotation();

        const auto EntityRotationQuat = FQuat{EntityRotation};
        const auto Rot = jolt::Conv(EntityRotationQuat);

        const auto& PhysicsSystem = _PhysicsSystem.Pin();
        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        BodyInterface.MoveKinematic(InCurrent.Get_BodyId(), jolt::Conv(EntityPosition), Rot,
            InDeltaT.Get_Seconds());
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_Probe_UpdateTransform_LinearCast::
    FProcessor_Probe_UpdateTransform_LinearCast(
        const RegistryType& InRegistry,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem) { }

    auto
        FProcessor_Probe_UpdateTransform_LinearCast::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Probe_Current& InCurrent,
            const FFragment_Transform_Previous& InPreviousTransform,
            const FFragment_Transform& InTransform) const
        -> void
    {
        using namespace JPH;

        auto Settings = ShapeCastSettings{};
        Settings.mBackFaceModeTriangles = EBackFaceMode::CollideWithBackFaces;
        Settings.mBackFaceModeConvex = EBackFaceMode::CollideWithBackFaces;
        Settings.mActiveEdgeMode = EActiveEdgeMode::CollideOnlyWithActive;
        Settings.mUseShrunkenShapeAndConvexRadius = true;
        Settings.mReturnDeepestPoint = false;

        const auto& PhysicsSystem = _PhysicsSystem.Pin();
        const auto& BodyInterface = PhysicsSystem->GetBodyInterface();
        const auto& Shape = BodyInterface.GetShape(InCurrent.Get_BodyId());

        const auto& PrevTransform = InPreviousTransform.Get_Transform();
        const auto& PrevLocation = InPreviousTransform.Get_Transform().GetLocation();
        const auto& CurrLocation = InTransform.Get_Transform().GetLocation();

        auto ShapeCast = RShapeCast{
            Shape,
            jolt::Conv(PrevTransform.GetScale3D()),
            jolt::Conv(PrevTransform),
            jolt::Conv(CurrLocation) - jolt::Conv(PrevLocation)
        };

        auto Collector = details::ContactCastCollector{InHandle, &BodyInterface};
        PhysicsSystem->GetNarrowPhaseQuery().CastShape(ShapeCast, Settings, Vec3::sReplicate(0.0f), Collector);

        const auto& OverlappingProbes = Collector.Get_OverlappingProbes();
        const auto& BeginOverlaps = Collector.Get_BeginOverlaps();

        for (const auto& Overlap : OverlappingProbes)
        {
            auto Probe = Overlap.Get_Probe();

            // Other LinearCast Probes will do their own overlaps
            if (Probe != InHandle && Probe.Has<FTag_Probe_LinearCast>())
            { continue; }

            if (ck::IsValid(Overlap.Get_BeginOverlap()))
            {
                UCk_Utils_Probe_UE::Request_BeginOverlap(Probe, *Overlap.Get_BeginOverlap());
            }
            else
            {
                UCk_Utils_Probe_UE::Request_OverlapUpdated(Probe, *Overlap.Get_UpdateOverlap());
            }

            //if (const auto& CurrentOverlappingProbes = Probe.Get<FFragment_Probe_Current>().Get_CurrentOverlaps();
            //    CurrentOverlappingProbes.Contains(FCk_Probe_OverlapInfo{Overlap.Get_OtherEntity()}))
            //{
            //    UCk_Utils_Probe_UE::Request_OverlapUpdated(Handle, FCk_Request_Probe_OverlapUpdated{Overlap});
            //    continue;
            //}

            //UCk_Utils_Probe_UE::Request_BeginOverlap(Handle, Overlap);
        }

        for (auto Overlap : InCurrent.Get_CurrentOverlaps())
        {
            auto OtherEntity = Overlap.Get_OtherEntity();

            if (BeginOverlaps.Contains(OtherEntity))
            { continue; }

            UCk_Utils_Probe_UE::Request_EndOverlap(InHandle, FCk_Request_Probe_EndOverlap{OtherEntity});
            UCk_Utils_Probe_UE::Request_EndOverlap(OtherEntity, FCk_Request_Probe_EndOverlap{InHandle});
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_Probe_HandleRequests::
    FProcessor_Probe_HandleRequests(
        const RegistryType& InRegistry,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem) {}

    auto
        FProcessor_Probe_EnsureStaticNotMoved_DEBUG::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent)
            -> void
    {
        CK_TRIGGER_ENSURE(TEXT("Probe [{}] with MotionType [{}] had its Transform changed.\n"
                "If this Probe is meant to move its MotionType shouldn't be [{}]"),
            InHandle,
            InParams.Get_MotionType(),
            InParams.Get_MotionType());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Probe_DebugDrawAll::
        DoTick(
            TimeType InDeltaT)
            -> void
    {
        if (NOT UCk_Utils_SpatialQuery_Settings::Get_DebugPreviewAllProbes())
        { return; }

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_Probe_DebugDrawAll::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_DebugInfo& InDebugInfo,
            const FFragment_Transform& InTransform)
        -> void
    {
        using namespace JPH;

        auto EntityPosition = InTransform.Get_Transform().GetLocation();
        auto EntityRotation = InTransform.Get_Transform().GetRotation();

        const auto& LineThickness = InDebugInfo.Get_LineThickness();
        const auto& DebugColor =
            UCk_Utils_Probe_UE::Get_IsEnabledDisabled(InHandle) == ECk_EnableDisable::Disable
            ? InDebugInfo.Get_DisabledColor().ToFColor(true)
            : UCk_Utils_Probe_UE::Get_IsOverlapping(InHandle)
            ? InDebugInfo.Get_OverlapColor().ToFColor(true)
            : InDebugInfo.Get_Color().ToFColor(true);

        const auto& Shape = InHandle.Get<Ref<JPH::Shape>>();

        if (ck::Is_NOT_Valid(Shape.GetPtr(), ck::IsValid_Policy_NullptrOnly{}))
        {
            return;
        }

        Shape::GetTrianglesContext IoContext;
        auto Mat4 = Mat44::sIdentity();
        Mat4.SetTranslation(jolt::Conv(EntityPosition));
        auto Bounds = Shape->GetWorldSpaceBounds(Mat4, Vec3{1.f, 1.f, 1.f});

        Shape->GetTrianglesStart(IoContext, Bounds, jolt::Conv(EntityPosition), jolt::Conv(EntityRotation),
            JPH::Vec3{1.f, 1.f, 1.f});

        auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

        Float3 Vertices[Shape::cGetTrianglesMinTrianglesRequested * 3];

        for (auto NumTris = Shape->GetTrianglesNext(IoContext, Shape->cGetTrianglesMinTrianglesRequested,
                 Vertices);
             NumTris != 0;)
        {
            for (auto Tri = 0; Tri < NumTris; ++Tri)
            {
                auto Index = Tri * 3;
                DrawDebugLine(World, jolt::Conv(Vertices[Index + 0]), jolt::Conv(Vertices[Index + 1]),
                    DebugColor,
                    false, 0, 0, LineThickness);
                DrawDebugLine(World, jolt::Conv(Vertices[Index + 1]), jolt::Conv(Vertices[Index + 2]),
                    DebugColor,
                    false, 0, 0, LineThickness);
                DrawDebugLine(World, jolt::Conv(Vertices[Index + 2]), jolt::Conv(Vertices[Index + 0]),
                    DebugColor,
                    false, 0, 0, LineThickness);
            }

            NumTris = Shape->GetTrianglesNext(IoContext, Shape->cGetTrianglesMinTrianglesRequested, Vertices);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Probe_HandleRequests::
        DoTick(
            TimeType InDeltaT)
            -> void
    {
        _TransientEntity.Clear<FTag_Probe_Updated>();

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_Probe_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Probe_Current& InCurrent,
            const FFragment_Probe_Requests& InRequestsComp) const
            -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](
            FFragment_Probe_Requests& InRequests)
            {
                algo::ForEachRequest(InRequests._Requests, Visitor([&](
                    const auto& InRequest)
                    {
                        DoHandleRequest(InHandle, InCurrent, InRequest);
                    }));
            });
    }

    auto
        FProcessor_Probe_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Probe_Current& InCurrent,
            const FCk_Request_Probe_BeginOverlap& InRequest)
            -> void
    {
        const auto OverlapInfo = FCk_Probe_OverlapInfo{InRequest.Get_OtherEntity()}
                                 .Set_ContactPoints(InRequest.Get_ContactPoints())
                                 .Set_ContactNormal(InRequest.Get_ContactNormal());

        if (InCurrent._CurrentOverlaps.Contains(OverlapInfo))
        {
            DoHandleRequest(InHandle, InCurrent, FCk_Request_Probe_OverlapUpdated{InRequest});
            return;
        }

        InCurrent._CurrentOverlaps.Add(OverlapInfo);

        UCk_Utils_Probe_UE::Request_MarkProbe_AsOverlapping(InHandle);

        const auto Payload = FCk_Probe_Payload_OnBeginOverlap{
            InRequest.Get_OtherEntity(),
            InRequest.Get_ContactPoints(),
            InRequest.Get_ContactNormal(),
            InRequest.Get_PhysicalMaterial()
        };

        UUtils_Signal_OnProbeBeginOverlap::Broadcast(InHandle, MakePayload(InHandle, Payload));
    }

    auto
        FProcessor_Probe_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Probe_Current& InCurrent,
            const FCk_Request_Probe_OverlapUpdated& InRequest)
            -> void
    {
        const auto OverlapInfo = FCk_Probe_OverlapInfo{InRequest.Get_OtherEntity()}
                                 .Set_ContactPoints(InRequest.Get_ContactPoints())
                                 .Set_ContactNormal(InRequest.Get_ContactNormal());

        if (NOT InCurrent._CurrentOverlaps.Contains(OverlapInfo))
        {
            DoHandleRequest(InHandle, InCurrent, FCk_Request_Probe_BeginOverlap{InRequest});
            return;
        }

        const auto Payload = FCk_Probe_Payload_OnOverlapUpdated{
            InRequest.Get_OtherEntity(),
            InRequest.Get_ContactPoints(),
            InRequest.Get_ContactNormal(),
            InRequest.Get_PhysicalMaterial()
        };

        InCurrent._CurrentOverlaps.Add(OverlapInfo);

        UUtils_Signal_OnProbeOverlapUpdated::Broadcast(InHandle, MakePayload(InHandle, Payload));
    }

    auto
        FProcessor_Probe_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Probe_Current& InCurrent,
            const FCk_Request_Probe_EndOverlap& InRequest)
            -> void
    {
        const auto OverlapInfo = FCk_Probe_OverlapInfo{InRequest.Get_OtherEntity()};

        const auto& NumRemovedItems = InCurrent._CurrentOverlaps.Remove(OverlapInfo);

        CK_ENSURE_IF_NOT(NumRemovedItems > 0,
            TEXT("Received EndOverlap Request for Probe [{}] with Other Entity [{}], but it was NOT overlapping with it."),
            InHandle,
            InRequest.Get_OtherEntity())
        {
            return;
        }

        if (InCurrent.Get_CurrentOverlaps().IsEmpty())
        {
            UCk_Utils_Probe_UE::Request_MarkProbe_AsNotOverlapping(InHandle);
        }

        UUtils_Signal_OnProbeEndOverlap::Broadcast(InHandle,
            MakePayload(InHandle, FCk_Probe_Payload_OnEndOverlap{InRequest.Get_OtherEntity()}));
    }

    auto
        FProcessor_Probe_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Probe_Current& InCurrent,
            const FCk_Request_Probe_EnableDisable& InRequest) const
            -> void
    {
        const auto& PhysicsSystem = _PhysicsSystem.Pin();

        switch (InRequest.Get_EnableDisable())
        {
            case ECk_EnableDisable::Enable:
            {
                if (InHandle.Try_Remove<ck::FTag_Probe_Disabled>() == 0)
                { return; }

                if (NOT InHandle.Has<FTag_Probe_LinearCast>())
                {
                    PhysicsSystem->GetBodyInterface().AddBody(InCurrent.Get_BodyId(), JPH::EActivation::Activate);
                }

                UUtils_Signal_OnProbeEnableDisable::Broadcast(InHandle,
                    MakePayload(InHandle, FCk_Probe_Payload_OnEnableDisable{ECk_EnableDisable::Enable}));
                break;
            }
            case ECk_EnableDisable::Disable:
            {
                if (InHandle.Has<FTag_Probe_Disabled>())
                { return; }

                if (NOT InHandle.Has<FTag_Probe_LinearCast>())
                {
                    PhysicsSystem->GetBodyInterface().RemoveBody(InCurrent.Get_BodyId());
                }

                InHandle.AddOrGet<ck::FTag_Probe_Disabled>();

                UUtils_Signal_OnProbeEnableDisable::Broadcast(InHandle,
                    MakePayload(InHandle, FCk_Probe_Payload_OnEnableDisable{ECk_EnableDisable::Disable}));
                break;
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_Probe_Teardown::
    FProcessor_Probe_Teardown(
        const RegistryType& InRegistry,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem) {}

    auto
        FProcessor_Probe_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_Params& InParams,
            const FFragment_Probe_Current& InCurrent) const
            -> void
    {
        const auto& DoManuallyTriggerAllEndOverlaps = [&]() -> void
        {
            for (const auto& OverlapInfo : InCurrent.Get_CurrentOverlaps())
            {
                const auto& OtherEntity = OverlapInfo.Get_OtherEntity();

                UUtils_Signal_OnProbeEndOverlap::Broadcast(InHandle,
                    MakePayload(InHandle, FCk_Probe_Payload_OnEndOverlap{OtherEntity}));

                if (auto OtherEntityAsProbe = UCk_Utils_Probe_UE::Cast(OtherEntity); ck::IsValid(OtherEntity) &&
                    UCk_Utils_Probe_UE::Get_IsOverlappingWith(OtherEntityAsProbe, InHandle))
                {
                    UCk_Utils_Probe_UE::Request_EndOverlap(OtherEntityAsProbe, FCk_Request_Probe_EndOverlap{InHandle});
                }
            }
        };

        const auto& PhysicsSystem = _PhysicsSystem.Pin();
        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        if (UCk_Utils_Probe_UE::Get_IsEnabledDisabled(InHandle) == ECk_EnableDisable::Enable)
        {
            DoManuallyTriggerAllEndOverlaps();

            if (NOT InHandle.Has<FTag_Probe_LinearCast>())
            {
                BodyInterface.RemoveBody(InCurrent.Get_BodyId());
            }
        }

        if (NOT InHandle.Has<FTag_Probe_LinearCast>())
        {
            BodyInterface.DestroyBody(InCurrent.Get_BodyId());
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
