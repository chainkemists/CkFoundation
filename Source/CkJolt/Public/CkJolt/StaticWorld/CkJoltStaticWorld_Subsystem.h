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
class UPrimitiveComponent;

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

    /// Normalizes a package name into the form the COOK recorded. PIE renames every level package to
    /// /Game/Path/UEDPIE_<N>_Name; the cook only ever runs on non-PIE worlds, so a raw PIE name matches
    /// nothing and the cooked path silently degrades to a live sweep. Everything that looks cooked data
    /// up by package name goes through here.
    CKJOLT_API auto Get_PackageLookupKey(const FString& InPackageName) -> FName;

    /// The key a level's cooked actor table is filed under in the index. See Get_PackageLookupKey.
    CKJOLT_API auto Get_LevelLookupKey(const ULevel& InLevel) -> FName;
}

// --------------------------------------------------------------------------------------------------------------------

/*
 * Owns the baked static-world bodies, tracked per-ULevel so they add/remove in lockstep with level
 * streaming (World Partition runtime cells stream as ULevels, so one delegate pair covers WP and legacy
 * sublevels uniformly). Stale cooked data (version or per-actor hash mismatch) is ENSURED loudly and
 * skipped — never silently used, never re-extracted at runtime. See CkJolt/CLAUDE.md § Static world.
 *
 * Collision sync: every tracked source component's OnComponentCollisionSettingsChangedEvent is bound at
 * bake time, so SetActorEnableCollision / SetCollisionEnabled flips the attribution entity's bodies out
 * of (and back into) the scene automatically — the Jolt static world tracks engine collision state with
 * no game-code Jolt call. Visibility is deliberately NOT a sync key: hidden-but-solid is a legitimate
 * authored state.
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

protected:
    // Follows UCk_Jolt_Subsystem into Editor worlds when ECk_Jolt_EditorStaticWorldMode is on — the static
    // world IS the geometry an editor-time or cook-time consumer reads.
    auto DoesSupportWorldType(const EWorldType::Type InWorldType) const -> bool override;

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

    /// Runs this world's one-time initial sweep of its levels if it has not run yet, and does nothing on
    /// every later call. The lazy entry point for a consumer that needs static geometry in a world where
    /// OnWorldBeginPlay never fires — an Editor world, or the world a cook commandlet loads. Nothing sweeps
    /// a map merely because it was opened, so the cost lands on the consumer that asked for geometry.
    auto
    Request_EnsureSwept() -> void;

#if WITH_EDITOR
    /// The editor-world sibling of the actor-edit hooks, for the edits that broadcast none of them — undo
    /// and redo. Frees every tracked level's bodies and re-runs the sweep, so the static world is
    /// re-derived from the world as it now stands. Its cost is one full live extract, which is why it is
    /// not bound to anything finer.
    auto
    Request_ResweepAllLevels() -> void;
#endif

    /// Extracts and adds static bodies for a single actor at runtime; returns the number added.
    auto
    Request_BakeActor(
        const AActor& InActor) -> int32;

    /// Removes the bodies baked for this actor — by Request_BakeActor OR by the level sweep, which files
    /// every actor it bakes in the same index (so a request can reach level geometry, in Game/PIE as in
    /// the editor). A never-baked actor is a no-op.
    auto
    Request_RemoveActor(
        const AActor& InActor) -> void;

    /// Extracts and adds static bodies for a single PRIMITIVE COMPONENT at runtime — the surface for
    /// runtime-composed geometry (CkUnrealComponent-hosted ISMs and friends). ExplicitActor
    /// semantics: the bake filter is bypassed, the caller declared the geometry static-in-intent.
    /// Re-baking the same component REPLACES its previous bodies (call again after repopulating an
    /// ISM's instances). Returns the number of bodies added.
    auto
    Request_BakeComponent(
        const UPrimitiveComponent& InComponent) -> int32;

    /// Removes bodies previously added via Request_BakeComponent for this component.
    auto
    Request_RemoveComponent(
        const UPrimitiveComponent& InComponent) -> void;

    /// The single idempotent funnel for freeing a source actor's bodies, ending in EMPTYING the fragment's
    /// body-id array (that emptiness is the idempotence guard, since both the removal paths and
    /// FProcessor_JoltStaticActor_EndPlay call it). Also unbinds the entity's collision-sync routes.
    /// Does NOT destroy the entity.
    auto
    Request_RemoveBodiesForEntity(
        FCk_Handle_JoltStaticActor& InActorEntity) -> void;

private:
    // Collision-sync reconcile: routed from a tracked component's OnComponentCollisionSettingsChangedEvent.
    // Recomputes the desired in-scene state from engine collision truth and flips the entity's bodies when
    // it changed; a component-path entity re-BAKES on re-enable instead (its pose may be stale).
    UFUNCTION()
    void
    OnTrackedComponentCollisionSettingsChanged(
        UPrimitiveComponent* InChangedComponent);

private:
    // The bake path both Request_BakeActor and the editor authoring handlers land on. Policy and filter
    // belong to the CALLER: the public request declares ExplicitActor (static-in-intent, filter bypassed),
    // while an editor handler bakes through the SAME LevelSweep extraction it admitted the actor on —
    // re-extracting under ExplicitActor there would skip the mobility test and the component filters, so a
    // Movable door on an admitted actor would bake permanently.
    auto
    DoBake_Actor(
        const AActor& InActor,
        ck::jolt::bake::ECk_Jolt_ExtractionPolicy InPolicy,
        const ck::jolt::bake::FCk_Jolt_BakeFilter& InFilter) -> int32;

    auto
    DoHandle_LevelAdded(
        ULevel* InLevel,
        UWorld* InWorld) -> void;

    auto
    DoHandle_LevelRemoved(
        ULevel* InLevel,
        UWorld* InWorld) -> void;

#if WITH_EDITOR
    // Editor-world authoring sync. An Editor world never reaches OnWorldBeginPlay and never streams its
    // levels, so the actor edits themselves are the only thing that can keep the static world current.
    // A move is a remove + re-bake because a static body's pose is baked into the body.
    auto
    DoHandle_EditorActorAdded(
        AActor* InActor) -> void;

    auto
    DoHandle_EditorActorDeleted(
        AActor* InActor) -> void;

    auto
    DoHandle_EditorActorMoved(
        AActor* InActor) -> void;

    // World filter + the sweep's OWN admission rule (a LevelSweep extraction under the project bake filter
    // yielding at least one body) — never a second copy of the mobility/filter policy.
    auto
    DoGet_IsEditorSyncCandidate(
        const AActor* InActor) -> bool;

    // Two independent gates, because there are two independent settings — an Editor world answers to
    // _EditorStaticWorldMode ONLY. Shared by every entry point that may reach either world type.
    auto
    DoGet_IsStaticWorldEnabled(
        const UWorld& InWorld) const -> bool;
#endif

    // The world's initial sweep of every loaded level, and the single writer of _HasSwept. Reached from
    // OnWorldBeginPlay in a Game/PIE world and from Request_EnsureSwept everywhere else; each caller gates
    // on its OWN setting before calling, so neither setting can silently disable the other's world.
    auto
    DoRun_InitialSweep(
        UWorld& InWorld) -> void;

    // Returns the level's extraction stats so the BeginPlay sweep can report a per-world summary
    // (zeroed for the cooked path — its skips happened at cook time and are loud there).
    auto
    DoAdd_BodiesForLevel(
        ULevel& InLevel) -> ck::jolt::bake::FCk_Jolt_ExtractionStats;

    auto
    DoRemove_BodiesForLevel(
        ULevel& InLevel) -> void;

    // The two indexes are maintained together: _LevelBodies owns a level's teardown list, _ActorEntities
    // owns actor -> entity resolution for requests. A swept entity is in BOTH, so every path that adds or
    // frees one goes through these three.
    auto
    DoFile_ActorEntity(
        const FCk_Handle_JoltStaticActor& InActorEntity) -> void;

    // Drops the index entry only when it still points at THIS entity (a later re-bake owns its own entry).
    auto
    DoUnfile_ActorEntity(
        const FCk_Handle_JoltStaticActor& InActorEntity) -> void;

    // Adds a REQUEST-baked entity to its actor's level teardown list, so a sub-level unload frees it with
    // the rest of the level instead of leaving its bodies behind.
    auto
    DoList_EntityInLevel(
        const AActor& InActor,
        const FCk_Handle_JoltStaticActor& InActorEntity) -> void;

    // Drops the entity from its actor's level teardown list, so the level cannot re-visit an entity a
    // request already destroyed.
    auto
    DoUnlist_EntityFromLevel(
        const AActor& InActor,
        const FCk_Handle_JoltStaticActor& InActorEntity) -> void;

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

    auto
    DoCreate_ComponentEntity(
        const FCk_Handle& InTransientEntity,
        const UPrimitiveComponent& InSourceComponent) -> FCk_Handle_JoltStaticActor;

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

    // Binds the collision-sync event routes for a freshly created attribution entity: the actor overload
    // binds ALL of the source actor's primitives (the cooked path retains no per-component attribution,
    // and an unbaked component still participates in the desired-state OR); the component overload binds
    // the one source component and stamps the fragment's _SourceComponent.
    auto
    DoBind_CollisionSync(
        FCk_Handle_JoltStaticActor& InActorEntity,
        const AActor& InSourceActor) -> void;

    auto
    DoBind_CollisionSync(
        FCk_Handle_JoltStaticActor& InComponentEntity,
        const UPrimitiveComponent& InSourceComponent) -> void;

    auto
    DoBind_ComponentRoute(
        FCk_Handle_JoltStaticActor& InEntity,
        UPrimitiveComponent& InComponent) -> void;

    auto
    DoUnbind_CollisionSync(
        FCk_Handle_JoltStaticActor& InEntity) -> void;

    // Removes the entity's bodies from the broadphase WITHOUT destroying them (or re-adds them). The
    // caller owns the async-step guard (see OnTrackedComponentCollisionSettingsChanged).
    auto
    DoSet_BodiesInScene(
        FCk_Handle_JoltStaticActor& InEntity,
        bool InInScene) -> void;

    // Unset when every bound component is dead (source tearing down — EndPlay owns the free).
    auto
    DoGet_DesiredBodiesInScene(
        const FCk_Handle_JoltStaticActor& InEntity) const -> TOptional<bool>;

    // House rule for broadphase mutation outside the Jolt processors (see FProcessor_JoltStaticActor_EndPlay):
    // an in-flight async step is still READING the broadphase a flip mutates.
    auto
    DoWait_ForAsyncStepInFlight() -> void;

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

#if WITH_EDITOR
    FDelegateHandle _EditorActorAddedHandle;
    FDelegateHandle _EditorActorDeletedHandle;
    FDelegateHandle _EditorActorMovedHandle;
#endif

    ck::jolt::bake::FCk_Jolt_ShapeCache _LiveShapeCache;

    struct FLevelBodies
    {
        TArray<FCk_Handle_JoltStaticActor> _ActorEntities;
        TArray<int32> _CellIndices;

        // The sweep's "already visited this level" answer. It cannot be the mere PRESENCE of the entry:
        // a request bake files its entity here too, and a level whose only entries came that way still
        // owes its sweep.
        bool _Swept = false;
    };

    TMap<TWeakObjectPtr<ULevel>, FLevelBodies> _LevelBodies;

    // EVERY baked actor, sweep-baked and manually baked alike — the index Request_RemoveActor resolves
    // through. _LevelBodies keeps its own per-level list because level removal frees by level, not by actor.
    TMap<TWeakObjectPtr<const AActor>, FCk_Handle_JoltStaticActor> _ActorEntities;
    TMap<TWeakObjectPtr<const UPrimitiveComponent>, FCk_Handle_JoltStaticActor> _ManualComponentEntities;
    TMap<int32, FLoadedCell> _LoadedCells;

    // Collision-sync event routing: every bound source component -> its attribution entity. Populated by
    // DoBind_ComponentRoute, cleaned per-entity by DoUnbind_CollisionSync (weak keys hash by index+serial,
    // so removal works after the component dies).
    TMap<TWeakObjectPtr<UPrimitiveComponent>, FCk_Handle_JoltStaticActor> _ComponentEventRoutes;

    int32 _NumStaticBodies = 0;
    int32 _BodyChurnSinceOptimize = 0;
    bool _CookedIndexLoadAttempted = false;
    bool _HasSwept = false;

    // True only while DoRun_InitialSweep is walking the world's levels. _HasSwept is set at the HEAD of
    // that walk, so it alone would leave the add handler open for the sweep's whole duration and an actor
    // spawned into a not-yet-visited level would be baked twice.
    bool _IsSweeping = false;
};

// --------------------------------------------------------------------------------------------------------------------
