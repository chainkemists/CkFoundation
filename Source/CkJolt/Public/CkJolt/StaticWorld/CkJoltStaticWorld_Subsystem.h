#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkJolt/StaticWorld/CkJoltBakeExtraction.h"
#include "CkJolt/StaticWorld/CkJoltStaticActor_Fragment_Data.h"
#include "CkJolt/StaticWorld/CkJoltStaticWorld_Data.h"
#include "CkJolt/Subsystem/CkJolt_Subsystem.h"

#include "CkJoltStaticWorld_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class ULevel;

namespace JPH
{
    class BodyInterface;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    /// Simple ray hit against the static world (test/introspection surface; the channel-filtered query
    /// API is UCk_Utils_JoltQuery_UE).
    struct CKJOLT_API FCk_Jolt_StaticWorldRayHit
    {
        bool _HasHit = false;
        FVector _Position = FVector::ZeroVector;
        // The hit body's source-actor attribution entity; INVALID when the body has no live entity.
        FCk_Handle _Entity;
    };

    /// Resolves the cooked-index asset path for a map by convention:
    /// <Root>/<MapPathSansRootPrefix>/JoltIndex — nothing hard-references cooked assets.
    CKJOLT_API auto Get_CookedIndexAssetPath(
        const FString& InCookedDataRootPath,
        const FString& InMapPackageName) -> FString;

    CKJOLT_API auto Get_CookedCellAssetPath(
        const FString& InCookedDataRootPath,
        const FString& InMapPackageName,
        FIntPoint InCellId) -> FString;
}

// --------------------------------------------------------------------------------------------------------------------

/*
 * Owns the baked static-world bodies, tracked per-ULevel so they add/remove in lockstep with level
 * streaming (World Partition runtime cells stream as ULevels, so one delegate pair covers WP and legacy
 * sublevels uniformly). Stale cooked data (version or per-actor hash mismatch) is ENSURED loudly and
 * skipped — never silently used, never re-extracted at runtime. See CkJolt/CLAUDE.md § Static world.
 */
UCLASS(DisplayName = "CkSubsystem_JoltStaticWorld")
class CKJOLT_API UCk_JoltStaticWorld_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_JoltStaticWorld_Subsystem_UE);

public:
    auto
    Initialize(
        FSubsystemCollectionBase& InCollection) -> void override;

    auto
    Deinitialize() -> void override;

    auto
    OnWorldBeginPlay(
        UWorld& InWorld) -> void override;

public:
    auto
    Get_NumStaticBodies() const -> int32;

    auto
    Get_NumUniqueShapes() const -> int32;

    /// Ray against static-world bodies only (Static body domain).
    auto
    Get_RayCastStaticWorld(
        const FVector& InStart,
        const FVector& InEnd) const -> ck::jolt::FCk_Jolt_StaticWorldRayHit;

    /// Extracts and adds static bodies for a single actor at runtime; returns the number added.
    auto
    Request_BakeActor(
        const AActor& InActor) -> int32;

    /// Removes bodies previously added via Request_BakeActor for this actor.
    auto
    Request_RemoveActor(
        const AActor& InActor) -> void;

    /// The single idempotent funnel for freeing a source actor's bodies, ending in EMPTYING the fragment's
    /// body-id array (that emptiness is the idempotence guard, since both the removal paths and
    /// FProcessor_JoltStaticActor_EndPlay call it). Does NOT destroy the entity.
    auto
    Request_RemoveBodiesForEntity(
        FCk_Handle_JoltStaticActor& InActorEntity) -> void;

