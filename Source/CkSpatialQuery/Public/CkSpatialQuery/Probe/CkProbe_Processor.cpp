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

#include <DrawDebugHelpers.h>

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Probe_EnsureStaticNotMoved_DEBUG);
CK_REGISTER_PROCESSOR(ck::FProcessor_Probe_DebugDraw);
CK_REGISTER_PROCESSOR(ck::FProcessor_Probe_DebugDrawAll);

#define CK_PROBE_FACTORY(ProcessorType) \
    CK_REGISTER_PROCESSOR_WITH_FACTORY(ProcessorType, \
        [](const FCk_Registry& InRegistry) -> ck::concepts::FTickableType \
        { \
            const auto& PhysicsSystem = InRegistry.GetContext<TWeakPtr<JPH::PhysicsSystem>>(); \
            return ProcessorType{InRegistry, PhysicsSystem}; \
        })

CK_PROBE_FACTORY(ck::FProcessor_Probe_Setup);
CK_PROBE_FACTORY(ck::FProcessor_Probe_UpdateTransform);
CK_PROBE_FACTORY(ck::FProcessor_Probe_UpdateTransform_LinearCast);
CK_PROBE_FACTORY(ck::FProcessor_Probe_HandleRequests);
CK_PROBE_FACTORY(ck::FProcessor_Probe_EndPlay);
CK_PROBE_FACTORY(ck::FProcessor_Probe_UpdateShape);

#undef CK_PROBE_FACTORY

namespace ck_probe
{
    auto
        OnBoxDimensionsChanged(
            FCk_Handle_ShapeBox InHandle,
            FCk_ShapeBox_Dimensions InDimensions)
        -> void
    {
        InHandle.Add<ck::FTag_Probe_ShapeUpdated>();
    }

    auto
        OnSphereDimensionsChanged(
            FCk_Handle_ShapeSphere InHandle,
            FCk_ShapeSphere_Dimensions InDimensions)
        -> void
    {
        InHandle.Add<ck::FTag_Probe_ShapeUpdated>();
    }

    auto
        OnCapsuleDimensionsChanged(
            FCk_Handle_ShapeCapsule InHandle,
            FCk_ShapeCapsule_Dimensions InDimensions)
        -> void
    {
        InHandle.Add<ck::FTag_Probe_ShapeUpdated>();
    }

