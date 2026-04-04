#include "CkProbeTrace_Utils.h"

#include "CkProbe_Utils.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"
#include "CkSpatialQuery/CkSpatialQuery_Utils.h"
#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"
#include "CkSpatialQuery/Settings/CkSpatialQuery_Settings.h"
#include "CkSpatialQuery/Subsystem/CkSpatialQuery_Subsystem.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include <Kismet/KismetMathLibrary.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::details
{
    class CastRayCollector : public JPH::CastRayCollector
    {
    public:
        CK_GENERATED_BODY(CastRayCollector);

    public:
        auto
            AddHit(
                const ResultType& InResult)
            -> void override
        {
            const auto Entity = static_cast<FCk_Entity::IdType>(jolt::Get_ProbeBodyUserData(_BodyInterface, InResult.mBodyID));

            if (_AnyHandle.Get_Entity().Get_ID() == Entity)
            { return; }

            const auto OtherProbe = UCk_Utils_Probe_UE::Cast(_AnyHandle.Get_ValidHandle(Entity));

            _Hits.Emplace(std::make_pair(OtherProbe, InResult.mFraction));
        }

    private:
        FCk_Handle _AnyHandle;
        const JPH::BodyInterface& _BodyInterface;

        TArray<std::pair<FCk_Handle_Probe, float>> _Hits;

    public:
        CK_PROPERTY_GET(_Hits);

        CK_DEFINE_CONSTRUCTOR(CastRayCollector, _AnyHandle, _BodyInterface);
    };

    // ----------------------------------------------------------------------------------------------------------------

    class CastShapeCollector : public JPH::CastShapeCollector
    {
    public:
        CK_GENERATED_BODY(CastShapeCollector);

    public:
        auto
            AddHit(
                const JPH::ShapeCastResult& InResult)
            -> void override
        {
            const auto Entity = static_cast<FCk_Entity::IdType>(jolt::Get_ProbeBodyUserData(_BodyInterface, InResult.mBodyID2));

            if (_AnyHandle.Get_Entity().Get_ID() == Entity)
            { return; }

            const auto OtherProbe = UCk_Utils_Probe_UE::Cast(_AnyHandle.Get_ValidHandle(Entity));
            _Hits.Emplace(std::make_pair(OtherProbe, InResult.mFraction));
        }

    private:
        FCk_Handle _AnyHandle;
        const JPH::BodyInterface& _BodyInterface;
        TArray<std::pair<FCk_Handle_Probe, float>> _Hits;

    public:
        CK_PROPERTY_GET(_Hits);

        CK_DEFINE_CONSTRUCTOR(CastShapeCollector, _AnyHandle, _BodyInterface);
    };
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ProbeTrace_UE, FCk_Handle_ProbeTrace, ck::FTag_ProbeTrace)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProbeTrace_UE::
    Request_MultiLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings)
    -> TArray<FCk_Probe_RayCast_Result>
{
    const auto Subsystem = UCk_Utils_EcsWorld_Subsystem_UE::Get_WorldSubsystem<UCk_SpatialQuery_Subsystem>(InAnyHandle);
    const auto& PhysicsSystem = Subsystem->Get_PhysicsSystem().Pin();

    CK_ENSURE_IF_NOT(ck::IsValid(PhysicsSystem),
        TEXT("PhysicsSystem is NOT valid. Unable to start trace using Handle [{}]"), InAnyHandle)
    { return {}; }

    constexpr auto FireOverlaps = true;
    constexpr auto TryDebugDraw = true;
    return Request_MultiLineTrace(InAnyHandle, InSettings, FireOverlaps, TryDebugDraw, *PhysicsSystem);
}

