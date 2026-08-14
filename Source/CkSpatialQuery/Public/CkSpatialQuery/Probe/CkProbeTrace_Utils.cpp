#include "CkProbeTrace_Utils.h"

#include "CkProbe_Utils.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"
#include "CkSpatialQuery/CkSpatialQuery_Utils.h"
#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"
#include "CkSpatialQuery/Settings/CkSpatialQuery_Settings.h"
#include "CkJolt/CollisionLayers/CkJoltCollisionLayerTable.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include <Kismet/KismetMathLibrary.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_probe_trace_utils
{
    struct FProbeTrace_HitClassification
    {
        bool _Keep = false;
        ECk_ProbeTrace_HitKind _Kind = ECk_ProbeTrace_HitKind::Probe;
        FCk_Handle _Entity;
        FCk_Handle_Probe _Probe;
    };

    /// The three-way UserData branch is deliberate and must NOT be collapsed into a single
    /// TryGet_EntityFromBody call: that helper returns an invalid handle both for a body owned by the
    /// tracing entity itself and for a body with no entity at all. A self-hit must be DROPPED, while
    /// UserData 0 is a legitimate anonymous world body. Reading the raw UserData first keeps them apart.
    /// Raw entity id 0 is never resolved — it is the registry's transient root, not a body owner.
    inline auto
        Classify_BodyHit(
            const FCk_Handle& InAnyHandle,
            const JPH::BodyInterface& InBodyInterface,
            JPH::BodyID InBodyId,
            ECk_ProbeTrace_WorldHitPolicy InWorldHitPolicy,
            const FCk_Jolt_QueryFilter& InWorldFilter,
            const ck::jolt::FCk_Jolt_CollisionLayerTable& InLayerTable,
            const TArray<FCk_Handle>& InIgnoredEntities)
        -> FProbeTrace_HitClassification
    {
        auto Classification = FProbeTrace_HitClassification{};

        const auto WorldHitsAreVisible = InWorldHitPolicy != ECk_ProbeTrace_WorldHitPolicy::Ignore;

        const auto& DoPassesWorldChannel = [&]() -> bool
        {
            return InLayerTable.Get_ResponseOfLayerToChannel(InBodyInterface.GetObjectLayer(InBodyId),
                InWorldFilter.Get_Channel()) >= InWorldFilter.Get_MinResponse();
        };

        const auto UserData = ck::jolt::Get_BodyUserData(InBodyInterface, InBodyId);

        if (UserData == 0)
        {
            if (NOT WorldHitsAreVisible || NOT DoPassesWorldChannel())
            { return Classification; }

            Classification._Keep = true;
            Classification._Kind = ECk_ProbeTrace_HitKind::World;

            return Classification;
        }

        if (static_cast<FCk_Entity::IdType>(UserData) == InAnyHandle.Get_Entity().Get_ID())
        { return Classification; }

        const auto Entity = ck::jolt::TryGet_EntityFromBody(InAnyHandle, InBodyInterface, InBodyId);

        if (const auto OtherProbe = UCk_Utils_Probe_UE::Cast(Entity);
            ck::IsValid(OtherProbe))
        {
            if (InIgnoredEntities.Contains(Entity))
            { return Classification; }

            Classification._Keep = true;
            Classification._Kind = ECk_ProbeTrace_HitKind::Probe;
            Classification._Entity = Entity;
            Classification._Probe = OtherProbe;

            return Classification;
        }

        if (NOT WorldHitsAreVisible || NOT DoPassesWorldChannel())
        { return Classification; }

        if (ck::IsValid(Entity) && InIgnoredEntities.Contains(Entity))
        { return Classification; }

        Classification._Keep = true;
        Classification._Kind = ECk_ProbeTrace_HitKind::World;
        Classification._Entity = Entity;

        return Classification;
    }

    // ----------------------------------------------------------------------------------------------------------------

    struct FProbeTrace_RayHit
    {
        JPH::BodyID _BodyId;
        float _Fraction = 0.0f;
        JPH::SubShapeID _SubShapeId2;
        ECk_ProbeTrace_HitKind _Kind = ECk_ProbeTrace_HitKind::Probe;
        FCk_Handle _Entity;
        FCk_Handle_Probe _Probe;
    };

    struct FProbeTrace_ShapeHit
    {
        JPH::BodyID _BodyId;
        float _Fraction = 0.0f;
        JPH::Vec3 _PenetrationAxis = JPH::Vec3::sZero();
        JPH::Vec3 _ContactPointOn2 = JPH::Vec3::sZero();
        ECk_ProbeTrace_HitKind _Kind = ECk_ProbeTrace_HitKind::Probe;
        FCk_Handle _Entity;
        FCk_Handle_Probe _Probe;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /// Jolt only reports a surface normal for shape casts; for a ray the face has to be asked of the body
    /// itself. Lock failure means the body died between the cast and this read — Up is the same fallback
    /// the CkJolt query utils use.
    inline auto
        Get_RaySurfaceNormal(
            const JPH::PhysicsSystem& InPhysicsSystem,
            const JPH::BodyID& InBodyId,
            const JPH::SubShapeID& InSubShapeId,
            const JPH::Vec3& InWorldPosition)
        -> FVector
    {
        const auto Lock = JPH::BodyLockRead{InPhysicsSystem.GetBodyLockInterface(), InBodyId};

        if (NOT Lock.Succeeded())
        { return FVector::UpVector; }

        return ck::jolt::Conv(Lock.GetBody().GetWorldSpaceSurfaceNormal(InSubShapeId, InWorldPosition));
    }
}

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
            const auto Classification = ck_probe_trace_utils::Classify_BodyHit(_AnyHandle, _BodyInterface, InResult.mBodyID,
                _WorldHitPolicy, _WorldFilter, _LayerTable, _IgnoredEntities);

            if (NOT Classification._Keep)
            { return; }

            _Hits.Emplace(ck_probe_trace_utils::FProbeTrace_RayHit
            {
                InResult.mBodyID,
                InResult.mFraction,
                InResult.mSubShapeID2,
                Classification._Kind,
                Classification._Entity,
                Classification._Probe
            });
        }

    private:
        FCk_Handle _AnyHandle;
        const JPH::BodyInterface& _BodyInterface;
        ECk_ProbeTrace_WorldHitPolicy _WorldHitPolicy = ECk_ProbeTrace_WorldHitPolicy::Ignore;
        const FCk_Jolt_QueryFilter& _WorldFilter;
        const jolt::FCk_Jolt_CollisionLayerTable& _LayerTable;
        const TArray<FCk_Handle>& _IgnoredEntities;

        TArray<ck_probe_trace_utils::FProbeTrace_RayHit> _Hits;

    public:
        CK_PROPERTY_GET(_Hits);

        CK_DEFINE_CONSTRUCTOR(CastRayCollector, _AnyHandle, _BodyInterface, _WorldHitPolicy, _WorldFilter,
            _LayerTable, _IgnoredEntities);
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
            const auto Classification = ck_probe_trace_utils::Classify_BodyHit(_AnyHandle, _BodyInterface, InResult.mBodyID2,
                _WorldHitPolicy, _WorldFilter, _LayerTable, _IgnoredEntities);

            if (NOT Classification._Keep)
            { return; }

            _Hits.Emplace(ck_probe_trace_utils::FProbeTrace_ShapeHit
            {
                InResult.mBodyID2,
                InResult.mFraction,
                InResult.mPenetrationAxis,
                InResult.mContactPointOn2,
                Classification._Kind,
                Classification._Entity,
                Classification._Probe
            });
        }

    private:
        FCk_Handle _AnyHandle;
        const JPH::BodyInterface& _BodyInterface;
        ECk_ProbeTrace_WorldHitPolicy _WorldHitPolicy = ECk_ProbeTrace_WorldHitPolicy::Ignore;
        const FCk_Jolt_QueryFilter& _WorldFilter;
        const jolt::FCk_Jolt_CollisionLayerTable& _LayerTable;
        const TArray<FCk_Handle>& _IgnoredEntities;

        TArray<ck_probe_trace_utils::FProbeTrace_ShapeHit> _Hits;

    public:
        CK_PROPERTY_GET(_Hits);

        CK_DEFINE_CONSTRUCTOR(CastShapeCollector, _AnyHandle, _BodyInterface, _WorldHitPolicy, _WorldFilter,
            _LayerTable, _IgnoredEntities);
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
    constexpr auto FireOverlaps = true;
    constexpr auto TryDebugDraw = true;
    return Request_MultiLineTrace(InAnyHandle, InSettings, FireOverlaps, TryDebugDraw,
        FCk_ProbeTrace_Context::Get_ForEntity(InAnyHandle));
}

