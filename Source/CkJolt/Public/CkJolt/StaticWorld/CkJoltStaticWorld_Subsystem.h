#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkJolt/StaticWorld/CkJoltBakeExtraction.h"
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
        FName _SourceActorName;
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
 * Owns the baked static-world bodies: thousands of Jolt bodies with NO ECS entities, tracked
 * per-ULevel so they add/remove in exact lockstep with level streaming (World Partition runtime
 * cells stream as ULevels, so one delegate pair covers WP and legacy sublevels uniformly).
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

    /// Ray against static-world bodies only (Static_World object layer). Test/introspection
    /// surface until the Phase-2 channel-filtered query API.
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
        TArray<uint32>& OutBodyIds) -> void;

    auto
    DoAdd_BodiesForLevel_Cooked(
        ULevel& InLevel,
        TArray<uint32>& OutBodyIds,
        TArray<int32>& OutCellIndices) -> void;

    auto
    DoCreate_BodiesFromExtracted(
        const TArray<ck::jolt::bake::FCk_Jolt_ExtractedBody>& InExtracted,
        FName InSourceActorName,
        TArray<uint32>& OutBodyIds) -> void;

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
    TObjectPtr<UCk_Jolt_CookedWorldIndex_UE> _CookedIndex;

    FDelegateHandle _LevelAddedHandle;
    FDelegateHandle _LevelRemovedHandle;

    ck::jolt::bake::FCk_Jolt_ShapeCache _LiveShapeCache;

    struct FLevelBodies
    {
        TArray<uint32> _BodyIds;
        TArray<int32> _CellIndices;
    };

    TMap<TWeakObjectPtr<ULevel>, FLevelBodies> _LevelBodies;
    TMap<TWeakObjectPtr<const AActor>, TArray<uint32>> _ManualActorBodies;
    TMap<int32, FLoadedCell> _LoadedCells;

    // Maps BodyID (index+sequence) -> source actor name, for hit attribution and tests.
    TMap<uint32, FName> _BodyToActorName;

    int32 _NumStaticBodies = 0;
    int32 _BodyChurnSinceOptimize = 0;
    bool _CookedIndexLoadAttempted = false;
};

// --------------------------------------------------------------------------------------------------------------------