auto
    UCk_Utils_ProbeTrace_UE::
    Request_SingleLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings)
    -> FCk_Probe_RayCast_Result
{
    const auto Subsystem = UCk_Utils_EcsWorld_Subsystem_UE::Get_WorldSubsystem<UCk_SpatialQuery_Subsystem>(InAnyHandle);
    const auto& PhysicsSystem = Subsystem->Get_PhysicsSystem().Pin();

    CK_ENSURE_IF_NOT(ck::IsValid(PhysicsSystem),
        TEXT("PhysicsSystem is NOT valid. Unable to start trace using Handle [{}]"), InAnyHandle)
    { return {}; }

    constexpr auto FireOverlaps = true;
    constexpr auto TryDrawDebug = true;

    const auto& Result = Request_SingleLineTrace(InAnyHandle, InSettings, FireOverlaps, TryDrawDebug, *PhysicsSystem);

    if (ck::Is_NOT_Valid(Result))
    { return {}; }

    Request_DrawLineTrace(InAnyHandle, InSettings, *Result);
    return *Result;
}

auto
    UCk_Utils_ProbeTrace_UE::
    Create_LineTrace_Persistent(
        const FCk_Probe_RayCastPersistent_Settings& InSettings)
    -> FCk_Handle_ProbeTrace
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSettings.Get_StartPos()),
        TEXT("Cannot create Persistent Probe Line Trace because the Start Position Entity is INVALID!"))
    { return {}; }

    auto ProbeTrace = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InSettings.Get_StartPos());
    ProbeTrace.Add<ck::FFragment_ProbeTrace_RayCast>(InSettings);
    ProbeTrace.Add<ck::FTag_ProbeTrace>();

    UCk_Utils_Handle_UE::Set_DebugName(ProbeTrace, *ck::Format_UE(TEXT("ProbeTrace_Line_[{}]"), InSettings.Get_StartPos()));

    return Cast(ProbeTrace);
}

auto
    UCk_Utils_ProbeTrace_UE::
    Create_ShapeTrace_Persistent(
        const FCk_Probe_ShapeCastPersistent_Settings& InSettings)
    -> FCk_Handle_ProbeTrace
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSettings.Get_StartPos()),
        TEXT("Cannot create Persistent Probe Shape Trace because the Start Position Entity is INVALID!"))
    { return {}; }

    auto ProbeTrace = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InSettings.Get_StartPos());
    ProbeTrace.Add<ck::FFragment_ProbeTrace_ShapeCast>(InSettings);
    ProbeTrace.Add<ck::FTag_ProbeTrace>();

    UCk_Utils_Handle_UE::Set_DebugName(ProbeTrace, *ck::Format_UE(TEXT("ProbeTrace_{}_[{}]"), InSettings.Get_Shape().Get_ShapeType(), InSettings.Get_StartPos()));

    return Cast(ProbeTrace);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProbeTrace_UE::
    Request_MultiShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings)
    -> TArray<FCk_ShapeCast_Result>
{
    const auto Subsystem = UCk_Utils_EcsWorld_Subsystem_UE::Get_WorldSubsystem<UCk_SpatialQuery_Subsystem>(InAnyHandle);
    const auto& PhysicsSystem = Subsystem->Get_PhysicsSystem().Pin();

    CK_ENSURE_IF_NOT(ck::IsValid(PhysicsSystem),
        TEXT("PhysicsSystem is NOT valid. Unable to start shape trace using Handle [{}]"), InAnyHandle)
    { return {}; }

    constexpr auto FireOverlaps = true;
    constexpr auto TryDebugDraw = true;
    return Request_MultiShapeTrace(InAnyHandle, InSettings, FireOverlaps, TryDebugDraw, *PhysicsSystem);
}

auto
    UCk_Utils_ProbeTrace_UE::
    Request_SingleShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings)
    -> FCk_ShapeCast_Result
{
    const auto Subsystem = UCk_Utils_EcsWorld_Subsystem_UE::Get_WorldSubsystem<UCk_SpatialQuery_Subsystem>(InAnyHandle);
    const auto& PhysicsSystem = Subsystem->Get_PhysicsSystem().Pin();

    CK_ENSURE_IF_NOT(ck::IsValid(PhysicsSystem),
        TEXT("PhysicsSystem is NOT valid. Unable to start shape trace using Handle [{}]"), InAnyHandle)
    { return {}; }

    constexpr auto FireOverlaps = true;
    constexpr auto TryDrawDebug = true;

    const auto& Result = Request_SingleShapeTrace(InAnyHandle, InSettings, FireOverlaps, TryDrawDebug, *PhysicsSystem);

    if (ck::Is_NOT_Valid(Result))
    { return {}; }

    Request_DrawShapeTrace(InAnyHandle, InSettings, *Result);
    return *Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProbeTrace_UE::
    BindTo_OnBeginOverlap(
        FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnBeginOverlap& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_ProbeTrace
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeTraceBeginOverlap, InProbeTraceEntity, InDelegate, InBindingPolicy,
        InPostFireBehavior);
    return InProbeTraceEntity;
}

