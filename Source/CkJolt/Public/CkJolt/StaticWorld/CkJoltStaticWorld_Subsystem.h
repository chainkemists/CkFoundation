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
    /// Simple ray hit against the static world (Phase-1 test/introspection surface — the general
    /// channel-filtered query API arrives with the Phase-2 layer table).
    struct CKJOLT_API FCk_Jolt_StaticWorldRayHit
    {
        bool _HasHit = false;
        FVector _Position = FVector::ZeroVector;
        // The hit body's source-actor attribution entity (FCk_Handle_JoltStaticActor), resolved from the
        // body's Jolt user-data; INVALID when the body has no live entity.
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
 * Owns the baked static-world bodies with ECS-first attribution: ONE entity per source actor that
 * contributes at least one baked body (ck::FFragment_JoltStaticActor_Current), tracked per-ULevel so bodies
 * add/remove in exact lockstep with level streaming (World Partition runtime cells stream as ULevels, so one
 * delegate pair covers WP and legacy sublevels uniformly). Each body is stamped with its source actor's
 * entity id as Jolt user-data, so a static-world hit resolves back to the entity through the same path a
 * dynamic JoltBody hit uses — there is no separate FName attribution table.
 *
 * Lifecycle is bidirectional: the level-streaming / Request_RemoveActor / Deinitialize paths remove bodies
 * AND destroy the attribution entities; and if an attribution entity is destroyed first,
 * FProcessor_JoltStaticActor_EndPlay removes its bodies through the same idempotent funnel
 * (Request_RemoveBodiesForEntity).
 *
 * Sources: live extraction from level actors (PIE default — stale cooked data can never silently
 * affect PIE) or cooked per-cell data assets (packaged builds always; PIE opt-in via settings).
 * Stale cooked data (version or per-actor hash mismatch) is ENSURED loudly and skipped — never
 * silently used, never re-extracted at runtime.
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

    /// Ray against static-world bodies only (Static body domain). Test/introspection surface;
    /// the channel-filtered query API is UCk_Utils_JoltQuery_UE.
    auto
    Get_RayCastStaticWorld(
        const FVector& InStart,
        const FVector& InEnd) const -> ck::jolt::FCk_Jolt_StaticWorldRayHit;

    /// Extracts and adds static bodies for a single actor at runtime (also the AS test surface).
    /// Returns the number of bodies added.
    auto
    Request_BakeActor(
        const AActor& InActor) -> int32;

    /// Removes bodies previously added via Request_BakeActor for this actor.
    auto
    Request_RemoveActor(
        const AActor& InActor) -> void;

    /// The single idempotent funnel for freeing a source actor's bodies: reads the entity's fragment body
    /// ids, removes/destroys them via the BodyInterface, decrements the body count + broadphase churn, then
    /// EMPTIES the fragment's body-id array (that emptiness is the idempotence guard). Called by every
    /// removal path AND by FProcessor_JoltStaticActor_EndPlay when the entity is destroyed first — whichever
    /// runs first frees the bodies; the other sees an empty array and no-ops. Does NOT destroy the entity.
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

    auto
    DoAdd_BodiesForLevel(
        ULevel& InLevel) -> void;

    auto
    DoRemove_BodiesForLevel(
        ULevel& InLevel) -> void;

    auto
    DoAdd_BodiesForLevel_LiveExtract(
        ULevel& InLevel,
        const FCk_Handle& InTransientEntity,
        TArray<FCk_Handle_JoltStaticActor>& OutActorEntities,
        TArray<uint32>& OutBodyIdsForBatch) -> void;

    auto
    DoAdd_BodiesForLevel_Cooked(
        ULevel& InLevel,
        const FCk_Handle& InTransientEntity,
        TArray<FCk_Handle_JoltStaticActor>& OutActorEntities,
        TArray<uint32>& OutBodyIdsForBatch,
        TArray<int32>& OutCellIndices) -> void;

    // Creates the attribution entity for a contributing actor (transient-owned), adds the fragment, caches
    // the source actor + name, and stamps its DebugName. Invalid return = the transient entity was not ready.
    auto
    DoCreate_ActorEntity(
        const FCk_Handle& InTransientEntity,
        const AActor& InSourceActor) -> FCk_Handle_JoltStaticActor;

    // Creates the extracted bodies, stamps each with InActorEntity's id as Jolt user-data, and appends the
    // raw body ids to BOTH the entity's fragment and the batch-add accumulator.
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

    // The registry's transient entity (attribution entities are parented under it). Invalid until the ECS
    // world is ready — the caller SKIPS the level and the OnWorldBeginPlay sweep re-attempts it.
    auto
    DoGet_TransientEntity() const -> FCk_Handle;

    // Resolves a body's Jolt user-data into a live handle, registry-liveness-check FIRST (no ensure on dead
    // ids) — mirrors CkJoltQuery_Utils::TryResolve_Entity. Invalid when the id is 0 or dead.
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
