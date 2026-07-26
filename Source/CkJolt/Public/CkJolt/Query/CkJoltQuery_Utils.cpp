#include "CkJoltQuery_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkJolt/CkJoltShapeFactory.h"
#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Subsystem/CkJolt_Subsystem.h"

#include <Engine/World.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_query_utils
{
    struct FQueryContext
    {
        UWorld* _World = nullptr;
        UCk_Jolt_Subsystem* _JoltSubsystem = nullptr;
        TSharedPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    static auto Get_QueryContext(const UObject* InWorldContextObject) -> FQueryContext
    {
        auto Context = FQueryContext{};

        if (ck::Is_NOT_Valid(InWorldContextObject, ck::IsValid_Policy_NullptrOnly{}))
        { return Context; }

        Context._World = InWorldContextObject->GetWorld();
        if (ck::Is_NOT_Valid(Context._World))
        { return Context; }

        Context._JoltSubsystem = Context._World->GetSubsystem<UCk_Jolt_Subsystem>();
        if (ck::Is_NOT_Valid(Context._JoltSubsystem, ck::IsValid_Policy_NullptrOnly{}))
        { return Context; }

        Context._PhysicsSystem = Context._JoltSubsystem->Get_PhysicsSystem().Pin();
        return Context;
    }

    // Registry-liveness check FIRST — no ensure on dead ids (mirrors the probe contact translation).
    static auto TryResolve_Entity(const FQueryContext& InContext, uint64 InUserData) -> FCk_Handle
    {
        const auto* EcsWorldSubsystem = InContext._World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
        if (ck::Is_NOT_Valid(EcsWorldSubsystem, ck::IsValid_Policy_NullptrOnly{}))
        { return {}; }

        // UserData 0 = NO entity. Raw entity id 0 is always the registry's live transient root — resolving
        // it would return the transient root as a spurious result.
        if (InUserData == 0)
        { return {}; }

        const auto TransientEntity = EcsWorldSubsystem->Get_TransientEntity();
        const auto RegView = TransientEntity.Get_RegistryView();

        const auto Entity = FCk_Entity{static_cast<FCk_Entity::IdType>(InUserData)};
        if (NOT RegView.IsValid(Entity))
        { return {}; }

        return TransientEntity.Get_ValidHandle(Entity.Get_ID());
    }

    static auto Fill_HitAttribution(
        const FQueryContext& InContext,
        const JPH::BodyID& InBodyId,
        FCk_Jolt_HitResult& InOutResult)
        -> void
    {
        const auto& BodyInterface = InContext._PhysicsSystem->GetBodyInterface();

        InOutResult.Set_Entity(TryResolve_Entity(InContext, BodyInterface.GetUserData(InBodyId)));
    }

    static auto Get_SurfaceNormal(
        const FQueryContext& InContext,
        const JPH::BodyID& InBodyId,
        const JPH::SubShapeID& InSubShapeId,
        const JPH::Vec3& InWorldPosition)
        -> FVector
    {
        const auto Lock = JPH::BodyLockRead{InContext._PhysicsSystem->GetBodyLockInterface(), InBodyId};

        if (NOT Lock.Succeeded())
        { return FVector::UpVector; }

        return ck::jolt::Conv(Lock.GetBody().GetWorldSpaceSurfaceNormal(InSubShapeId, InWorldPosition));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_JoltQuery_UE::
    Get_RayCast(
        const UObject* InWorldContextObject,
        FVector InStart,
        FVector InEnd,
        FCk_Jolt_QueryFilter InFilter)
    -> FCk_Jolt_HitResult
{
    using namespace ck_jolt_query_utils;

    auto Result = FCk_Jolt_HitResult{};

    const auto Context = Get_QueryContext(InWorldContextObject);
    if (ck::Is_NOT_Valid(Context._PhysicsSystem))
    { return Result; }

    const auto Ray = JPH::RRayCast{ck::jolt::Conv(InStart), ck::jolt::Conv(InEnd - InStart)};
    auto RayResult = JPH::RayCastResult{};

    const auto ChannelFilter = ck::jolt::FCk_Jolt_ChannelQueryFilter{
        Context._JoltSubsystem->Get_LayerTable(), InFilter.Get_Channel(), InFilter.Get_MinResponse()};

    if (NOT Context._PhysicsSystem->GetNarrowPhaseQuery().CastRay(Ray, RayResult,
        JPH::BroadPhaseLayerFilter{}, ChannelFilter))
    { return Result; }

    const auto HitPosition = Ray.GetPointOnRay(RayResult.mFraction);

    Result.Set_HasHit(true);
    Result.Set_Position(ck::jolt::Conv(HitPosition));
    Result.Set_Fraction(RayResult.mFraction);
    Result.Set_Normal(Get_SurfaceNormal(Context, RayResult.mBodyID, RayResult.mSubShapeID2, HitPosition));
    Fill_HitAttribution(Context, RayResult.mBodyID, Result);

    return Result;
}

auto
    UCk_Utils_JoltQuery_UE::
    Get_ShapeCast(
        const UObject* InWorldContextObject,
        FVector InStart,
        FVector InEnd,
        FRotator InRotation,
        FCk_Jolt_ShapeDimensions InShape,
        FCk_Jolt_QueryFilter InFilter)
    -> FCk_Jolt_HitResult
{
    using namespace ck_jolt_query_utils;

    auto Result = FCk_Jolt_HitResult{};

    const auto Context = Get_QueryContext(InWorldContextObject);
    if (ck::Is_NOT_Valid(Context._PhysicsSystem))
    { return Result; }

    const auto Shape = ck::jolt::CreateShape_FromDimensions(InShape, TEXT("JoltQuery_ShapeCast"));
    if (Shape == nullptr)
    { return Result; }

    const auto StartTransform = JPH::RMat44::sRotationTranslation(
        ck::jolt::Conv(InRotation.Quaternion()), ck::jolt::Conv(InStart));

    const auto ShapeCast = JPH::RShapeCast{
        Shape.GetPtr(), JPH::Vec3::sReplicate(1.0f), StartTransform, ck::jolt::Conv(InEnd - InStart)};

    const auto ChannelFilter = ck::jolt::FCk_Jolt_ChannelQueryFilter{
        Context._JoltSubsystem->Get_LayerTable(), InFilter.Get_Channel(), InFilter.Get_MinResponse()};

    auto Collector = JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector>{};

    Context._PhysicsSystem->GetNarrowPhaseQuery().CastShape(
        ShapeCast, JPH::ShapeCastSettings{}, JPH::RVec3::sZero(), Collector,
        JPH::BroadPhaseLayerFilter{}, ChannelFilter);

    if (NOT Collector.HadHit())
    { return Result; }

    const auto& Hit = Collector.mHit;

    Result.Set_HasHit(true);
    Result.Set_Position(ck::jolt::Conv(Hit.mContactPointOn2));
    Result.Set_Fraction(Hit.mFraction);
    Result.Set_Normal(ck::jolt::Conv(-Hit.mPenetrationAxis.Normalized()));
    Fill_HitAttribution(Context, Hit.mBodyID2, Result);

    return Result;
}

auto
    UCk_Utils_JoltQuery_UE::
    Get_Overlap(
        const UObject* InWorldContextObject,
        FVector InLocation,
        FRotator InRotation,
        FCk_Jolt_ShapeDimensions InShape,
        FCk_Jolt_QueryFilter InFilter)
    -> TArray<FCk_Jolt_HitResult>
{
    using namespace ck_jolt_query_utils;

    auto Results = TArray<FCk_Jolt_HitResult>{};

    const auto Context = Get_QueryContext(InWorldContextObject);
    if (ck::Is_NOT_Valid(Context._PhysicsSystem))
    { return Results; }

    const auto Shape = ck::jolt::CreateShape_FromDimensions(InShape, TEXT("JoltQuery_Overlap"));
    if (Shape == nullptr)
    { return Results; }

    const auto Transform = JPH::RMat44::sRotationTranslation(
        ck::jolt::Conv(InRotation.Quaternion()), ck::jolt::Conv(InLocation));

    const auto ChannelFilter = ck::jolt::FCk_Jolt_ChannelQueryFilter{
        Context._JoltSubsystem->Get_LayerTable(), InFilter.Get_Channel(), InFilter.Get_MinResponse()};

    auto Collector = JPH::AllHitCollisionCollector<JPH::CollideShapeCollector>{};

    Context._PhysicsSystem->GetNarrowPhaseQuery().CollideShape(
        Shape.GetPtr(), JPH::Vec3::sReplicate(1.0f), Transform, JPH::CollideShapeSettings{},
        JPH::RVec3::sZero(), Collector, JPH::BroadPhaseLayerFilter{}, ChannelFilter);

    Results.Reserve(Collector.mHits.size());

    for (const auto& Hit : Collector.mHits)
    {
        auto Result = FCk_Jolt_HitResult{};
        Result.Set_HasHit(true);
        Result.Set_Position(ck::jolt::Conv(Hit.mContactPointOn2));
        Result.Set_Normal(ck::jolt::Conv(-Hit.mPenetrationAxis.Normalized()));
        Fill_HitAttribution(Context, Hit.mBodyID2, Result);
        Results.Emplace(MoveTemp(Result));
    }

    return Results;
}

auto
    UCk_Utils_JoltQuery_UE::
    Get_RayCastMulti(
        const UObject* InWorldContextObject,
        FVector InStart,
        FVector InEnd,
        FCk_Jolt_QueryFilter InFilter)
    -> TArray<FCk_Jolt_HitResult>
{
    using namespace ck_jolt_query_utils;

    auto Results = TArray<FCk_Jolt_HitResult>{};

    const auto Context = Get_QueryContext(InWorldContextObject);
    if (ck::Is_NOT_Valid(Context._PhysicsSystem))
    { return Results; }

    const auto Ray = JPH::RRayCast{ck::jolt::Conv(InStart), ck::jolt::Conv(InEnd - InStart)};

    const auto ChannelFilter = ck::jolt::FCk_Jolt_ChannelQueryFilter{
        Context._JoltSubsystem->Get_LayerTable(), InFilter.Get_Channel(), InFilter.Get_MinResponse()};

    auto Collector = JPH::AllHitCollisionCollector<JPH::CastRayCollector>{};

    Context._PhysicsSystem->GetNarrowPhaseQuery().CastRay(
        Ray, JPH::RayCastSettings{}, Collector, JPH::BroadPhaseLayerFilter{}, ChannelFilter);

    Collector.Sort();

    Results.Reserve(Collector.mHits.size());

    for (const auto& Hit : Collector.mHits)
    {
        const auto HitPosition = Ray.GetPointOnRay(Hit.mFraction);

        auto Result = FCk_Jolt_HitResult{};
        Result.Set_HasHit(true);
        Result.Set_Position(ck::jolt::Conv(HitPosition));
        Result.Set_Fraction(Hit.mFraction);
        Result.Set_Normal(Get_SurfaceNormal(Context, Hit.mBodyID, Hit.mSubShapeID2, HitPosition));
        Fill_HitAttribution(Context, Hit.mBodyID, Result);
        Results.Emplace(MoveTemp(Result));
    }

    return Results;
}

auto
    UCk_Utils_JoltQuery_UE::
    Get_ShapeCastMulti(
        const UObject* InWorldContextObject,
        FVector InStart,
        FVector InEnd,
        FRotator InRotation,
        FCk_Jolt_ShapeDimensions InShape,
        FCk_Jolt_QueryFilter InFilter)
    -> TArray<FCk_Jolt_HitResult>
{
    using namespace ck_jolt_query_utils;

    auto Results = TArray<FCk_Jolt_HitResult>{};

    const auto Context = Get_QueryContext(InWorldContextObject);
    if (ck::Is_NOT_Valid(Context._PhysicsSystem))
    { return Results; }

    const auto Shape = ck::jolt::CreateShape_FromDimensions(InShape, TEXT("JoltQuery_ShapeCastMulti"));
    if (Shape == nullptr)
    { return Results; }

    const auto StartTransform = JPH::RMat44::sRotationTranslation(
        ck::jolt::Conv(InRotation.Quaternion()), ck::jolt::Conv(InStart));

    const auto ShapeCast = JPH::RShapeCast{
        Shape.GetPtr(), JPH::Vec3::sReplicate(1.0f), StartTransform, ck::jolt::Conv(InEnd - InStart)};

    const auto ChannelFilter = ck::jolt::FCk_Jolt_ChannelQueryFilter{
        Context._JoltSubsystem->Get_LayerTable(), InFilter.Get_Channel(), InFilter.Get_MinResponse()};

    auto Collector = JPH::AllHitCollisionCollector<JPH::CastShapeCollector>{};

    Context._PhysicsSystem->GetNarrowPhaseQuery().CastShape(
        ShapeCast, JPH::ShapeCastSettings{}, JPH::RVec3::sZero(), Collector,
        JPH::BroadPhaseLayerFilter{}, ChannelFilter);

    Collector.Sort();

    Results.Reserve(Collector.mHits.size());

    for (const auto& Hit : Collector.mHits)
    {
        auto Result = FCk_Jolt_HitResult{};
        Result.Set_HasHit(true);
        Result.Set_Position(ck::jolt::Conv(Hit.mContactPointOn2));
        Result.Set_Fraction(Hit.mFraction);
        Result.Set_Normal(ck::jolt::Conv(-Hit.mPenetrationAxis.Normalized()));
        Fill_HitAttribution(Context, Hit.mBodyID2, Result);
        Results.Emplace(MoveTemp(Result));
    }

    return Results;
}

auto
    UCk_Utils_JoltQuery_UE::
    Get_OverlapEntities(
        const UObject* InWorldContextObject,
        FVector InLocation,
        FRotator InRotation,
        FCk_Jolt_ShapeDimensions InShape,
        FCk_Jolt_QueryFilter InFilter)
    -> TArray<FCk_Handle>
{
    auto Results = TArray<FCk_Handle>{};

    const auto Hits = Get_Overlap(InWorldContextObject, InLocation, InRotation, InShape, InFilter);

    for (const auto& Hit : Hits)
    {
        const auto Entity = Hit.Get_Entity();
        if (ck::Is_NOT_Valid(Entity))
        { continue; }

        Results.AddUnique(Entity);
    }

    return Results;
}

// --------------------------------------------------------------------------------------------------------------------