auto
    UCk_Utils_ProbeTrace_UE::
    UnbindFrom_OnBeginOverlap(
        FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnBeginOverlap& InDelegate)
    -> FCk_Handle_ProbeTrace
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeTraceBeginOverlap, InProbeTraceEntity, InDelegate);
    return InProbeTraceEntity;
}

auto
    UCk_Utils_ProbeTrace_UE::
    BindTo_OnOverlapUpdated(
        FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnOverlapUpdated& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_ProbeTrace
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeTraceOverlapUpdated, InProbeTraceEntity, InDelegate, InBindingPolicy,
        InPostFireBehavior);
    return InProbeTraceEntity;
}

auto
    UCk_Utils_ProbeTrace_UE::
    UnbindFrom_OnOverlapUpdated(
        FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnOverlapUpdated& InDelegate)
    -> FCk_Handle_ProbeTrace
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeTraceOverlapUpdated, InProbeTraceEntity, InDelegate);
    return InProbeTraceEntity;
}

auto
    UCk_Utils_ProbeTrace_UE::
    BindTo_OnEndOverlap(
        FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnEndOverlap& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_ProbeTrace
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeTraceEndOverlap, InProbeTraceEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InProbeTraceEntity;
}

auto
    UCk_Utils_ProbeTrace_UE::
    UnbindFrom_OnEndOverlap(
        FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnEndOverlap& InDelegate)
    -> FCk_Handle_ProbeTrace
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeTraceEndOverlap, InProbeTraceEntity, InDelegate);
    return InProbeTraceEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProbeTrace_UE::
    Request_MultiLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings,
        bool InFireOverlaps,
        bool InTryDrawDebug,
        const JPH::PhysicsSystem& InPhysicsSystem)
    -> TArray<FCk_Probe_RayCast_Result>
{
    using namespace ck;

    const auto& BodyInterface = InPhysicsSystem.GetBodyInterface();

    const auto& StartPos = InSettings.Get_StartPos();
    const auto& EndPos = InSettings.Get_EndPos();
    const auto& RayCast = JPH::RRayCast{JPH::RayCast{jolt::Conv(StartPos), jolt::Conv(EndPos - StartPos)}};

    const auto& RayCastSettings = JPH::RayCastSettings
    {
        jolt::Conv(InSettings.Get_BackFaceModeTriangles()),
        jolt::Conv(InSettings.Get_BackFaceModeConvex())
    };

    auto Collector = details::CastRayCollector{InAnyHandle, BodyInterface};
    InPhysicsSystem.GetNarrowPhaseQuery().CastRay(RayCast, RayCastSettings, Collector);

    auto Result = TArray<FCk_Probe_RayCast_Result>{};
    for (const auto& [HitProbe, Fraction] : Collector.Get_Hits())
    {
        const auto HitLocation = StartPos + Fraction * (EndPos - StartPos);

        Result.Emplace(FCk_Probe_RayCast_Result
        {
            HitProbe,
            HitLocation,
            StartPos - HitLocation,
            StartPos,
            EndPos
        });
    }

    if (InSettings.Get_Filter().IsEmpty())
    { return Result; }

    auto FilteredResult = decltype(Result){};

    if (Result.IsEmpty())
    {
        if (InTryDrawDebug)
        { Request_DrawLineTrace(InAnyHandle, InSettings, {}); }
    }

    for (const auto& Hit : Result)
    {
        const auto ProbeName = UCk_Utils_Probe_UE::Get_Name(Hit.Get_Probe());

        if (NOT InSettings.Get_Filter().HasTag(ProbeName))
        { continue; }

        if (InTryDrawDebug)
        { Request_DrawLineTrace(InAnyHandle, InSettings, Hit); }

        FilteredResult.Emplace(Hit);
    }

    if (InFireOverlaps)
    {
        for (const auto& Hit : FilteredResult)
        {
            auto Probe = Hit.Get_Probe();
            UCk_Utils_Probe_UE::Request_BeginOverlap(Probe,
                FCk_Request_Probe_BeginOverlap{InAnyHandle, TArray<FVector>{Hit.Get_HitLocation()}, Hit.Get_NormalDirLen(), nullptr});

            UCk_Utils_Probe_UE::Request_EndOverlap(Probe, FCk_Request_Probe_EndOverlap{InAnyHandle});
        }
    }

    return FilteredResult;
}

