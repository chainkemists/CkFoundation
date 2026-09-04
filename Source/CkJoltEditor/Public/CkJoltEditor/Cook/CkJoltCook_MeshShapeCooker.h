#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJoltEditor/Cook/CkJoltCook_Types.h"

#include <AssetRegistry/AssetData.h>

// --------------------------------------------------------------------------------------------------------------------

class UStaticMesh;

// --------------------------------------------------------------------------------------------------------------------

/// Per-mesh-asset shape cooker: writes one UCk_Jolt_CookedMeshShape_UE (scale-1 SaveWithChildren
/// blob) per mesh under the configured _BakedMeshShapeRoots whose collision is worth pre-baking
/// (contains a convex hull or cooked tri-mesh — pure-primitive collision is cheaper to rebuild at
/// runtime than to load). Incremental: an existing asset whose BodySetupGuid, trace flag, and
/// versions all match is skipped only after its current tri-mesh blob passes the winding audit. A
/// strongly inside-out current blob rebuilds from source so closed components can normalize; a corrupt
/// blob fails. Forced rebuilds run the same audit before replacing a current blob.
/// Orphaned shape assets (source mesh gone or no longer worth
/// baking) are LOGGED, not auto-deleted — same v1 policy as the map cook.
///
/// The sweep is decomposed so a caller can drive it across frames instead of blocking the editor:
/// Collect_Candidates (asset registry only, loads nothing) -> Cook_SingleMeshShape per item ->
/// Report_Orphans. Cook_MeshShapes composes the three for the commandlet and menu paths.
/// Consumed by UCk_JoltCook_EditorSubsystem_UE and UCk_JoltCook_Commandlet.
class CKJOLTEDITOR_API FCk_Jolt_MeshShapeCooker
{
public:
    CK_GENERATED_BODY(FCk_Jolt_MeshShapeCooker);

    struct FCookStats
    {
        int32 _NumMeshesConsidered = 0;
        int32 _NumShapesCooked = 0;
        int32 _NumUpToDate = 0;
        int32 _NumSkippedNotWorthBaking = 0;
        int32 _NumFailed = 0;
        int32 _NumOrphans = 0;
        bool _Success = false;
    };

public:
    /// The whole sweep, synchronously. Blocks for as long as it takes to LOAD every candidate mesh
    /// (BodySetupGuid is not an asset-registry tag, so staleness cannot be judged without loading);
    /// drive the decomposed API below instead when the editor must stay responsive.
    static auto
    Cook_MeshShapes(
        ck::jolt::cook::ECk_Jolt_CookMode InMode = ck::jolt::cook::ECk_Jolt_CookMode::Cook,
        bool InForceRebuild = false) -> FCookStats;

    /// Every static mesh under the configured _BakedMeshShapeRoots. Asset-registry only — no mesh is
    /// loaded, so this is safe to call before deciding whether the sweep is worth running.
    static auto
    Collect_Candidates() -> TArray<FAssetData>;

    /// Cooks ONE mesh's pre-baked shape — the unit both the sliced sweep and the on-save hook drive.
    /// OutCookedAssetPath receives the cooked asset's object path whenever one is in play (cooked,
    /// up to date, or a failed write) so a sliced caller can accumulate the in-use set for
    /// Report_Orphans. InForceRebuild bypasses the freshness shortcut for explicitly requested full rebakes.
    static auto
    Cook_SingleMeshShape(
        const UStaticMesh& InMesh,
        ck::jolt::cook::ECk_Jolt_CookMode InMode,
        FString* OutCookedAssetPath = nullptr,
        bool InForceRebuild = false) -> ck::jolt::cook::ECk_Jolt_MeshShapeCookResult;

    static auto
    Accumulate_SingleResult(
        ck::jolt::cook::ECk_Jolt_MeshShapeCookResult InResult,
        FCookStats& InOutStats) -> void;

    /// Warns about cooked shape assets no longer claimed by any candidate mesh. Returns the count.
    static auto
    Report_Orphans(
        const TSet<FString>& InCookedAssetPathsInUse) -> int32;

    static auto
    Log_CookStats(
        const FCookStats& InStats,
        ck::jolt::cook::ECk_Jolt_CookMode InMode) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