private:
    auto
    DoHandle_LevelAdded(
        ULevel* InLevel,
        UWorld* InWorld) -> void;

    auto
    DoHandle_LevelRemoved(
        ULevel* InLevel,
        UWorld* InWorld) -> void;

    // Returns the level's extraction stats so the BeginPlay sweep can report a per-world summary
    // (zeroed for the cooked path — its skips happened at cook time and are loud there).
    auto
    DoAdd_BodiesForLevel(
        ULevel& InLevel) -> ck::jolt::bake::FCk_Jolt_ExtractionStats;

    auto
    DoRemove_BodiesForLevel(
        ULevel& InLevel) -> void;

    auto
    DoAdd_BodiesForLevel_LiveExtract(
        ULevel& InLevel,
        const FCk_Handle& InTransientEntity,
        TArray<FCk_Handle_JoltStaticActor>& OutActorEntities,
        TArray<uint32>& OutBodyIdsForBatch,
        ck::jolt::bake::FCk_Jolt_ExtractionStats& OutStats) -> void;

    auto
    DoAdd_BodiesForLevel_Cooked(
        ULevel& InLevel,
        const FCk_Handle& InTransientEntity,
        TArray<FCk_Handle_JoltStaticActor>& OutActorEntities,
        TArray<uint32>& OutBodyIdsForBatch,
        TArray<int32>& OutCellIndices) -> void;

    // Invalid return = the transient entity was not ready.
    auto
    DoCreate_ActorEntity(
        const FCk_Handle& InTransientEntity,
        const AActor& InSourceActor) -> FCk_Handle_JoltStaticActor;

    // Appends the raw body ids to BOTH the entity's fragment and the batch-add accumulator.
    auto
    DoCreate_BodiesFromExtracted(
        const TArray<ck::jolt::bake::FCk_Jolt_ExtractedBody>& InExtracted,
        FCk_Handle_JoltStaticActor& InActorEntity,
        TArray<uint32>& OutBodyIdsForBatch) -> void;

    auto
    DoBatchAdd_Bodies(
        const TArray<uint32>& InBodyIds) -> void;

    auto
    DoNote_BodiesChanged(
        int32 InCount) -> void;

    auto
    Get_BodyInterface() const -> JPH::BodyInterface*;

    auto
    Get_UsesCookedData() const -> bool;

    auto
    DoEnsure_IndexLoaded() -> bool;

    // Invalid until the ECS world is ready — the caller SKIPS the level for the OnWorldBeginPlay sweep.
    auto
    DoGet_TransientEntity() const -> FCk_Handle;

    // Registry-liveness check FIRST, no ensure on dead ids. Invalid when the id is 0 or dead.
    auto
    DoResolve_EntityFromUserData(
        uint64 InUserData) const -> FCk_Handle;

    struct FLoadedCell
    {
        TArray<JPH::Ref<JPH::Shape>> _Shapes;
        int32 _RefCount = 0;
    };

    auto
    DoEnsure_CellLoaded(
        int32 InCellIndex) -> FLoadedCell*;

    auto
    DoRelease_Cell(
        int32 InCellIndex) -> void;

private:
    UPROPERTY(Transient)
    TWeakObjectPtr<UCk_Jolt_Subsystem> _JoltSubsystem;

    UPROPERTY(Transient)
    TWeakObjectPtr<UCk_EcsWorld_Subsystem_UE> _EcsWorldSubsystem;

    UPROPERTY(Transient)
    TObjectPtr<UCk_Jolt_CookedWorldIndex_UE> _CookedIndex;

    FDelegateHandle _LevelAddedHandle;
    FDelegateHandle _LevelRemovedHandle;

    ck::jolt::bake::FCk_Jolt_ShapeCache _LiveShapeCache;

    struct FLevelBodies
    {
        TArray<FCk_Handle_JoltStaticActor> _ActorEntities;
        TArray<int32> _CellIndices;
    };

    TMap<TWeakObjectPtr<ULevel>, FLevelBodies> _LevelBodies;
    TMap<TWeakObjectPtr<const AActor>, FCk_Handle_JoltStaticActor> _ManualActorEntities;
    TMap<int32, FLoadedCell> _LoadedCells;

    int32 _NumStaticBodies = 0;
    int32 _BodyChurnSinceOptimize = 0;
    bool _CookedIndexLoadAttempted = false;
};

// --------------------------------------------------------------------------------------------------------------------