auto
    UCk_Utils_ProbeTrace_UE::
    Request_SingleLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings,
        bool InFireOverlaps,
        bool InTryDrawDebug,
        const JPH::PhysicsSystem& InPhysicsSystem)
    -> TOptional<FCk_Probe_RayCast_Result>
{
    const auto& Result = Request_MultiLineTrace(InAnyHandle, InSettings, InFireOverlaps, InTryDrawDebug, InPhysicsSystem);

    if (Result.IsEmpty())
    {
        Request_DrawLineTrace(InAnyHandle, InSettings, {});
        return {};
    }

    Request_DrawLineTrace(InAnyHandle, InSettings, Result[0]);
    return Result[0];
}

auto
    UCk_Utils_ProbeTrace_UE::
    Request_DrawLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings,
        TOptional<FCk_Probe_RayCast_Result> InResult)
    -> void
{
    const auto HasDebugDrawTag = InAnyHandle.Has<ck::FTag_ProbeTrace_DebugDraw>();

    if (NOT HasDebugDrawTag && NOT UCk_Utils_SpatialQuery_Settings::Get_DebugPreviewAllLineTraces())
    { return; }

    const auto IsServer = UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InAnyHandle);
    const auto IsClient = UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(InAnyHandle);

    if (NOT HasDebugDrawTag)
    {
        const auto ShouldDrawServer = IsServer && UCk_Utils_SpatialQuery_Settings::Get_DebugPreviewServerLineTraces();
        const auto ShouldDrawClient = IsClient && UCk_Utils_SpatialQuery_Settings::Get_DebugPreviewClientLineTraces();

        if (NOT (ShouldDrawServer || ShouldDrawClient))
        { return; }
    }

    const auto LineThickness = UCk_Utils_SpatialQuery_Settings::Get_ProbeLineTraceDebugThickness();
    const auto Duration = UCk_Utils_SpatialQuery_Settings::Get_ProbeLineTraceDebugDuration();

    auto HitColor = FLinearColor::Green;
    auto MissColor = FLinearColor::Red;
    auto NoHitColor = FLinearColor::White;
    auto BoxColor = FLinearColor::Yellow;

    if (IsClient)
    {
        HitColor = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f);
        MissColor = FLinearColor(1.0f, 0.0f, 1.0f, 1.0f);
        NoHitColor = FLinearColor::White;
        BoxColor = FLinearColor::Yellow;
    }

    const auto WorldContext = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAnyHandle);

    if (ck::IsValid(InResult))
    {
        UCk_Utils_DebugDraw_UE::DrawDebugLine(WorldContext, InResult->Get_StartPos(),
            InResult->Get_HitLocation(), HitColor, Duration, LineThickness);

        UCk_Utils_DebugDraw_UE::DrawDebugLine(WorldContext, InResult->Get_HitLocation(), InResult->Get_EndPos(),
            MissColor, Duration, LineThickness);

        UCk_Utils_DebugDraw_UE::DrawDebugBox(WorldContext, InResult->Get_HitLocation(), FVector{1.0},
            BoxColor,
            UKismetMathLibrary::FindLookAtRotation(InResult->Get_HitLocation(), InResult->Get_StartPos()), Duration, LineThickness);
    }
    else
    {
        UCk_Utils_DebugDraw_UE::DrawDebugLine(WorldContext, InSettings.Get_StartPos(), InSettings.Get_EndPos(),
            NoHitColor, Duration, LineThickness);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProbeTrace_UE::
    Request_MultiShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings,
        bool InFireOverlaps,
        bool InTryDrawDebug,
        const JPH::PhysicsSystem& InPhysicsSystem)
    -> TArray<FCk_ShapeCast_Result>
{
    using namespace ck;

    JPH::Ref<JPH::Shape> JoltShape;

    switch (const auto Shape = InSettings.Get_Shape();
            Shape.Get_ShapeType())
    {
        case ECk_Shape_Type::Box:
        {
            const auto& Dimensions = Shape.Get_Box();
            const auto Settings = JPH::BoxShapeSettings{jolt::Conv(Dimensions.Get_HalfExtents()), Dimensions.Get_ConvexRadius()};
            Settings.SetEmbedded();

            JoltShape = Settings.Create().Get();
            break;
        }
        case ECk_Shape_Type::Sphere:
        {
            const auto& Dimensions = Shape.Get_Sphere();
            const auto Settings = JPH::SphereShapeSettings{Dimensions.Get_Radius()};
            Settings.SetEmbedded();

            JoltShape = Settings.Create().Get();
            break;
        }
        case ECk_Shape_Type::Capsule:
        {
            const auto& Dimensions = Shape.Get_Capsule();
            const auto Settings = JPH::CapsuleShapeSettings{Dimensions.Get_HalfHeight(), Dimensions.Get_Radius()};
            Settings.SetEmbedded();

            JoltShape = Settings.Create().Get();
            break;
        }
        case ECk_Shape_Type::Cylinder:
        {
            const auto& Dimensions = Shape.Get_Capsule();
            const auto Settings = JPH::CylinderShapeSettings{Dimensions.Get_HalfHeight(), Dimensions.Get_Radius()};
            Settings.SetEmbedded();

            JoltShape = Settings.Create().Get();
            break;
        }
        default:
        {
            CK_INVALID_ENUM(Shape.Get_ShapeType());
            return {};
        }
    }

    if (NOT JoltShape)
    {
        CK_TRIGGER_ENSURE(TEXT("Failed to create shape for trace"));
        return {};
    }

    const auto& BodyInterface = InPhysicsSystem.GetBodyInterface();
    const auto& StartPos = InSettings.Get_StartPos();
    const auto& EndPos = InSettings.Get_EndPos();
    const auto& Orientation = UKismetMathLibrary::FindLookAtRotation(StartPos, EndPos);

    const auto StartTransform = FTransform{Orientation, StartPos};
    const auto Direction = EndPos - StartPos;

    const auto ShapeCast = JPH::RShapeCast{
        JoltShape,
        JPH::Vec3::sReplicate(1.0f),
        jolt::Conv(StartTransform),
        jolt::Conv(Direction)
    };

    auto ShapeCastSettings = JPH::ShapeCastSettings{};
    // TODO: There are more settings that could be exposed/set here
    ShapeCastSettings.mBackFaceModeTriangles = jolt::Conv(InSettings.Get_BackFaceModeTriangles());
    ShapeCastSettings.mBackFaceModeConvex = jolt::Conv(InSettings.Get_BackFaceModeConvex());

    auto Collector = details::CastShapeCollector{InAnyHandle, BodyInterface};
    InPhysicsSystem.GetNarrowPhaseQuery().CastShape(ShapeCast, ShapeCastSettings, JPH::Vec3::sReplicate(0.0f), Collector);

    auto Result = TArray<FCk_ShapeCast_Result>{};
    for (const auto& [Probe, Fraction] : Collector.Get_Hits())
    {
        const auto HitLocation = StartPos + Fraction * Direction;

        Result.Emplace(FCk_ShapeCast_Result
        {
            Probe,
            HitLocation,
            StartPos - HitLocation,
            StartPos,
            EndPos,
            Fraction
        });
    }

    if (InSettings.Get_Filter().IsEmpty())
    {
        if (InTryDrawDebug && Result.IsEmpty())
        { Request_DrawShapeTrace(InAnyHandle, InSettings, {}); }
        return Result;
    }

    auto FilteredResult = decltype(Result){};

    for (const auto& Hit : Result)
    {
        const auto ProbeName = UCk_Utils_Probe_UE::Get_Name(Hit.Get_Probe());

        if (NOT InSettings.Get_Filter().HasTag(ProbeName))
        { continue; }

        if (InTryDrawDebug)
        { Request_DrawShapeTrace(InAnyHandle, InSettings, Hit); }

        FilteredResult.Emplace(Hit);
    }

    if (InTryDrawDebug && FilteredResult.IsEmpty())
    { Request_DrawShapeTrace(InAnyHandle, InSettings, {}); }

    if (InFireOverlaps)
    {
        for (const auto& Hit : FilteredResult)
        {
            auto Probe = Hit.Get_Probe();
            UCk_Utils_Probe_UE::Request_BeginOverlap(Probe,
                FCk_Request_Probe_BeginOverlap{InAnyHandle, TArray<FVector>{Hit.Get_HitLocation()}, Hit.Get_NormalDirLen(), nullptr});

            UCk_Utils_Probe_UE::Request_EndOverlap(Probe, FCk_Request_Probe_EndOverlap{InAnyHandle});
        }
    }

    return FilteredResult;
}

