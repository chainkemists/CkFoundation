#pragma once

#include "CkGroundNav/Bake/CkGroundNav_DataLayerSelector.h"
#include "CkGroundNav/Cook/CkGroundNav_CookedFieldIndex.h"
#include "CkGroundNav/Volume/CkGroundNavVolume_Fragment_Data.h"

#include <Containers/Array.h>

namespace ck::groundnav { class ICk_GroundNav_GeometryBackend; }

class UWorld;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav::cook
{
    enum class ECk_GroundNav_CookMode : uint8
    {
        Cook,
        DryRun
    };

    struct FCk_GroundNav_CookTilePlan
    {
        FIntPoint _Coord = FIntPoint::ZeroValue;
        FBox _WorldBounds = FBox{ForceInit};
        TArray<uint8> _Blob;
    };

    struct FCk_GroundNav_CookFieldPlan
    {
        FName _SourceLevelPackage;
        FName _CookKey;
        int32 _StreamingVolumeId = INDEX_NONE;
        FCk_GroundNav_DataLayerSelector _DataLayerSelector;
        FGameplayTag _ProfileTag;
        uint64 _Fingerprint = 0;
        FCk_GroundNav_CookedLatticeKey _LatticeKey;
        TArray<uint8> _SerializedField;
        TArray<FCk_GroundNav_CookTilePlan> _Tiles;
    };

    struct FCk_GroundNav_CookFieldStats
    {
        int32 _NumFields = 0;
        int32 _NumTiles = 0;
        int32 _NumAssetsWritten = 0;
        bool _Success = false;
    };

    struct FCk_GroundNav_CookIdentity
    {
        FName _SourceLevelPackage;
        FName _CookKey;
        int32 _StreamingVolumeId = INDEX_NONE;
        FCk_GroundNav_DataLayerSelector _DataLayerSelector;
    };

    /**
     * The durable ownership input for one source-manifest asset. The caller supplies this from a
     * World Partition catalog; the cooker deliberately does not derive a partition id from a cell
     * name or a transient editor object.
     */
    struct FCk_GroundNav_CookSourcePartition
    {
        int32 _StreamingVolumeId = INDEX_NONE;
        int32 _PartitionId = INDEX_NONE;
        FCk_GroundNav_DataLayerSelector _DataLayerSelector;
        TArray<FIntPoint> _TileCoords;
    };

    /** A fully resolved, deterministic source-manifest save request. */
    struct FCk_GroundNav_CookSourceManifestPlan
    {
        FName _SourceLevelPackage;
        FName _CookKey;
        int32 _StreamingVolumeId = INDEX_NONE;
        int32 _PartitionId = INDEX_NONE;
        FCk_GroundNav_DataLayerSelector _DataLayerSelector;
        FCk_GroundNav_CookedLatticeKey _LatticeKey;
        TArray<FIntPoint> _TileCoords;
        TArray<FGameplayTag> _ProfileTags;
        TArray<uint64> _ProfileFingerprints;
        TArray<TArray<FString>> _ProfileTilePaths;
        FString _AssetPath;
    };

    /// Pure bake plus optional asset persistence. The commandlet and the hermetic driver both call this
    /// one implementation; DryRun executes the identical bake and serialization path, then stops before
    /// package creation or writes.
    class CKGROUNDNAVEDITOR_API FCk_GroundNav_FieldCooker
    {
    public:
        /// Ensures the editor world's Jolt static geometry exists before any pure bake reads it.
        static auto
        Prepare_WorldGeometry(
            UWorld& InWorld)
            -> bool;

        static auto
        Get_AreCookIdentitiesUnique(
            TConstArrayView<FCk_GroundNav_CookIdentity> InIdentities)
            -> bool;

        static auto
        Cook_Volume(
            UWorld&                                        InWorld,
            const FCk_Fragment_GroundNavVolume_ParamsData& InParams,
            FName                                          InSourceLevelPackage,
            ECk_GroundNav_CookMode                         InMode,
            TArray<FCk_GroundNav_CookFieldPlan>&           OutPlans,
            const ICk_GroundNav_GeometryBackend*          InGeometryBackend = nullptr)
            -> FCk_GroundNav_CookFieldStats;

        /// Persists a plan made by Cook_Volume. Kept separate so a commandlet can preflight every
        /// volume before creating any package, then write those exact already-baked bytes once.
        static auto
        Save_Plans(
            TConstArrayView<FCk_GroundNav_CookFieldPlan> InPlans)
            -> FCk_GroundNav_CookFieldStats;

        /**
         * Resolves explicit World Partition ownership inputs into deterministic manifest save plans.
         * Every partition is checked against the complete default-plus-variant field-plan set before
         * OutPlans changes. Returned plans are ordered by partition id; coordinates must be sorted
         * and cannot be claimed by two partitions.
         */
        static auto
        Try_BuildSourceManifestPlans(
            TConstArrayView<FCk_GroundNav_CookFieldPlan>     InFieldPlans,
            TConstArrayView<FCk_GroundNav_CookSourcePartition> InPartitions,
            TArray<FCk_GroundNav_CookSourceManifestPlan>&    OutPlans)
            -> bool;

        /**
         * Persists already-resolved manifest plans only after every referenced tile package already
         * exists on disk. Call this after a successful Save_Plans for the exact field-plan batch;
         * it deliberately refuses to publish a manifest that can dangle from its tile assets.
         */
        static auto
        Save_SourceManifestPlans(
            TConstArrayView<FCk_GroundNav_CookSourceManifestPlan> InPlans)
            -> bool;
    };
}

// --------------------------------------------------------------------------------------------------------------------