auto
    UCk_Utils_ProbeTrace_UE::
    Request_SingleLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings)
    -> FCk_Probe_RayCast_Result
{
    constexpr auto FireOverlaps = true;
    constexpr auto TryDrawDebug = true;

    // The internal Single overload already debug-draws its result — no extra draw here.
    const auto& Result = Request_SingleLineTrace(InAnyHandle, InSettings, FireOverlaps, TryDrawDebug,
        FCk_ProbeTrace_Context::Get_ForEntity(InAnyHandle));

    if (ck::Is_NOT_Valid(Result))
    { return {}; }

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
    ProbeTrace.Add<TSet<FCk_Probe_OverlapInfo>>();

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
    ProbeTrace.Add<TSet<FCk_Probe_OverlapInfo>>();

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
    constexpr auto FireOverlaps = true;
    constexpr auto TryDebugDraw = true;
    return Request_MultiShapeTrace(InAnyHandle, InSettings, FireOverlaps, TryDebugDraw,
        FCk_ProbeTrace_Context::Get_ForEntity(InAnyHandle));
}

auto
    UCk_Utils_ProbeTrace_UE::
    Request_SingleShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings)
    -> FCk_ShapeCast_Result
{
    constexpr auto FireOverlaps = true;
    constexpr auto TryDrawDebug = true;

    // The internal Single overload already debug-draws its result — no extra draw here.
    const auto& Result = Request_SingleShapeTrace(InAnyHandle, InSettings, FireOverlaps, TryDrawDebug,
        FCk_ProbeTrace_Context::Get_ForEntity(InAnyHandle));

    if (ck::Is_NOT_Valid(Result))
    { return {}; }

    return *Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ProbeTrace_UE::
    Get_CurrentOverlaps(
        const FCk_Handle_ProbeTrace& InProbeTrace)
    -> TSet<FCk_Probe_OverlapInfo>
{
    return InProbeTrace.Get<TSet<FCk_Probe_OverlapInfo>>();
}

auto
    UCk_Utils_ProbeTrace_UE::
    Get_IsEnabledDisabled(
        const FCk_Handle_ProbeTrace& InProbeTrace)
    -> ECk_EnableDisable
{
    return InProbeTrace.Has<ck::FTag_ProbeTrace_Disabled>()
        ? ECk_EnableDisable::Disable
        : ECk_EnableDisable::Enable;
}

auto
    UCk_Utils_ProbeTrace_UE::
    Request_EnableDisable(
        FCk_Handle_ProbeTrace& InProbeTrace,
        const FCk_Request_Probe_EnableDisable& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_ProbeTrace
{
    switch (InRequest.Get_EnableDisable())
    {
        case ECk_EnableDisable::Enable:
        {
            InProbeTrace.Try_Remove<ck::FTag_ProbeTrace_Disabled>();
            break;
        }
        case ECk_EnableDisable::Disable:
        {
            // The disabled tag short-circuits the processor's stale-overlap loop, so listeners
            // would stay stuck mid-overlap unless we end every pair here first.
            const auto& DoManuallyTriggerAllEndOverlaps = [&]() -> void
            {
                for (const auto& OverlapInfo : Get_CurrentOverlaps(InProbeTrace))
                {
                    const auto& OtherEntity = OverlapInfo.Get_OtherEntity();

                    ck::UUtils_Signal_OnProbeTraceEndOverlap::Broadcast(InProbeTrace,
                        ck::MakePayload(InProbeTrace, FCk_Probe_Payload_OnEndOverlap{OtherEntity}));

                    if (auto OtherEntityAsProbe = UCk_Utils_Probe_UE::Cast(OtherEntity);
                        ck::IsValid(OtherEntityAsProbe))
                    {
                        UCk_Utils_Probe_UE::Request_EndOverlap(OtherEntityAsProbe,
                            FCk_Request_Probe_EndOverlap{InProbeTrace}, {});
                    }
                }
            };

            DoManuallyTriggerAllEndOverlaps();

            // Re-enable must start fresh: the next cast re-populates as BeginOverlaps.
            InProbeTrace.Get<TSet<FCk_Probe_OverlapInfo>>().Empty();

            InProbeTrace.AddOrGet<ck::FTag_ProbeTrace_Disabled>();
            break;
        }
    }

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InProbeTrace, ECk_Request_OperationResult::Succeeded);

    return InProbeTrace;
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

auto
    UCk_Utils_ProbeTrace_UE::
    BindTo_OnProbeTraceWorldHit(
        FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnWorldHit& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_ProbeTrace
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnProbeTraceWorldHit, InProbeTraceEntity, InDelegate, InBindingPolicy,
        InPostFireBehavior);
    return InProbeTraceEntity;
}

auto
    UCk_Utils_ProbeTrace_UE::
    UnbindFrom_OnProbeTraceWorldHit(
        FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnWorldHit& InDelegate)
    -> FCk_Handle_ProbeTrace
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnProbeTraceWorldHit, InProbeTraceEntity, InDelegate);
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
        const FCk_ProbeTrace_Context& InContext)
    -> TArray<FCk_Probe_RayCast_Result>
{
    using namespace ck;

    const auto PinnedPhysicsSystem = InContext._PhysicsSystem.Pin();
    const auto ContextIsValid = ck::IsValid(PinnedPhysicsSystem) && InContext._LayerTable != nullptr;

    CK_ENSURE_IF_NOT(ContextIsValid,
        TEXT("ProbeTrace context is NOT valid. Unable to start trace using Handle [{}]"), InAnyHandle)
    { return {}; }

    const auto& PhysicsSystem = *PinnedPhysicsSystem;

    const auto& BodyInterface = PhysicsSystem.GetBodyInterface();

    const auto& StartPos = InSettings.Get_StartPos();
    const auto& EndPos = InSettings.Get_EndPos();
    const auto& RayCast = JPH::RRayCast{JPH::RayCast{jolt::Conv(StartPos), jolt::Conv(EndPos - StartPos)}};

    const auto& RayCastSettings = JPH::RayCastSettings
    {
        jolt::Conv(InSettings.Get_BackFaceModeTriangles()),
        jolt::Conv(InSettings.Get_BackFaceModeConvex())
    };

    auto Collector = details::CastRayCollector{InAnyHandle, BodyInterface, InSettings.Get_WorldHitPolicy(),
        InSettings.Get_WorldFilter(), *InContext._LayerTable, InSettings.Get_IgnoredEntities()};
    PhysicsSystem.GetNarrowPhaseQuery().CastRay(RayCast, RayCastSettings, Collector);

    // Jolt collectors receive hits in broadphase-traversal order, NOT distance order — sort by
    // fraction so Multi results are nearest-first and Single's Result[0] is the closest hit.
    auto SortedHits = Collector.Get_Hits();
    SortedHits.Sort([](const auto& InA, const auto& InB) { return InA._Fraction < InB._Fraction; });

    const auto FilterIsEmpty = InSettings.Get_Filter().IsEmpty();

    auto KeptHits = decltype(SortedHits){};

    for (const auto& Hit : SortedHits)
    {
        if (Hit._Kind == ECk_ProbeTrace_HitKind::Probe && NOT FilterIsEmpty)
        {
            const auto ProbeName = UCk_Utils_Probe_UE::Get_Name(Hit._Probe);

            // Direction is load-bearing: the probe NAME is matched against the filter, never the
            // reverse — same as Get_CanOverlapWith.
            if (NOT ProbeName.MatchesAny(InSettings.Get_Filter()))
            { continue; }
        }

        KeptHits.Emplace(Hit);

        // Blocking truncates AFTER the blocker: it is the final element, and nothing behind it is
        // returned or overlap-fired.
        if (Hit._Kind == ECk_ProbeTrace_HitKind::World &&
            InSettings.Get_WorldHitPolicy() == ECk_ProbeTrace_WorldHitPolicy::Blocking)
        { break; }
    }

    auto Result = TArray<FCk_Probe_RayCast_Result>{};

    for (const auto& Hit : KeptHits)
    {
        const auto HitLocation = StartPos + Hit._Fraction * (EndPos - StartPos);

        auto HitResult = FCk_Probe_RayCast_Result
        {
            Hit._Probe,
            HitLocation,
            StartPos - HitLocation,
            StartPos,
            EndPos
        };

        HitResult.Set_HitKind(Hit._Kind);
        HitResult.Set_HitEntity(Hit._Entity);
        HitResult.Set_SurfaceNormal(ck_probe_trace_utils::Get_RaySurfaceNormal(PhysicsSystem, Hit._BodyId,
            Hit._SubShapeId2, RayCast.GetPointOnRay(Hit._Fraction)));
        HitResult.Set_Fraction(Hit._Fraction);

        Result.Emplace(MoveTemp(HitResult));
    }

    // An empty filter matches every probe AND returns before the overlap block. That predates the
    // world-hit policy and is deliberately preserved: existing empty-filter callers have never fired
    // overlaps and must not start now.
    if (FilterIsEmpty)
    {
        if (InTryDrawDebug && Result.IsEmpty())
        { Request_DrawLineTrace(InAnyHandle, InSettings, {}); }
        return Result;
    }

    if (InTryDrawDebug)
    {
        for (const auto& Hit : Result)
        { Request_DrawLineTrace(InAnyHandle, InSettings, Hit); }

        // Post-filter, mirroring the shape twin: a trace whose every hit was filtered out is still a miss.
        if (Result.IsEmpty())
        { Request_DrawLineTrace(InAnyHandle, InSettings, {}); }
    }

    if (InFireOverlaps && InSettings.Get_OverlapNotifyPolicy() == ECk_ProbeResponse_Policy::Notify)
    {
        for (const auto& Hit : Result)
        {
            // World hits never ping a probe — there is no probe on the other end.
            if (Hit.Get_HitKind() != ECk_ProbeTrace_HitKind::Probe)
            { continue; }

            auto Probe = Hit.Get_Probe();
            UCk_Utils_Probe_UE::Request_BeginOverlap(Probe,
                FCk_Request_Probe_BeginOverlap{InAnyHandle, TArray<FVector>{Hit.Get_HitLocation()}, Hit.Get_NormalDirLen(), nullptr}, {});

            UCk_Utils_Probe_UE::Request_EndOverlap(Probe, FCk_Request_Probe_EndOverlap{InAnyHandle}, {});
        }
    }

    return Result;
}