auto
    UCk_Utils_ProbeTrace_UE::
    Request_SingleShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings,
        bool InFireOverlaps,
        bool InTryDrawDebug,
        const JPH::PhysicsSystem& InPhysicsSystem)
    -> TOptional<FCk_ShapeCast_Result>
{
    const auto& Result = Request_MultiShapeTrace(InAnyHandle, InSettings, InFireOverlaps, InTryDrawDebug, InPhysicsSystem);

    if (Result.IsEmpty())
    {
        Request_DrawShapeTrace(InAnyHandle, InSettings, {});
        return {};
    }

    Request_DrawShapeTrace(InAnyHandle, InSettings, Result[0]);
    return Result[0];
}

auto
    UCk_Utils_ProbeTrace_UE::
    Request_DrawShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings,
        TOptional<FCk_ShapeCast_Result> InResult)
    -> void
{
    const auto HasDebugDrawTag = InAnyHandle.Has<ck::FTag_ProbeTrace_DebugDraw>();

    if (NOT HasDebugDrawTag && NOT UCk_Utils_SpatialQuery_Settings::Get_DebugPreviewAllLineTraces())
    { return; }

    constexpr auto LineThickness = 0.5f;

    const auto WorldContext = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAnyHandle);
    const auto& StartPos = InSettings.Get_StartPos();
    const auto& EndPos = InSettings.Get_EndPos();
    const auto& Orientation = UKismetMathLibrary::FindLookAtRotation(StartPos, EndPos);
    const auto& Shape = InSettings.Get_Shape();

    if (ck::IsValid(InResult))
    {
        const auto& HitLocation = InResult->Get_HitLocation();

        DrawShapeAtLocation(WorldContext, InSettings, StartPos, Orientation,
            FLinearColor::Red, 0, LineThickness);
        DrawShapeAtLocation(WorldContext, InSettings, HitLocation, Orientation,
            FLinearColor::Yellow, 0, LineThickness);
        DrawShapeAtLocation(WorldContext, InSettings, EndPos, Orientation,
            FLinearColor::Green, 0, LineThickness);

        DrawShapeConnector(WorldContext, StartPos, HitLocation, Shape,
            FLinearColor::Red, 0, LineThickness);
        DrawShapeConnector(WorldContext, HitLocation, EndPos, Shape,
            FLinearColor::Green, 0, LineThickness);
    }
    else
    {
        DrawShapeAtLocation(WorldContext, InSettings, StartPos, Orientation,
            FLinearColor::Red, 0, LineThickness);
        DrawShapeAtLocation(WorldContext, InSettings, EndPos, Orientation,
            FLinearColor::Green, 0, LineThickness);

        DrawShapeConnector(WorldContext, StartPos, EndPos, Shape,
            FLinearColor::White, 0, LineThickness);
    }
}