    auto
        OnCylinderDimensionsChanged(
            FCk_Handle_ShapeCylinder InHandle,
            FCk_ShapeCylinder_Dimensions InDimensions)
        -> void
    {
        InHandle.Add<ck::FTag_Probe_ShapeUpdated>();
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::details
{
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

        TSet<FCk_Handle> _BeginOverlaps;
        TArray<FCk_ProbeBeginOverlaps> _OverlappingProbes;

    public:
        CK_PROPERTY_GET(_OverlappingProbes);
        CK_PROPERTY_GET(_BeginOverlaps);

        CK_DEFINE_CONSTRUCTOR(ContactCastCollector, _ProbeHandle, _BodyInterface);
    };

    // ================================================================================================================
    // TProbeShapeFactory specializations
    // ================================================================================================================

    template <>
    struct TProbeShapeFactory<FFragment_ShapeBox_Current>
    {
        static constexpr const TCHAR* ShapeName = TEXT("Box");

        static auto
        CreateShapeSettings(
            FCk_Handle_Probe InHandle)
            -> JPH::BoxShapeSettings
        {
            const auto BoxParams = UCk_Utils_ShapeBox_UE::Get_Dimensions(UCk_Utils_ShapeBox_UE::Cast(InHandle));
            return JPH::BoxShapeSettings{jolt::Conv(BoxParams.Get_HalfExtents()), BoxParams.Get_ConvexRadius()};
        }

        static auto
        CreateUpdateShapeSettings(
            const FFragment_ShapeBox_Current& InShape)
            -> JPH::BoxShapeSettings
        {
            const auto& Dimensions = InShape.Get_Dimensions();
            return JPH::BoxShapeSettings{jolt::Conv(Dimensions.Get_HalfExtents()), Dimensions.Get_ConvexRadius()};
        }

        static auto
        BindDimensionsChanged(
            FCk_Handle_Probe& InHandle)
            -> decltype(auto)
        {
            return UUtils_Signal_OnShapeBoxDimensionsChanged::Bind<
                &ck_probe::OnBoxDimensionsChanged,
                ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
                ECk_Signal_PostFireBehavior::DoNothing>(InHandle);
        }
    };

    template <>
    struct TProbeShapeFactory<FFragment_ShapeSphere_Current>
    {
        static constexpr const TCHAR* ShapeName = TEXT("Sphere");

        static auto
        CreateShapeSettings(
            FCk_Handle_Probe InHandle)
            -> JPH::SphereShapeSettings
        {
            const auto Params = UCk_Utils_ShapeSphere_UE::Get_Dimensions(UCk_Utils_ShapeSphere_UE::Cast(InHandle));
            return JPH::SphereShapeSettings{Params.Get_Radius()};
        }

        static auto
        CreateUpdateShapeSettings(
            const FFragment_ShapeSphere_Current& InShape)
            -> JPH::SphereShapeSettings
        {
            const auto& Dimensions = InShape.Get_Dimensions();
            return JPH::SphereShapeSettings{Dimensions.Get_Radius()};
        }

        static auto
        BindDimensionsChanged(
            FCk_Handle_Probe& InHandle)
            -> decltype(auto)
        {
            return UUtils_Signal_OnShapeSphereDimensionsChanged::Bind<
                &ck_probe::OnSphereDimensionsChanged,
                ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
                ECk_Signal_PostFireBehavior::DoNothing>(InHandle);
        }
    };

    template <>
    struct TProbeShapeFactory<FFragment_ShapeCapsule_Current>
    {
        static constexpr const TCHAR* ShapeName = TEXT("Capsule");

        static auto
        CreateShapeSettings(
            FCk_Handle_Probe InHandle)
            -> JPH::CapsuleShapeSettings
        {
            const auto Params = UCk_Utils_ShapeCapsule_UE::Get_Dimensions(UCk_Utils_ShapeCapsule_UE::Cast(InHandle));
            return JPH::CapsuleShapeSettings{Params.Get_HalfHeight(), Params.Get_Radius()};
        }

        static auto
        CreateUpdateShapeSettings(
            const FFragment_ShapeCapsule_Current& InShape)
            -> JPH::CapsuleShapeSettings
        {
            const auto& Dimensions = InShape.Get_Dimensions();
            return JPH::CapsuleShapeSettings{Dimensions.Get_HalfHeight(), Dimensions.Get_Radius()};
        }

        static auto
        BindDimensionsChanged(
            FCk_Handle_Probe& InHandle)
            -> decltype(auto)
        {
            return UUtils_Signal_OnShapeCapsuleDimensionsChanged::Bind<
                &ck_probe::OnCapsuleDimensionsChanged,
                ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
                ECk_Signal_PostFireBehavior::DoNothing>(InHandle);
        }
    };

    template <>
    struct TProbeShapeFactory<FFragment_ShapeCylinder_Current>
    {
        static constexpr const TCHAR* ShapeName = TEXT("Cylinder");

        static auto
        CreateShapeSettings(
            FCk_Handle_Probe InHandle)
            -> JPH::CylinderShapeSettings
        {
            const auto Params = UCk_Utils_ShapeCylinder_UE::Get_Dimensions(UCk_Utils_ShapeCylinder_UE::Cast(InHandle));
            return JPH::CylinderShapeSettings{Params.Get_HalfHeight(), Params.Get_Radius(), Params.Get_ConvexRadius()};
        }

        static auto
        CreateUpdateShapeSettings(
            const FFragment_ShapeCylinder_Current& InShape)
            -> JPH::CylinderShapeSettings
        {
            const auto& Dimensions = InShape.Get_Dimensions();
            return JPH::CylinderShapeSettings{Dimensions.Get_HalfHeight(), Dimensions.Get_Radius(), Dimensions.Get_ConvexRadius()};
        }

        static auto
        BindDimensionsChanged(
            FCk_Handle_Probe& InHandle)
            -> decltype(auto)
        {
            return UUtils_Signal_OnShapeCylinderDimensionsChanged::Bind<
                &ck_probe::OnCylinderDimensionsChanged,
                ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
                ECk_Signal_PostFireBehavior::DoNothing>(InHandle);
        }
    };

    // ================================================================================================================
    // TProcessor_ProbeSetup
    // ================================================================================================================

    template <typename T_ShapeFragment>
    TProcessor_ProbeSetup<T_ShapeFragment>::
    TProcessor_ProbeSetup(
        const RegistryType& InRegistry,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : Super(InRegistry)
        , _PhysicsSystem(InPhysicsSystem) {}

    template <typename T_ShapeFragment>
    auto
        TProcessor_ProbeSetup<T_ShapeFragment>::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const T_ShapeFragment& InShape,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent,
            const FFragment_Transform& InTransform) const
        -> void
    {
        using Factory = TProbeShapeFactory<T_ShapeFragment>;

        InHandle.template Remove<MarkedDirtyBy>();

        using namespace JPH;
        const auto& EntityPosition = InTransform.Get_Transform().GetLocation();
        const auto& EntityRotation = InTransform.Get_Transform().GetRotation();

        const auto Settings = Factory::CreateShapeSettings(InHandle);
        Settings.SetEmbedded();

        auto ShapeResult = Settings.Create();

        if (NOT ShapeResult.IsValid())
        {
            CK_TRIGGER_ENSURE(TEXT("Failed to create {} shape for probe setup on Entity [{}].\n"
                "Jolt Error: [{}]"),
                Factory::ShapeName, InHandle,
                FString{ShapeResult.GetError().c_str()});
            return;
        }

        auto Shape = ShapeResult.Get();

        InHandle.template Add<Ref<JPH::Shape>>(Shape);

        auto ShapeSettings = BodyCreationSettings{
            Shape,
            jolt::Conv(EntityPosition),
            jolt::Conv(EntityRotation),
            EMotionType::Kinematic,
            ObjectLayer{1}
        };
        ShapeSettings.mIsSensor = true;
        ShapeSettings.mCollideKinematicVsNonDynamic = true;
        ShapeSettings.mGravityFactor = 0.0f;

        switch (InParams.Get_MotionType())
        {
            case ECk_MotionType::Static:
            {
                ShapeSettings.mMotionType = EMotionType::Static;
                InHandle.template Add<FTag_Probe_MotionType_Static>();
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
            case ECk_MotionQuality::Discrete: ShapeSettings.mMotionQuality = EMotionQuality::Discrete;
                break;
            case ECk_MotionQuality::LinearCast: ShapeSettings.mMotionQuality = EMotionQuality::LinearCast;
                break;
        }

        if (InParams.Get_PersistContacts() == ECk_Probe_PersistContacts::Enabled)
        {
            InHandle.template Add<FTag_Probe_PersistContacts>();
        }

        const auto& PhysicsSystem = _PhysicsSystem.Pin();

        if (NOT PhysicsSystem)
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        const auto Body = BodyInterface.CreateBody(ShapeSettings);

        if (ck::Is_NOT_Valid(Body, ck::IsValid_Policy_NullptrOnly{}))
        {
            CK_TRIGGER_ENSURE(TEXT("Failed to create Jolt body for {} probe setup on Entity [{}]. Max body count may have been reached"),
                Factory::ShapeName, InHandle);
            return;
        }

        Body->SetUserData(static_cast<uint64>(InHandle.Get_Entity().Get_ID()));
        Body->SetCollideKinematicVsNonDynamic(true);

        InCurrent._BodyId = Body->GetID();

        InCurrent._ShapeDimensionsChangedConnection = Factory::BindDimensionsChanged(InHandle);

        if (InHandle.template Has<FTag_Probe_LinearCast>())
        { return; }

        // Deactivate the body for LinearCast because we use ShapeCasts for LinearCast, since Jolt does
        // NOT support LinearCast for sensors.
        BodyInterface.AddBody(Body->GetID(), EActivation::Activate);
    }

    // ================================================================================================================
    // TProcessor_ProbeUpdateShape
    // ================================================================================================================

    template <typename T_ShapeFragment>
    TProcessor_ProbeUpdateShape<T_ShapeFragment>::
    TProcessor_ProbeUpdateShape(
        const RegistryType& InRegistry,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : Super(InRegistry)
        , _PhysicsSystem(InPhysicsSystem) {}

    template <typename T_ShapeFragment>
    auto
        TProcessor_ProbeUpdateShape<T_ShapeFragment>::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const T_ShapeFragment& InShape,
            FFragment_Probe_Current& InCurrent) const
        -> void
    {
        using Factory = TProbeShapeFactory<T_ShapeFragment>;
        using namespace JPH;

        InHandle.template Remove<MarkedDirtyBy>();
        const auto& PhysicsSystem = _PhysicsSystem.Pin();

        if (ck::Is_NOT_Valid(PhysicsSystem))
        { return; }

        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        const auto Settings = Factory::CreateUpdateShapeSettings(InShape);
        Settings.SetEmbedded();

        auto ShapeResult = Settings.Create();

        if (NOT ShapeResult.IsValid())
        {
            CK_TRIGGER_ENSURE(TEXT("Failed to create {} shape for probe shape update on Entity [{}].\n"
                "Jolt Error: [{}]"),
                Factory::ShapeName, InHandle,
                FString{ShapeResult.GetError().c_str()});
            return;
        }

        auto NewShape = ShapeResult.Get();

        InHandle.template Replace<Ref<JPH::Shape>>(NewShape);

        BodyInterface.SetShape(
            InCurrent.Get_BodyId(),
            NewShape,
            false,
            EActivation::Activate);
    }

    // ================================================================================================================
    // Explicit template instantiations
    // ================================================================================================================

    template class TProcessor_ProbeSetup<FFragment_ShapeBox_Current>;
    template class TProcessor_ProbeSetup<FFragment_ShapeSphere_Current>;
    template class TProcessor_ProbeSetup<FFragment_ShapeCapsule_Current>;
    template class TProcessor_ProbeSetup<FFragment_ShapeCylinder_Current>;

    template class TProcessor_ProbeUpdateShape<FFragment_ShapeBox_Current>;
    template class TProcessor_ProbeUpdateShape<FFragment_ShapeSphere_Current>;
    template class TProcessor_ProbeUpdateShape<FFragment_ShapeCapsule_Current>;
    template class TProcessor_ProbeUpdateShape<FFragment_ShapeCylinder_Current>;
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

    auto
        FProcessor_Probe_Setup::
        Pump()
        -> void
    {
        _Processor_BoxProbe.Pump();
        _Processor_SphereProbe.Pump();
        _Processor_CapsuleProbe.Pump();
        _Processor_CylinderProbe.Pump();
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

        const auto Rot = jolt::Conv(EntityRotation);

        const auto PhysicsSystem = _PhysicsSystem.Pin();
        CK_ENSURE_IF_NOT(ck::IsValid(PhysicsSystem),
            TEXT("PhysicsSystem is no longer valid during Probe UpdateTransform"))
        { return; }
        auto& BodyInterface = PhysicsSystem->GetBodyInterface();

        BodyInterface.SetPositionAndRotation(InCurrent.Get_BodyId(), jolt::Conv(EntityPosition), Rot, JPH::EActivation::Activate);
    }

    // --------------------------------------------------------------------------------------------------------------------

    // NOTE: Signal broadcasts (Request_BeginOverlap, Request_EndOverlap, etc.) are NOT thread-safe.
    // This processor MUST remain sequential. If parallelized, signal broadcasting must be
    // deferred to the flush phase via deferred commands.
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

        }

        for (auto Overlap : InCurrent.Get_CurrentOverlaps())
        {
            auto OtherEntity = Overlap.Get_OtherEntity();

            if (BeginOverlaps.Contains(OtherEntity))
            { continue; }

            auto MaybeOtherProbe = UCk_Utils_Probe_UE::Cast(OtherEntity);

            if (ck::Is_NOT_Valid(MaybeOtherProbe))
            { continue; }

            UCk_Utils_Probe_UE::Request_EndOverlap(InHandle, FCk_Request_Probe_EndOverlap{OtherEntity});
            UCk_Utils_Probe_UE::Request_EndOverlap(MaybeOtherProbe, FCk_Request_Probe_EndOverlap{InHandle});
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

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

    namespace
    {
        auto
            DoProbeDebugDraw(
                FCk_Handle_Probe InHandle,
                const FFragment_Probe_DebugInfo& InDebugInfo,
                const FFragment_Transform& InTransform)
            -> void
        {
            using namespace JPH;

            const auto& EntityPosition = InTransform.Get_Transform().GetLocation();
            const auto& EntityRotation = InTransform.Get_Transform().GetRotation();

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
            const auto& Bounds = Shape->GetWorldSpaceBounds(Mat4, Vec3{1.f, 1.f, 1.f});

            Shape->GetTrianglesStart(IoContext, Bounds, jolt::Conv(EntityPosition), jolt::Conv(EntityRotation),
                JPH::Vec3{1.f, 1.f, 1.f});

            const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

            Float3 Vertices[Shape::cGetTrianglesMinTrianglesRequested * 3];

            for (auto NumTris = Shape->GetTrianglesNext(IoContext, Shape->cGetTrianglesMinTrianglesRequested,
                     Vertices);
                 NumTris != 0;)
            {
                for (auto Tri = 0; Tri < NumTris; ++Tri)
                {
                    const auto Index = Tri * 3;
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
    }

    auto
        FProcessor_Probe_DebugDraw::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_DebugInfo& InDebugInfo,
            const FFragment_Transform& InTransform)
            -> void
    {
        DoProbeDebugDraw(InHandle, InDebugInfo, InTransform);
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
        DoProbeDebugDraw(InHandle, InDebugInfo, InTransform);
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_Probe_HandleRequests::
        FProcessor_Probe_HandleRequests(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem) {
    }

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
        InHandle.CopyAndRemove(InRequestsComp,
        [&](FFragment_Probe_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests.Get_Requests(), Visitor(
            [&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InCurrent, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
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

        CK_LOG_ERROR_IF_NOT(ck::spatialquery, NumRemovedItems > 0,
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
                    auto& BodyInterface = PhysicsSystem->GetBodyInterface();

                    BodyInterface.AddBody(InCurrent.Get_BodyId(), JPH::EActivation::Activate);

                    const auto& EntityTransform = InHandle.Get<ck::FFragment_Transform>().Get_Transform();
                    const auto& EntityPosition = EntityTransform.GetLocation();
                    const auto& EntityRotation = EntityTransform.GetRotation();

                    const auto EntityRotationQuat = FQuat{EntityRotation};
                    const auto Rot = jolt::Conv(EntityRotationQuat);

                    BodyInterface.SetPositionAndRotation(InCurrent.Get_BodyId(), jolt::Conv(EntityPosition), Rot, JPH::EActivation::Activate);
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

    FProcessor_Probe_EndPlay::
    FProcessor_Probe_EndPlay(
        const RegistryType& InRegistry,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem) {}

    auto
        FProcessor_Probe_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent) const
        -> void
    {
        if (InCurrent._ShapeDimensionsChangedConnection)
        {
            InCurrent._ShapeDimensionsChangedConnection.release();
        }

        const auto& DoManuallyTriggerAllEndOverlaps = [&]() -> void
        {
            for (const auto& OverlapInfo : InCurrent.Get_CurrentOverlaps())
            {
                const auto& OtherEntity = OverlapInfo.Get_OtherEntity();

                UUtils_Signal_OnProbeEndOverlap::Broadcast(InHandle,
                    MakePayload(InHandle, FCk_Probe_Payload_OnEndOverlap{OtherEntity}));

                if (auto OtherEntityAsProbe = UCk_Utils_Probe_UE::Cast(OtherEntity);
                    ck::IsValid(OtherEntityAsProbe) && UCk_Utils_Probe_UE::Get_IsOverlappingWith(OtherEntityAsProbe, InHandle))
                {
                    UCk_Utils_Probe_UE::Request_EndOverlap(OtherEntityAsProbe, FCk_Request_Probe_EndOverlap{InHandle});
                }
            }
        };

        const auto& PhysicsSystem = _PhysicsSystem.Pin();
        auto& BodyInterface = PhysicsSystem->GetBodyInterface();
        const auto& BodyId = InCurrent.Get_BodyId();

        if (UCk_Utils_Probe_UE::Get_IsEnabledDisabled(InHandle) == ECk_EnableDisable::Enable)
        {
            DoManuallyTriggerAllEndOverlaps();

            if (NOT InHandle.Has<FTag_Probe_LinearCast>())
            {
                if (NOT BodyInterface.IsAdded(BodyId))
                {
                    ck::spatialquery::Log(TEXT("Teardown running on Probe [{}] that NEVER had its Jolt body added to the physics system. "
                        "The Probe may be getting destroyed in the same frame. If not, then Did its Setup run correctly?"),
                        InHandle);

                    return;
                }

                BodyInterface.RemoveBody(BodyId);
            }
        }

        if (NOT InHandle.Has<FTag_Probe_LinearCast>())
        {
            if (NOT BodyInterface.IsAdded(BodyId))
            { return; }

            BodyInterface.DestroyBody(BodyId);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_Probe_UpdateShape::
    FProcessor_Probe_UpdateShape(
        const RegistryType& InRegistry,
        const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : _Processor_BoxProbe(InRegistry, InPhysicsSystem)
        , _Processor_SphereProbe(InRegistry, InPhysicsSystem)
        , _Processor_CapsuleProbe(InRegistry, InPhysicsSystem)
        , _Processor_CylinderProbe(InRegistry, InPhysicsSystem) {
    }

    auto
        FProcessor_Probe_UpdateShape::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        _Processor_BoxProbe.Tick(InDeltaT);
        _Processor_SphereProbe.Tick(InDeltaT);
        _Processor_CapsuleProbe.Tick(InDeltaT);
        _Processor_CylinderProbe.Tick(InDeltaT);
    }

    auto
        FProcessor_Probe_UpdateShape::
        Pump()
        -> void
    {
        _Processor_BoxProbe.Pump();
        _Processor_SphereProbe.Pump();
        _Processor_CapsuleProbe.Pump();
        _Processor_CylinderProbe.Pump();
    }
}

// --------------------------------------------------------------------------------------------------------------------
