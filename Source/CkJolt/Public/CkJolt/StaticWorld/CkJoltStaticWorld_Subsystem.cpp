#include "CkJoltStaticWorld_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Stats.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"

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

    // Levels do not reliably fire LevelRemovedFromWorld during world teardown — free every
    // remaining body slot while the Jolt world (which we InitializeDependency on, so it outlives
    // us) is still alive.
    if (auto* BodyInterface = Get_BodyInterface())
    {
        auto AllBodyIds = TArray<JPH::BodyID>{};

        for (const auto& [Level, LevelBodies] : _LevelBodies)
        {
            for (const auto& RawBodyId : LevelBodies._BodyIds)
            { AllBodyIds.Emplace(JPH::BodyID{RawBodyId}); }
        }

        for (const auto& [Actor, BodyIds] : _ManualActorBodies)
        {
            for (const auto& RawBodyId : BodyIds)
            { AllBodyIds.Emplace(JPH::BodyID{RawBodyId}); }
        }

        if (AllBodyIds.Num() > 0)
        {
            BodyInterface->RemoveBodies(AllBodyIds.GetData(), AllBodyIds.Num());
            BodyInterface->DestroyBodies(AllBodyIds.GetData(), AllBodyIds.Num());
        }
    }

    _LevelBodies.Empty();
    _ManualActorBodies.Empty();
    _LoadedCells.Empty();
    _BodyToActorName.Empty();
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

    if (const auto* ActorName = _BodyToActorName.Find(RayResult.mBodyID.GetIndexAndSequenceNumber()))
    { Result._SourceActorName = *ActorName; }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    TryGet_ActorNameForBody(
        uint32 InBodyIndexAndSeq) const
    -> FName
{
    if (const auto* ActorName = _BodyToActorName.Find(InBodyIndexAndSeq))
    { return *ActorName; }

    return {};
}

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

    auto BodyIds = TArray<uint32>{};
    DoCreate_BodiesFromExtracted(Extracted, InActor.GetFName(), BodyIds);
    DoBatchAdd_Bodies(BodyIds);

    _ManualActorBodies.FindOrAdd(&InActor).Append(BodyIds);
    DoNote_BodiesChanged(BodyIds.Num());

    return BodyIds.Num();
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    Request_RemoveActor(
        const AActor& InActor)
        -> void
{
    auto RemovedBodyIds = TArray<uint32>{};
    if (NOT _ManualActorBodies.RemoveAndCopyValue(&InActor, RemovedBodyIds))
    { return; }

    auto* BodyInterface = Get_BodyInterface();
    if (BodyInterface == nullptr)
    { return; }

    auto BodyIds = TArray<JPH::BodyID>{};
    BodyIds.Reserve(RemovedBodyIds.Num());
    for (const auto& RawBodyId : RemovedBodyIds)
    {
        BodyIds.Emplace(JPH::BodyID{RawBodyId});
        _BodyToActorName.Remove(RawBodyId);
    }

    BodyInterface->RemoveBodies(BodyIds.GetData(), BodyIds.Num());
    BodyInterface->DestroyBodies(BodyIds.GetData(), BodyIds.Num());

    _NumStaticBodies -= BodyIds.Num();
    _BodyChurnSinceOptimize += BodyIds.Num();

    if (ck::IsValid(_JoltSubsystem))
    { _JoltSubsystem->Request_OptimizeBroadPhaseBeforeNextUpdate(); }
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

    auto BodyIds = TArray<uint32>{};
    auto CellIndices = TArray<int32>{};

    if (Get_UsesCookedData())
    { DoAdd_BodiesForLevel_Cooked(InLevel, BodyIds, CellIndices); }
    else
    { DoAdd_BodiesForLevel_LiveExtract(InLevel, BodyIds); }

    if (BodyIds.IsEmpty())
    {
        // Cells may have been loaded even when every actor's bodies were skipped (stale hashes) —
        // release them so the assets can GC.
        for (const auto& CellIndex : CellIndices)
        { DoRelease_Cell(CellIndex); }
        return;
    }

    DoBatchAdd_Bodies(BodyIds);

    auto& LevelBodies = _LevelBodies.Add(&InLevel);
    LevelBodies._BodyIds = MoveTemp(BodyIds);
    LevelBodies._CellIndices = MoveTemp(CellIndices);

    DoNote_BodiesChanged(LevelBodies._BodyIds.Num());

    ck::jolt::Verbose(TEXT("JoltStaticWorld: level [{}] added [{}] static bodies (total [{}])"),
        InLevel.GetOutermost()->GetName(), LevelBodies._BodyIds.Num(), _NumStaticBodies);
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

    auto* BodyInterface = Get_BodyInterface();
    if (BodyInterface == nullptr)
    { return; }

    auto BodyIds = TArray<JPH::BodyID>{};
    BodyIds.Reserve(LevelBodies._BodyIds.Num());
    for (const auto& RawBodyId : LevelBodies._BodyIds)
    {
        BodyIds.Emplace(JPH::BodyID{RawBodyId});
        _BodyToActorName.Remove(RawBodyId);
    }

    BodyInterface->RemoveBodies(BodyIds.GetData(), BodyIds.Num());
    BodyInterface->DestroyBodies(BodyIds.GetData(), BodyIds.Num());

    for (const auto& CellIndex : LevelBodies._CellIndices)
    { DoRelease_Cell(CellIndex); }

    _NumStaticBodies -= BodyIds.Num();
    _BodyChurnSinceOptimize += BodyIds.Num();

    if (ck::IsValid(_JoltSubsystem))
    { _JoltSubsystem->Request_OptimizeBroadPhaseBeforeNextUpdate(); }

    ck::jolt::Verbose(TEXT("JoltStaticWorld: level [{}] removed [{}] static bodies (total [{}])"),
        InLevel.GetOutermost()->GetName(), BodyIds.Num(), _NumStaticBodies);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoAdd_BodiesForLevel_LiveExtract(
        ULevel& InLevel,
        TArray<uint32>& OutBodyIds)
        -> void
{
    for (const auto& Actor : InLevel.Actors)
    {
        if (ck::Is_NOT_Valid(Actor))
        { continue; }

        auto Extracted = TArray<ck::jolt::bake::FCk_Jolt_ExtractedBody>{};
        ck::jolt::bake::ExtractActor(*Actor, _LiveShapeCache, Extracted);

        if (Extracted.IsEmpty())
        { continue; }

        DoCreate_BodiesFromExtracted(Extracted, Actor->GetFName(), OutBodyIds);
    }
}

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoAdd_BodiesForLevel_Cooked(
        ULevel& InLevel,
        TArray<uint32>& OutBodyIds,
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

        // Ref the cell ONCE per level (not per actor) — DoEnsure_CellLoaded loads at refcount 0;
        // the +1 happens here on first use by this level.
        auto AlreadyUsed = false;
        UsedCellIndices.Add(ActorRef->Get_CellIndex(), &AlreadyUsed);
        if (NOT AlreadyUsed)
        {
            if (auto* Cell = _LoadedCells.Find(ActorRef->Get_CellIndex()))
            { ++Cell->_RefCount; }
        }

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

            const auto* Body = BodyInterface->CreateBody(BodySettings);

            CK_ENSURE_IF_NOT(Body != nullptr,
                TEXT("Jolt body slot exhaustion while loading cooked bodies for [{}] — raise MaxBodies"),
                Actor->GetFName())
            { break; }

            const auto RawBodyId = Body->GetID().GetIndexAndSequenceNumber();
            OutBodyIds.Emplace(RawBodyId);
            _BodyToActorName.Add(RawBodyId, Actor->GetFName());
        }
    }

    OutCellIndices = UsedCellIndices.Array();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltStaticWorld_Subsystem_UE::
    DoCreate_BodiesFromExtracted(
        const TArray<ck::jolt::bake::FCk_Jolt_ExtractedBody>& InExtracted,
        FName InSourceActorName,
        TArray<uint32>& OutBodyIds)
        -> void
{
    auto* BodyInterface = Get_BodyInterface();
    if (BodyInterface == nullptr)
    { return; }

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

        const auto* Body = BodyInterface->CreateBody(BodySettings);

        CK_ENSURE_IF_NOT(Body != nullptr,
            TEXT("Jolt body slot exhaustion while baking [{}] — raise MaxBodies"), InSourceActorName)
        { return; }

        const auto RawBodyId = Body->GetID().GetIndexAndSequenceNumber();
        OutBodyIds.Emplace(RawBodyId);
        _BodyToActorName.Add(RawBodyId, InSourceActorName);
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
