#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt/CollisionLayers/CkJoltCollisionLayer_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UWorld;

// --------------------------------------------------------------------------------------------------------------------

/// The shared static-world cooker: extracts every static actor's collision (the same
/// ck::jolt::bake extraction the live PIE path uses — one extraction path, no divergence),
/// partitions actor groups into bake-grid cells, serializes deduped shape blobs
/// (Shape::SaveWithChildren, one shared ShapeToIDMap per cell), and saves the per-cell +
/// index data assets under the configured cooked-data root.
///
/// Consumed by UCk_JoltCook_EditorSubsystem_UE (primary: world already booted by the editor)
/// and UCk_JoltCook_Commandlet (headless; see its header for the world-boot caveats).
class CKJOLTEDITOR_API FCk_Jolt_WorldCooker
{
public:
    CK_GENERATED_BODY(FCk_Jolt_WorldCooker);

    struct FCookStats
    {
        int32 _NumActors = 0;
        int32 _NumBodies = 0;
        int32 _NumCells = 0;
        int32 _NumUniqueShapes = 0;
        int32 _NumActorsUpToDate = 0;
        bool _Success = false;
    };

public:
    /// Cooks InWorld's currently-loaded static actors. For World Partition maps, unloaded
    /// actors are visited in batches via FWorldPartitionHelpers::ForEachActorWithLoading.
    /// Pass DryRun to extract + report without saving assets.
    static auto
    Cook_World(
        UWorld& InWorld,
        bool InDryRun = false) -> FCookStats;

    /// Recomputes every relevant actor's source hash against the existing cooked index and
    /// reports stale/missing/up-to-date counts without writing anything.
    static auto
    Validate_World(
        UWorld& InWorld) -> FCookStats;
};

// --------------------------------------------------------------------------------------------------------------------