auto
    UCk_Utils_ProbeTrace_UE::
    Request_SingleLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings,
        bool InFireOverlaps,
        bool InTryDrawDebug,
        const FCk_ProbeTrace_Context& InContext)
    -> TOptional<FCk_Probe_RayCast_Result>
{
    const auto& Result = Request_MultiLineTrace(InAnyHandle, InSettings, InFireOverlaps, InTryDrawDebug, InContext);

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
        TOptional<FCk_Probe_RayCast_Result> InResult,
        bool InIsDisabled)
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

    if (InIsDisabled)
    {
        HitColor = FLinearColor::Gray;
        MissColor = FLinearColor::Gray;
        NoHitColor = FLinearColor::Gray;
        BoxColor = FLinearColor::Gray;
    }

    const auto WorldContext = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAnyHandle);

    if (ck::IsValid(InResult) && NOT InIsDisabled)
    {
        // World hits reuse the whole hit vocabulary; only the marker changes colour, so a blocker is
        // distinguishable from a probe at a glance without a new settings knob.
        const auto MarkerColor = InResult->Get_HitKind() == ECk_ProbeTrace_HitKind::World
            ? MissColor
            : BoxColor;

        UCk_Utils_DebugDraw_UE::DrawDebugLine(WorldContext, InResult->Get_StartPos(),
            InResult->Get_HitLocation(), HitColor, Duration, LineThickness);

        UCk_Utils_DebugDraw_UE::DrawDebugLine(WorldContext, InResult->Get_HitLocation(), InResult->Get_EndPos(),
            MissColor, Duration, LineThickness);

        UCk_Utils_DebugDraw_UE::DrawDebugBox(WorldContext, InResult->Get_HitLocation(), FVector{1.0},
            MarkerColor,
            UKismetMathLibrary::FindLookAtRotation(InResult->Get_HitLocation(), InResult->Get_StartPos()), Duration, LineThickness);
    }
    else
    {
        UCk_Utils_DebugDraw_UE::DrawDebugLine(WorldContext, InSettings.Get_StartPos(), InSettings.Get_EndPos(),
            NoHitColor, Duration, LineThickness);

        // A bare gray line is easy to miss against scene geometry — mark the endpoint like the
        // hit case marks the hit location.
        if (InIsDisabled)
        {
            UCk_Utils_DebugDraw_UE::DrawDebugBox(WorldContext, InSettings.Get_EndPos(), FVector{1.0},
                BoxColor,
                UKismetMathLibrary::FindLookAtRotation(InSettings.Get_EndPos(), InSettings.Get_StartPos()), Duration, LineThickness);
        }
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
        const FCk_ProbeTrace_Context& InContext)
    -> TArray<FCk_ShapeCast_Result>
{
    using namespace ck;

    const auto PinnedPhysicsSystem = InContext._PhysicsSystem.Pin();
    const auto ContextIsValid = ck::IsValid(PinnedPhysicsSystem) && InContext._LayerTable != nullptr;

    CK_ENSURE_IF_NOT(ContextIsValid,
        TEXT("ProbeTrace context is NOT valid. Unable to start shape trace using Handle [{}]"), InAnyHandle)
    { return {}; }

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
        // Jolt's Capsule/Cylinder are Y-AXIS ALIGNED and this trace runs Z-up — the
        // RotatedTranslatedShape wrapper stands the leaf shape up. See CkSpatialQuery/CLAUDE.md.
        case ECk_Shape_Type::Capsule:
        {
            const auto& Dimensions = Shape.Get_Capsule();
            const auto InnerSettings = JPH::CapsuleShapeSettings{Dimensions.Get_HalfHeight(), Dimensions.Get_Radius()};
            InnerSettings.SetEmbedded();

            const auto Settings = JPH::RotatedTranslatedShapeSettings{
                JPH::Vec3::sZero(), jolt::Get_ShapeAxisCorrection_YToZ(), &InnerSettings};
            Settings.SetEmbedded();

            JoltShape = Settings.Create().Get();
            break;
        }
        case ECk_Shape_Type::Cylinder:
        {
            const auto& Dimensions = Shape.Get_Cylinder();
            const auto InnerSettings = JPH::CylinderShapeSettings{Dimensions.Get_HalfHeight(), Dimensions.Get_Radius(), Dimensions.Get_ConvexRadius()};
            InnerSettings.SetEmbedded();

            const auto Settings = JPH::RotatedTranslatedShapeSettings{
                JPH::Vec3::sZero(), jolt::Get_ShapeAxisCorrection_YToZ(), &InnerSettings};
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

    const auto& PhysicsSystem = *PinnedPhysicsSystem;

    const auto& BodyInterface = PhysicsSystem.GetBodyInterface();
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

    auto Collector = details::CastShapeCollector{InAnyHandle, BodyInterface, InSettings.Get_WorldHitPolicy(),
        InSettings.Get_WorldFilter(), *InContext._LayerTable, InSettings.Get_IgnoredEntities()};
    PhysicsSystem.GetNarrowPhaseQuery().CastShape(ShapeCast, ShapeCastSettings, JPH::Vec3::sReplicate(0.0f), Collector);

    // Same broadphase-order sort as the line-trace path above.
    auto SortedHits = Collector.Get_Hits();
    SortedHits.Sort([](const auto& InA, const auto& InB) { return InA._Fraction < InB._Fraction; });

    const auto FilterIsEmpty = InSettings.Get_Filter().IsEmpty();

    auto KeptHits = decltype(SortedHits){};

    for (const auto& Hit : SortedHits)
    {
        if (Hit._Kind == ECk_ProbeTrace_HitKind::Probe && NOT FilterIsEmpty)
        {
            const auto ProbeName = UCk_Utils_Probe_UE::Get_Name(Hit._Probe);

            if (NOT ProbeName.MatchesAny(InSettings.Get_Filter()))
            { continue; }
        }

        KeptHits.Emplace(Hit);

        if (Hit._Kind == ECk_ProbeTrace_HitKind::World &&
            InSettings.Get_WorldHitPolicy() == ECk_ProbeTrace_WorldHitPolicy::Blocking)
        { break; }
    }

    auto Result = TArray<FCk_ShapeCast_Result>{};

    for (const auto& Hit : KeptHits)
    {
        const auto HitLocation = StartPos + Hit._Fraction * Direction;

        auto HitResult = FCk_ShapeCast_Result
        {
            Hit._Probe,
            HitLocation,
            StartPos - HitLocation,
            StartPos,
            EndPos,
            Hit._Fraction
        };

        HitResult.Set_HitKind(Hit._Kind);
        HitResult.Set_HitEntity(Hit._Entity);
        // NormalizedOr, not Normalized: a zero-length shape cast (EQS's overlap test casts a sphere
        // from a point to itself) can report a degenerate penetration axis, and a raw divide would
        // put a NaN in the result. Up matches the ray path's lock-failure fallback.
        HitResult.Set_SurfaceNormal(jolt::Conv((-Hit._PenetrationAxis).NormalizedOr(JPH::Vec3::sAxisZ())));

        Result.Emplace(MoveTemp(HitResult));
    }

    // Empty-filter early return — see the line-trace twin for why the overlap block is skipped.
    if (FilterIsEmpty)
    {
        if (InTryDrawDebug && Result.IsEmpty())
        { Request_DrawShapeTrace(InAnyHandle, InSettings, {}); }
        return Result;
    }

    if (InTryDrawDebug)
    {
        for (const auto& Hit : Result)
        { Request_DrawShapeTrace(InAnyHandle, InSettings, Hit); }

        if (Result.IsEmpty())
        { Request_DrawShapeTrace(InAnyHandle, InSettings, {}); }
    }

    if (InFireOverlaps && InSettings.Get_OverlapNotifyPolicy() == ECk_ProbeResponse_Policy::Notify)
    {
        for (const auto& Hit : Result)
        {
            if (Hit.Get_HitKind() != ECk_ProbeTrace_HitKind::Probe)
            { continue; }

            auto Probe = Hit.Get_Probe();
            UCk_Utils_Probe_UE::Request_BeginOverlap(Probe,
                FCk_Request_Probe_BeginOverlap{InAnyHandle, TArray<FVector>{Hit.Get_HitLocation()}, Hit.Get_NormalDirLen(), nullptr}, {});

            UCk_Utils_Probe_UE::Request_EndOverlap(Probe, FCk_Request_Probe_EndOverlap{InAnyHandle}, {});
        }
    }

    return Result;
}

auto
    UCk_Utils_ProbeTrace_UE::
    Request_SingleShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings,
        bool InFireOverlaps,
        bool InTryDrawDebug,
        const FCk_ProbeTrace_Context& InContext)
    -> TOptional<FCk_ShapeCast_Result>
{
    const auto& Result = Request_MultiShapeTrace(InAnyHandle, InSettings, InFireOverlaps, InTryDrawDebug, InContext);

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
        TOptional<FCk_ShapeCast_Result> InResult,
        bool InIsDisabled)
    -> void
{
    const auto HasDebugDrawTag = InAnyHandle.Has<ck::FTag_ProbeTrace_DebugDraw>();

    if (NOT HasDebugDrawTag && NOT UCk_Utils_SpatialQuery_Settings::Get_DebugPreviewAllLineTraces())
    { return; }

    // Same settings group as the line-trace twin: honor the server/client preview sub-gates and the
    // shared thickness/duration settings (there are no shape-specific settings).
    if (NOT HasDebugDrawTag)
    {
        const auto ShouldDrawServer = UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InAnyHandle) &&
            UCk_Utils_SpatialQuery_Settings::Get_DebugPreviewServerLineTraces();
        const auto ShouldDrawClient = UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(InAnyHandle) &&
            UCk_Utils_SpatialQuery_Settings::Get_DebugPreviewClientLineTraces();

        if (NOT (ShouldDrawServer || ShouldDrawClient))
        { return; }
    }

    const auto LineThickness = UCk_Utils_SpatialQuery_Settings::Get_ProbeLineTraceDebugThickness();
    const auto Duration = UCk_Utils_SpatialQuery_Settings::Get_ProbeLineTraceDebugDuration();

    const auto WorldContext = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAnyHandle);
    const auto& StartPos = InSettings.Get_StartPos();
    const auto& EndPos = InSettings.Get_EndPos();
    const auto& Orientation = UKismetMathLibrary::FindLookAtRotation(StartPos, EndPos);
    const auto& Shape = InSettings.Get_Shape();

    if (InIsDisabled)
    {
        DrawShapeAtLocation(WorldContext, InSettings, StartPos, Orientation,
            FLinearColor::Gray, Duration, LineThickness);
        DrawShapeAtLocation(WorldContext, InSettings, EndPos, Orientation,
            FLinearColor::Gray, Duration, LineThickness);
        DrawShapeConnector(WorldContext, StartPos, EndPos, Shape,
            FLinearColor::Gray, Duration, LineThickness);
        return;
    }

    if (ck::IsValid(InResult))
    {
        const auto& HitLocation = InResult->Get_HitLocation();

        // Same reasoning as the line-trace twin: only the hit marker's colour distinguishes a world
        // blocker from a probe hit.
        const auto MarkerColor = InResult->Get_HitKind() == ECk_ProbeTrace_HitKind::World
            ? FLinearColor::Red
            : FLinearColor::Yellow;

        DrawShapeAtLocation(WorldContext, InSettings, StartPos, Orientation,
            FLinearColor::Red, Duration, LineThickness);
        DrawShapeAtLocation(WorldContext, InSettings, HitLocation, Orientation,
            MarkerColor, Duration, LineThickness);
        DrawShapeAtLocation(WorldContext, InSettings, EndPos, Orientation,
            FLinearColor::Green, Duration, LineThickness);

        DrawShapeConnector(WorldContext, StartPos, HitLocation, Shape,
            FLinearColor::Red, Duration, LineThickness);
        DrawShapeConnector(WorldContext, HitLocation, EndPos, Shape,
            FLinearColor::Green, Duration, LineThickness);
    }
    else
    {
        DrawShapeAtLocation(WorldContext, InSettings, StartPos, Orientation,
            FLinearColor::Red, Duration, LineThickness);
        DrawShapeAtLocation(WorldContext, InSettings, EndPos, Orientation,
            FLinearColor::Green, Duration, LineThickness);

        DrawShapeConnector(WorldContext, StartPos, EndPos, Shape,
            FLinearColor::White, Duration, LineThickness);
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
        }
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