auto
    UCk_Utils_ProbeTrace_UE::
    DrawShapeAtLocation(
        UWorld* InWorld,
        const FCk_ShapeCast_Settings& InSettings,
        const FVector& InLocation,
        const FRotator& InRotation,
        const FLinearColor& InColor,
        float InDuration,
        float InThickness)
    -> void
{
    switch (const auto Shape = InSettings.Get_Shape();
            Shape.Get_ShapeType())
    {
        case ECk_Shape_Type::Box:
        {
            const auto& BoxParams = Shape.Get_Box();
            UCk_Utils_DebugDraw_UE::DrawDebugBox(InWorld, InLocation, BoxParams.Get_HalfExtents(),
                InColor, InRotation, InDuration, InThickness);
            break;
        }
        case ECk_Shape_Type::Sphere:
        {
            const auto& SphereParams = Shape.Get_Sphere();
            UCk_Utils_DebugDraw_UE::DrawDebugSphere(InWorld, InLocation, SphereParams.Get_Radius(),
                12, InColor, InDuration, InThickness);
            break;
        }
        case ECk_Shape_Type::Capsule:
        {
            const auto& CapsuleParams = Shape.Get_Capsule();
            UCk_Utils_DebugDraw_UE::DrawDebugCapsule(InWorld, InLocation, CapsuleParams.Get_HalfHeight(),
                CapsuleParams.Get_Radius(), InRotation, InColor, InDuration, InThickness);
            break;
        }
        case ECk_Shape_Type::Cylinder:
        {
            const auto& CylinderParams = Shape.Get_Cylinder();
            const FTransform Transform{InRotation, InLocation};

            const FVector CylinderStart = Transform.TransformPosition(FVector(0, 0, -CylinderParams.Get_HalfHeight()));
            const FVector CylinderEnd = Transform.TransformPosition(FVector(0, 0, CylinderParams.Get_HalfHeight()));

            constexpr int32 NumSegments = 12;
            UCk_Utils_DebugDraw_UE::DrawDebugCylinder(InWorld, CylinderStart, CylinderEnd,
                CylinderParams.Get_Radius(), NumSegments, InColor, InDuration, InThickness);
            break;
        }
        default:
        {
            CK_INVALID_ENUM(Shape.Get_ShapeType());
            break;
        };
    }
}

