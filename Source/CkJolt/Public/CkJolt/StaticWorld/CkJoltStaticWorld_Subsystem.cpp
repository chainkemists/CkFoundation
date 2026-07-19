#include "CkJoltStaticWorld_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Stats.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
#include "CkJolt/StaticWorld/CkJoltStaticActor_Fragment.h"

#include <Engine/Level.h>
#include <Engine/World.h>
#include <GameFramework/Actor.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/StreamWrapper.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/RayCast.h>

#include <sstream>

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("JoltStaticWorld_LevelAdd"), STAT_CkJolt_StaticWorldLevelAdd, STATGROUP_CkJolt);
DECLARE_CYCLE_STAT(TEXT("JoltStaticWorld_LevelRemove"), STAT_CkJolt_StaticWorldLevelRemove, STATGROUP_CkJolt);

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    auto
        Get_CookedIndexAssetPath(
            const FString& InCookedDataRootPath,
            const FString& InMapPackageName)
        -> FString
    {
        // /Game/Maps/TestMap -> <Root>/Maps/TestMap/JoltIndex.JoltIndex
        auto MapSubPath = InMapPackageName;
        MapSubPath.RemoveFromStart(TEXT("/Game"));

        return ck::Format_UE(TEXT("{}{}/JoltIndex.JoltIndex"), InCookedDataRootPath, MapSubPath);
    }

    auto
        Get_CookedCellAssetPath(
            const FString& InCookedDataRootPath,
            const FString& InMapPackageName,
            FIntPoint InCellId)
        -> FString
    {
        auto MapSubPath = InMapPackageName;
        MapSubPath.RemoveFromStart(TEXT("/Game"));

        return ck::Format_UE(TEXT("{}{}/JoltCell_{}_{}.JoltCell_{}_{}"), InCookedDataRootPath, MapSubPath,
            InCellId.X, InCellId.Y, InCellId.X, InCellId.Y);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
        -> void
{
    Super::Initialize(InCollection);

    _JoltSubsystem = InCollection.InitializeDependency<UCk_Jolt_Subsystem>();

    // The attribution entities live in the ECS registry — depend on the ECS world subsystem so it outlives us
    // and Deinitialize can still read their fragments while we free the Jolt bodies.
    _EcsWorldSubsystem = InCollection.InitializeDependency<UCk_EcsWorld_Subsystem_UE>();

    _LevelAddedHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(
        this, &ThisType::DoHandle_LevelAdded);
    _LevelRemovedHandle = FWorldDelegates::LevelRemovedFromWorld.AddUObject(
        this, &ThisType::DoHandle_LevelRemoved);
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    Deinitialize()
        -> void
{
    FWorldDelegates::LevelAddedToWorld.Remove(_LevelAddedHandle);
    FWorldDelegates::LevelRemovedFromWorld.Remove(_LevelRemovedHandle);

    // Levels do not reliably fire LevelRemovedFromWorld during world teardown — free every remaining source
    // actor's bodies (and destroy its attribution entity) while both the Jolt world and the ECS registry
    // (which we InitializeDependency on, so they outlive us) are still alive. Dead handles are tolerated (an
    // entity destroyed first already routed through the funnel via its EndPlay processor).
    for (auto& [Level, LevelBodies] : _LevelBodies)
    {
        for (auto& ActorEntity : LevelBodies._ActorEntities)
        {
            if (ck::Is_NOT_Valid(ActorEntity))
            { continue; }

            Request_RemoveBodiesForEntity(ActorEntity);

            auto GenericHandle = FCk_Handle{ActorEntity};
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(GenericHandle);
        }
    }

    for (auto& [Actor, ActorEntity] : _ManualActorEntities)
    {
        if (ck::Is_NOT_Valid(ActorEntity))
        { continue; }

        Request_RemoveBodiesForEntity(ActorEntity);

        auto GenericHandle = FCk_Handle{ActorEntity};
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(GenericHandle);
    }

    _LevelBodies.Empty();
    _ManualActorEntities.Empty();
    _LoadedCells.Empty();
    _NumStaticBodies = 0;

    Super::Deinitialize();
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    OnWorldBeginPlay(
        UWorld& InWorld)
        -> void
{
    Super::OnWorldBeginPlay(InWorld);

#if WITH_EDITOR
    if (UCk_Utils_Jolt_ProjectSettings::Get_PIEStaticWorldMode() == ECk_Jolt_PIEStaticWorldMode::Disabled)
    { return; }
#endif

    for (const auto& Level : InWorld.GetLevels())
    {
        if (ck::Is_NOT_Valid(Level))
        { continue; }

        DoAdd_BodiesForLevel(*Level);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    Get_NumStaticBodies() const
    -> int32
{
    return _NumStaticBodies;
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    Get_NumUniqueShapes() const
    -> int32
{
    return _LiveShapeCache.Get_NumUniqueShapes();
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    Get_RayCastStaticWorld(
        const FVector& InStart,
        const FVector& InEnd) const
    -> ck::jolt::FCk_Jolt_StaticWorldRayHit
{
    auto Result = ck::jolt::FCk_Jolt_StaticWorldRayHit{};

    if (ck::Is_NOT_Valid(_JoltSubsystem))
    { return Result; }

    const auto PhysicsSystem = _JoltSubsystem->Get_PhysicsSystem().Pin();
    if (ck::Is_NOT_Valid(PhysicsSystem))
    { return Result; }

    const auto Ray = JPH::RRayCast{ck::jolt::Conv(InStart), ck::jolt::Conv(InEnd - InStart)};

    auto RayResult = JPH::RayCastResult{};

    const auto StaticWorldLayerFilter = ck::jolt::FCk_Jolt_DomainQueryFilter{
        _JoltSubsystem->Get_LayerTable(), ECk_Jolt_BodyDomain::Static};

    if (NOT PhysicsSystem->GetNarrowPhaseQuery().CastRay(Ray, RayResult,
        JPH::BroadPhaseLayerFilter{}, StaticWorldLayerFilter))
    { return Result; }

    Result._HasHit = true;
    Result._Position = ck::jolt::Conv(Ray.GetPointOnRay(RayResult.mFraction));

    // Attribution now flows through the body's Jolt user-data (the source actor's entity id), exactly like a
    // dynamic JoltBody hit — no separate FName table.
    const auto& BodyInterface = PhysicsSystem->GetBodyInterface();
    Result._Entity = DoResolve_EntityFromUserData(BodyInterface.GetUserData(RayResult.mBodyID));

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    Request_BakeActor(
        const AActor& InActor)
        -> int32
{
    auto Extracted = TArray<ck::jolt::bake::FCk_Jolt_ExtractedBody>{};
    ck::jolt::bake::ExtractActor(InActor, _LiveShapeCache, Extracted,
        ck::jolt::bake::ECk_Jolt_ExtractionPolicy::ExplicitActor);

    if (Extracted.IsEmpty())
    { return 0; }

    const auto TransientEntity = DoGet_TransientEntity();

    CK_ENSURE_IF_NOT(ck::IsValid(TransientEntity),
        TEXT("Request_BakeActor for [{}] has no live ECS transient entity to attribute bodies to — the ECS "
             "world is not ready."), InActor.GetFName())
    { return 0; }

    // Re-baking an already-baked actor REPLACES its attribution: free the previous bake's bodies and
    // entity first — overwriting the map entry would orphan them (unreachable by Request_RemoveActor
    // AND by Deinitialize).
    if (auto* ExistingEntity = _ManualActorEntities.Find(&InActor))
    {
        if (ck::IsValid(*ExistingEntity))
        {
            Request_RemoveBodiesForEntity(*ExistingEntity);

            auto GenericHandle = FCk_Handle{*ExistingEntity};
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(GenericHandle);
        }

        _ManualActorEntities.Remove(&InActor);
    }

    auto ActorEntity = DoCreate_ActorEntity(TransientEntity, InActor);
    if (ck::Is_NOT_Valid(ActorEntity))
    { return 0; }

    auto BodyIds = TArray<uint32>{};
    DoCreate_BodiesFromExtracted(Extracted, ActorEntity, BodyIds);
    DoBatchAdd_Bodies(BodyIds);

    _ManualActorEntities.Add(&InActor, ActorEntity);
    DoNote_BodiesChanged(BodyIds.Num());

    return BodyIds.Num();
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    Request_RemoveActor(
        const AActor& InActor)
        -> void
{
    auto ActorEntity = FCk_Handle_JoltStaticActor{};
    if (NOT _ManualActorEntities.RemoveAndCopyValue(&InActor, ActorEntity))
    { return; }

    if (ck::Is_NOT_Valid(ActorEntity))
    { return; }

    Request_RemoveBodiesForEntity(ActorEntity);

    auto GenericHandle = FCk_Handle{ActorEntity};
    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(GenericHandle);
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    Request_RemoveBodiesForEntity(
        FCk_Handle_JoltStaticActor& InActorEntity)
        -> void
{
    // IncludePendingKill so this works from FProcessor_JoltStaticActor_EndPlay (the entity is pending-destroy
    // there) as well as the fully-alive subsystem-driven paths.
    if (ck::Is_NOT_Valid(InActorEntity, ck::IsValid_Policy_IncludePendingKill{}))
    { return; }

    if (NOT InActorEntity.Has<ck::FFragment_JoltStaticActor_Current>())
    { return; }

    auto& Fragment = InActorEntity.Get<ck::FFragment_JoltStaticActor_Current>();

    // Empty body-id array = already freed. This is the idempotence guard that makes the bidirectional
    // lifecycle safe (whichever removal path runs first empties it; the other no-ops here).
    if (Fragment.Get_BodyIds().IsEmpty())
    { return; }

    auto* BodyInterface = Get_BodyInterface();
    if (BodyInterface == nullptr)
    {
        // Jolt world already gone (teardown) — nothing to free; still empty the array so a later pass no-ops.
        Fragment._BodyIds.Empty();
        return;
    }

    auto BodyIds = TArray<JPH::BodyID>{};
    BodyIds.Reserve(Fragment.Get_BodyIds().Num());
    for (const auto& RawBodyId : Fragment.Get_BodyIds())
    { BodyIds.Emplace(JPH::BodyID{RawBodyId}); }

    BodyInterface->RemoveBodies(BodyIds.GetData(), BodyIds.Num());
    BodyInterface->DestroyBodies(BodyIds.GetData(), BodyIds.Num());

    _NumStaticBodies -= BodyIds.Num();
    _BodyChurnSinceOptimize += BodyIds.Num();

    if (ck::IsValid(_JoltSubsystem))
    { _JoltSubsystem->Request_OptimizeBroadPhaseBeforeNextUpdate(); }

    Fragment._BodyIds.Empty();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoHandle_LevelAdded(
        ULevel* InLevel,
        UWorld* InWorld)
        -> void
{
    if (InWorld != GetWorld() || ck::Is_NOT_Valid(InLevel))
    { return; }

#if WITH_EDITOR
    if (UCk_Utils_Jolt_ProjectSettings::Get_PIEStaticWorldMode() == ECk_Jolt_PIEStaticWorldMode::Disabled)
    { return; }
#endif

    DoAdd_BodiesForLevel(*InLevel);
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoHandle_LevelRemoved(
        ULevel* InLevel,
        UWorld* InWorld)
        -> void
{
    if (InWorld != GetWorld() || ck::Is_NOT_Valid(InLevel))
    { return; }

    DoRemove_BodiesForLevel(*InLevel);
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoAdd_BodiesForLevel(
        ULevel& InLevel)
        -> void
{
    SCOPE_CYCLE_COUNTER(STAT_CkJolt_StaticWorldLevelAdd);

    if (_LevelBodies.Contains(&InLevel))
    { return; }

    // Every contributing actor needs a live transient entity to parent its attribution entity. If the ECS
    // world is not ready yet (a level can be added before BeginPlay), SKIP — the OnWorldBeginPlay sweep
    // re-attempts (this level is not recorded, so the _LevelBodies.Contains guard above lets it retry).
    const auto TransientEntity = DoGet_TransientEntity();
    if (ck::Is_NOT_Valid(TransientEntity))
    {
        ck::jolt::Verbose(TEXT("JoltStaticWorld: level [{}] added before the ECS world was ready — deferring "
            "to the BeginPlay sweep"), InLevel.GetOutermost()->GetName());
        return;
    }

    auto ActorEntities = TArray<FCk_Handle_JoltStaticActor>{};
    auto BodyIds = TArray<uint32>{};
    auto CellIndices = TArray<int32>{};

    if (Get_UsesCookedData())
    { DoAdd_BodiesForLevel_Cooked(InLevel, TransientEntity, ActorEntities, BodyIds, CellIndices); }
    else
    { DoAdd_BodiesForLevel_LiveExtract(InLevel, TransientEntity, ActorEntities, BodyIds); }

    if (ActorEntities.IsEmpty())
    {
        // Cells may have been loaded even when every actor's bodies were skipped (stale hashes) —
        // release them so the assets can GC.
        for (const auto& CellIndex : CellIndices)
        { DoRelease_Cell(CellIndex); }
        return;
    }

    DoBatchAdd_Bodies(BodyIds);

    const auto NumBodies = BodyIds.Num();
    const auto NumEntities = ActorEntities.Num();

    auto& LevelBodies = _LevelBodies.Add(&InLevel);
    LevelBodies._ActorEntities = MoveTemp(ActorEntities);
    LevelBodies._CellIndices = MoveTemp(CellIndices);

    DoNote_BodiesChanged(NumBodies);

    ck::jolt::Verbose(TEXT("JoltStaticWorld: level [{}] added [{}] static bodies across [{}] source entities "
        "(total [{}])"), InLevel.GetOutermost()->GetName(), NumBodies, NumEntities, _NumStaticBodies);
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoRemove_BodiesForLevel(
        ULevel& InLevel)
        -> void
{
    SCOPE_CYCLE_COUNTER(STAT_CkJolt_StaticWorldLevelRemove);

    auto LevelBodies = FLevelBodies{};
    if (NOT _LevelBodies.RemoveAndCopyValue(&InLevel, LevelBodies))
    { return; }

    for (auto& ActorEntity : LevelBodies._ActorEntities)
    {
        // Tolerate dead handles: an attribution entity destroyed first already freed its bodies through the
        // funnel (its EndPlay processor).
        if (ck::Is_NOT_Valid(ActorEntity))
        { continue; }

        Request_RemoveBodiesForEntity(ActorEntity);

        auto GenericHandle = FCk_Handle{ActorEntity};
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(GenericHandle);
    }

    // Cooked-cell refcounts stay level-scoped: releasing here (not on an early entity destroy) keeps a cell
    // pinned for as long as its level is loaded.
    for (const auto& CellIndex : LevelBodies._CellIndices)
    { DoRelease_Cell(CellIndex); }

    ck::jolt::Verbose(TEXT("JoltStaticWorld: level [{}] removed [{}] source entities (total bodies now [{}])"),
        InLevel.GetOutermost()->GetName(), LevelBodies._ActorEntities.Num(), _NumStaticBodies);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoAdd_BodiesForLevel_LiveExtract(
        ULevel& InLevel,
        const FCk_Handle& InTransientEntity,
        TArray<FCk_Handle_JoltStaticActor>& OutActorEntities,
        TArray<uint32>& OutBodyIdsForBatch)
        -> void
{
    for (const auto& Actor : InLevel.Actors)
    {
        if (ck::Is_NOT_Valid(Actor))
        { continue; }

        auto Extracted = TArray<ck::jolt::bake::FCk_Jolt_ExtractedBody>{};
        ck::jolt::bake::ExtractActor(*Actor, _LiveShapeCache, Extracted);

        // No contribution => no attribution entity (D1).
        if (Extracted.IsEmpty())
        { continue; }

        auto ActorEntity = DoCreate_ActorEntity(InTransientEntity, *Actor);
        if (ck::Is_NOT_Valid(ActorEntity))
        { continue; }

        DoCreate_BodiesFromExtracted(Extracted, ActorEntity, OutBodyIdsForBatch);
        OutActorEntities.Emplace(ActorEntity);
    }
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoAdd_BodiesForLevel_Cooked(
        ULevel& InLevel,
        const FCk_Handle& InTransientEntity,
        TArray<FCk_Handle_JoltStaticActor>& OutActorEntities,
        TArray<uint32>& OutBodyIdsForBatch,
        TArray<int32>& OutCellIndices)
        -> void
{
    if (NOT DoEnsure_IndexLoaded())
    { return; }

    auto* BodyInterface = Get_BodyInterface();
    if (BodyInterface == nullptr)
    { return; }

    const auto& ActorLookup = _CookedIndex->Get_ActorLookup();
    const auto& Cells = _CookedIndex->Get_Cells();
    auto UsedCellIndices = TSet<int32>{};

    for (const auto& Actor : InLevel.Actors)
    {
        if (ck::Is_NOT_Valid(Actor))
        { continue; }

        const auto* ActorRef = ActorLookup.Find(Actor->GetFName());
        if (ActorRef == nullptr)
        { continue; }

        CK_ENSURE_IF_NOT(Cells.IsValidIndex(ActorRef->Get_CellIndex()),
            TEXT("Cooked Jolt index for actor [{}] references cell [{}] out of range [{}] — corrupt index, skipping"),
            Actor->GetFName(), ActorRef->Get_CellIndex(), Cells.Num())
        { continue; }

        const auto* LoadedCell = DoEnsure_CellLoaded(ActorRef->Get_CellIndex());
        if (LoadedCell == nullptr)
        { continue; }

        const auto& CellAsset = Cells[ActorRef->Get_CellIndex()].Get_CellAsset();
        const auto& ActorGroups = CellAsset.Get()->Get_ActorGroups();

        CK_ENSURE_IF_NOT(ActorGroups.IsValidIndex(ActorRef->Get_GroupIndex()),
            TEXT("Cooked Jolt cell for actor [{}] references group [{}] out of range [{}] — corrupt cell, skipping"),
            Actor->GetFName(), ActorRef->Get_GroupIndex(), ActorGroups.Num())
        { continue; }

        const auto& Group = ActorGroups[ActorRef->Get_GroupIndex()];

        const auto CurrentHash = ck::jolt::bake::ComputeRuntimeCheckHash(*Actor);
        CK_ENSURE_IF_NOT(CurrentHash == Group.Get_RuntimeCheckHash(),
            TEXT("STALE cooked Jolt data for actor [{}] (hash [{}] vs cooked [{}]) — its bodies are SKIPPED, "
                 "not silently substituted. Re-cook the map."),
            Actor->GetFName(), CurrentHash, Group.Get_RuntimeCheckHash())
        { continue; }

        // No cooked bodies => no attribution entity (D1) and no cell ref.
        if (Group.Get_Bodies().IsEmpty())
        { continue; }

        // Ref the cell ONCE per level (not per actor) — DoEnsure_CellLoaded loads at refcount 0;
        // the +1 happens here on first use by this level.
        auto AlreadyUsed = false;
        UsedCellIndices.Add(ActorRef->Get_CellIndex(), &AlreadyUsed);
        if (NOT AlreadyUsed)
        {
            if (auto* Cell = _LoadedCells.Find(ActorRef->Get_CellIndex()))
            { ++Cell->_RefCount; }
        }

        auto ActorEntity = DoCreate_ActorEntity(InTransientEntity, *Actor);
        if (ck::Is_NOT_Valid(ActorEntity))
        { continue; }

        auto& Fragment = ActorEntity.Get<ck::FFragment_JoltStaticActor_Current>();
        const auto EntityUserData = static_cast<uint64>(ActorEntity.Get_Entity().Get_ID());

        for (const auto& Record : Group.Get_Bodies())
        {
            CK_ENSURE_IF_NOT(LoadedCell->_Shapes.IsValidIndex(Record.Get_ShapeIndex()),
                TEXT("Cooked body record for actor [{}] references shape [{}] out of range [{}] — skipping"),
                Actor->GetFName(), Record.Get_ShapeIndex(), LoadedCell->_Shapes.Num())
            { continue; }

            const auto& Shape = LoadedCell->_Shapes[Record.Get_ShapeIndex()];

            // The stored signature resolves to a live layer id at load — layer indices are
            // per-session and are never serialized.
            const auto Layer = _JoltSubsystem->Get_LayerTable().Get_OrRegisterLayer(Record.Get_Signature());
            if (Layer == JPH::cObjectLayerInvalid)
            { continue; }

            auto BodySettings = JPH::BodyCreationSettings{
                Shape.GetPtr(),
                ck::jolt::Conv(Record.Get_Position()),
                ck::jolt::Conv(Record.Get_Rotation()),
                JPH::EMotionType::Static,
                JPH::ObjectLayer{Layer}};
            BodySettings.mFriction = Record.Get_Friction();
            BodySettings.mRestitution = Record.Get_Restitution();
            // Stamp the source actor's entity id so a static hit resolves back to its attribution entity.
            BodySettings.mUserData = EntityUserData;

            const auto* Body = BodyInterface->CreateBody(BodySettings);

            CK_ENSURE_IF_NOT(Body != nullptr,
                TEXT("Jolt body slot exhaustion while loading cooked bodies for [{}] — raise MaxBodies"),
                Actor->GetFName())
            { break; }

            const auto RawBodyId = Body->GetID().GetIndexAndSequenceNumber();
            OutBodyIdsForBatch.Emplace(RawBodyId);
            Fragment._BodyIds.Emplace(RawBodyId);
        }

        OutActorEntities.Emplace(ActorEntity);
    }

    OutCellIndices = UsedCellIndices.Array();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoCreate_ActorEntity(
        const FCk_Handle& InTransientEntity,
        const AActor& InSourceActor)
        -> FCk_Handle_JoltStaticActor
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InTransientEntity);
    if (ck::Is_NOT_Valid(NewEntity))
    { return {}; }

    NewEntity.Add<ck::FFragment_JoltStaticActor_Current>();

    auto& Fragment = NewEntity.Get<ck::FFragment_JoltStaticActor_Current>();
    Fragment._SourceActor = &InSourceActor;
    Fragment._SourceActorName = InSourceActor.GetFName();

    UCk_Utils_Handle_UE::Set_DebugName(NewEntity, InSourceActor.GetFName());

    return ck::StaticCast<FCk_Handle_JoltStaticActor>(NewEntity);
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoCreate_BodiesFromExtracted(
        const TArray<ck::jolt::bake::FCk_Jolt_ExtractedBody>& InExtracted,
        FCk_Handle_JoltStaticActor& InActorEntity,
        TArray<uint32>& OutBodyIdsForBatch)
        -> void
{
    auto* BodyInterface = Get_BodyInterface();
    if (BodyInterface == nullptr)
    { return; }

    auto& Fragment = InActorEntity.Get<ck::FFragment_JoltStaticActor_Current>();
    const auto EntityUserData = static_cast<uint64>(InActorEntity.Get_Entity().Get_ID());

    for (const auto& Extracted : InExtracted)
    {
        if (Extracted._Shape == nullptr)
        { continue; }

        const auto Layer = _JoltSubsystem->Get_LayerTable().Get_OrRegisterLayer(Extracted._Signature);
        if (Layer == JPH::cObjectLayerInvalid)
        { continue; }

        auto BodySettings = JPH::BodyCreationSettings{
            Extracted._Shape.GetPtr(),
            ck::jolt::Conv(Extracted._Position),
            ck::jolt::Conv(Extracted._Rotation),
            JPH::EMotionType::Static,
            JPH::ObjectLayer{Layer}};
        BodySettings.mFriction = Extracted._Friction;
        BodySettings.mRestitution = Extracted._Restitution;
        // Stamp the source actor's entity id so a static hit resolves back to its attribution entity.
        BodySettings.mUserData = EntityUserData;

        const auto* Body = BodyInterface->CreateBody(BodySettings);

        CK_ENSURE_IF_NOT(Body != nullptr,
            TEXT("Jolt body slot exhaustion while baking [{}] — raise MaxBodies"), Fragment.Get_SourceActorName())
        { return; }

        const auto RawBodyId = Body->GetID().GetIndexAndSequenceNumber();
        OutBodyIdsForBatch.Emplace(RawBodyId);
        Fragment._BodyIds.Emplace(RawBodyId);
    }
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoBatchAdd_Bodies(
        const TArray<uint32>& InBodyIds)
        -> void
{
    if (InBodyIds.IsEmpty())
    { return; }

    auto* BodyInterface = Get_BodyInterface();
    if (BodyInterface == nullptr)
    { return; }

    auto BodyIds = TArray<JPH::BodyID>{};
    BodyIds.Reserve(InBodyIds.Num());
    for (const auto& RawBodyId : InBodyIds)
    { BodyIds.Emplace(JPH::BodyID{RawBodyId}); }

    const auto AddState = BodyInterface->AddBodiesPrepare(BodyIds.GetData(), BodyIds.Num());
    BodyInterface->AddBodiesFinalize(BodyIds.GetData(), BodyIds.Num(), AddState, JPH::EActivation::DontActivate);
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoNote_BodiesChanged(
        int32 InCount)
        -> void
{
    _NumStaticBodies += InCount;
    _BodyChurnSinceOptimize += InCount;

    if (ck::Is_NOT_Valid(_JoltSubsystem))
    { return; }

    if (_BodyChurnSinceOptimize >= UCk_Utils_Jolt_ProjectSettings::Get_BroadphaseOptimizeThreshold())
    {
        _BodyChurnSinceOptimize = 0;
        _JoltSubsystem->Request_OptimizeBroadPhaseBeforeNextUpdate();
    }
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    Get_BodyInterface() const
    -> JPH::BodyInterface*
{
    if (ck::Is_NOT_Valid(_JoltSubsystem))
    { return nullptr; }

    const auto PhysicsSystem = _JoltSubsystem->Get_PhysicsSystem().Pin();
    if (ck::Is_NOT_Valid(PhysicsSystem))
    { return nullptr; }

    return &PhysicsSystem->GetBodyInterface();
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    Get_UsesCookedData() const
    -> bool
{
#if WITH_EDITOR
    return UCk_Utils_Jolt_ProjectSettings::Get_PIEStaticWorldMode() == ECk_Jolt_PIEStaticWorldMode::Cooked;
#else
    return true;
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoGet_TransientEntity() const
    -> FCk_Handle
{
    if (ck::Is_NOT_Valid(_EcsWorldSubsystem))
    { return {}; }

    return _EcsWorldSubsystem->Get_TransientEntity();
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoResolve_EntityFromUserData(
        uint64 InUserData) const
    -> FCk_Handle
{
    // UserData 0 = NO entity: baked bodies always carry a non-zero source-actor entity id now, and raw entity
    // id 0 is always the registry's live transient root (resolving it would mis-attribute).
    if (InUserData == 0)
    { return {}; }

    if (ck::Is_NOT_Valid(_EcsWorldSubsystem))
    { return {}; }

    const auto TransientEntity = _EcsWorldSubsystem->Get_TransientEntity();
    if (ck::Is_NOT_Valid(TransientEntity))
    { return {}; }

    // Registry-liveness check FIRST — no ensure on a dead id (mirrors CkJoltQuery_Utils::TryResolve_Entity).
    const auto RegView = TransientEntity.Get_RegistryView();
    const auto Entity = FCk_Entity{static_cast<FCk_Entity::IdType>(InUserData)};
    if (NOT RegView.IsValid(Entity))
    { return {}; }

    return TransientEntity.Get_ValidHandle(Entity.Get_ID());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoEnsure_IndexLoaded()
    -> bool
{
    if (ck::IsValid(_CookedIndex))
    { return true; }

    if (_CookedIndexLoadAttempted)
    { return false; }

    _CookedIndexLoadAttempted = true;

    const auto MapPackageName = GetWorld()->PersistentLevel->GetOutermost()->GetName();
    const auto IndexPath = ck::jolt::Get_CookedIndexAssetPath(
        UCk_Utils_Jolt_ProjectSettings::Get_CookedDataRootPath(), MapPackageName);

    _CookedIndex = LoadObject<UCk_Jolt_CookedWorldIndex_UE>(nullptr, *IndexPath);

    if (ck::Is_NOT_Valid(_CookedIndex))
    {
        // Absence is LEGAL — maps opt into Jolt cooked data. Only stale data is a crime.
        ck::jolt::Display(TEXT("JoltStaticWorld: no cooked index at [{}] — map has no Jolt static world"),
            IndexPath);
        return false;
    }

    const auto CookVersionMatches = _CookedIndex->Get_CookVersion() == ck::jolt::CookVersion_Current;
    const auto JoltVersionMatches = _CookedIndex->Get_JoltVersionId() == static_cast<uint32>(JPH_VERSION_ID);

    CK_ENSURE_IF_NOT(CookVersionMatches && JoltVersionMatches,
        TEXT("Cooked Jolt index for [{}] is STALE (cook version [{}] vs [{}], Jolt version [{}] vs [{}]) — "
             "the entire map's cooked Jolt data is SKIPPED. Re-cook the map."),
        MapPackageName, _CookedIndex->Get_CookVersion(), ck::jolt::CookVersion_Current,
        _CookedIndex->Get_JoltVersionId(), static_cast<uint32>(JPH_VERSION_ID))
    {
        _CookedIndex = nullptr;
        return false;
    }

    return true;
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoEnsure_CellLoaded(
        int32 InCellIndex)
    -> FLoadedCell*
{
    // Loads at refcount 0 — the caller refs once per unique (level, cell) pair.
    if (auto* Existing = _LoadedCells.Find(InCellIndex))
    { return Existing; }

    const auto& Cells = _CookedIndex->Get_Cells();
    const auto& CellRef = Cells[InCellIndex];

    // Synchronous on purpose: this is the same moment Chaos pays for level add — collision must
    // exist the frame the level is visible.
    const auto* CellAsset = CellRef.Get_CellAsset().LoadSynchronous();

    CK_ENSURE_IF_NOT(ck::IsValid(CellAsset, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Cooked Jolt cell [{}] failed to load from [{}]"),
        InCellIndex, CellRef.Get_CellAsset().ToString())
    { return nullptr; }

    const auto CookVersionMatches = CellAsset->Get_CookVersion() == ck::jolt::CookVersion_Current;
    const auto JoltVersionMatches = CellAsset->Get_JoltVersionId() == static_cast<uint32>(JPH_VERSION_ID);

    CK_ENSURE_IF_NOT(CookVersionMatches && JoltVersionMatches,
        TEXT("Cooked Jolt cell [{}] is STALE (cook version [{}] vs [{}], Jolt version [{}] vs [{}]) — skipped"),
        InCellIndex, CellAsset->Get_CookVersion(), ck::jolt::CookVersion_Current,
        CellAsset->Get_JoltVersionId(), static_cast<uint32>(JPH_VERSION_ID))
    { return nullptr; }

    const auto& Blob = CellAsset->Get_ShapeBlob();

    auto BlobStream = std::istringstream{
        std::string{reinterpret_cast<const char*>(Blob.GetData()), static_cast<size_t>(Blob.Num())}};
    auto StreamWrapper = JPH::StreamInWrapper{BlobStream};

    auto IdToShape = JPH::Shape::IDToShapeMap{};
    auto IdToMaterial = JPH::Shape::IDToMaterialMap{};

    auto LoadedCell = FLoadedCell{};
    LoadedCell._Shapes.Reserve(CellAsset->Get_ShapeCount());

    for (auto Index = 0; Index < CellAsset->Get_ShapeCount(); ++Index)
    {
        const auto Result = JPH::Shape::sRestoreWithChildren(StreamWrapper, IdToShape, IdToMaterial);

        CK_ENSURE_IF_NOT(Result.IsValid(),
            TEXT("Cooked Jolt cell [{}] shape [{}] failed to restore: [{}] — cell skipped"),
            InCellIndex, Index, FString{Result.GetError().c_str()})
        { return nullptr; }

        LoadedCell._Shapes.Emplace(Result.Get());
    }

    LoadedCell._RefCount = 0;
    return &_LoadedCells.Add(InCellIndex, MoveTemp(LoadedCell));
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoRelease_Cell(
        int32 InCellIndex)
        -> void
{
    auto* LoadedCell = _LoadedCells.Find(InCellIndex);
    if (LoadedCell == nullptr)
    { return; }

    --LoadedCell->_RefCount;

    if (LoadedCell->_RefCount <= 0)
    { _LoadedCells.Remove(InCellIndex); }
}

// --------------------------------------------------------------------------------------------------------------------