auto
    UCk_Utils_ProbeTrace_UE::
    DrawShapeConnector(
        UWorld* InWorld,
        const FVector& InStartPos,
        const FVector& InEndPos,
        const FCk_AnyShape& InShape,
        const FLinearColor& InColor,
        float InDuration,
        float InThickness)
    -> void
{
    const FVector Direction = InEndPos - InStartPos;
    const float Distance = Direction.Size();

    if (Distance < KINDA_SMALL_NUMBER)
    { return; }

    const FVector MidPoint = (InStartPos + InEndPos) * 0.5f;
    const FRotator ConnectorOrientation = UKismetMathLibrary::FindLookAtRotation(InStartPos, InEndPos);

    switch (InShape.Get_ShapeType())
    {
        case ECk_Shape_Type::Box:
        {
            const auto& BoxParams = InShape.Get_Box();
            const FVector ConnectorHalfExtents = FVector{Distance * 0.5f, BoxParams.Get_HalfExtents().Y, BoxParams.Get_HalfExtents().Z};

            UCk_Utils_DebugDraw_UE::DrawDebugBox(InWorld, MidPoint, ConnectorHalfExtents,
                InColor, ConnectorOrientation, InDuration, InThickness);
            break;
        }
        case ECk_Shape_Type::Sphere:
        {
            const auto& SphereParams = InShape.Get_Sphere();
            const float Radius = SphereParams.Get_Radius();
            const float HalfHeight = (Distance * 0.5f) + Radius;

            const auto CapsuleOrientation = ConnectorOrientation + FRotator(90, 0, 0);

            UCk_Utils_DebugDraw_UE::DrawDebugCapsule(InWorld, MidPoint, HalfHeight, Radius,
                CapsuleOrientation, InColor, InDuration, InThickness);
            break;
        }
        case ECk_Shape_Type::Capsule:
        case ECk_Shape_Type::Cylinder:
        {
            UCk_Utils_DebugDraw_UE::DrawDebugLine(InWorld, InStartPos, InEndPos,
                InColor, InDuration, InThickness);
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InShape.Get_ShapeType());
            break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
