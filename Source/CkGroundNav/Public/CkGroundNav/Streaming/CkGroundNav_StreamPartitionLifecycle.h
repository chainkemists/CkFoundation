#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkGroundNav/Bake/CkGroundNav_DataLayerSelector.h"
#include "CkGroundNav/Facade/CkGroundNav_WorldFieldRegistry.h"

#include <CoreMinimal.h>

class UWorld;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav::stream_partitions
{
    /**
     * Durable identity of one generated World Partition source manifest. It deliberately identifies
     * a source, not a tile: a manifest may own several tile coordinates, while a tile remains
     * identified by { VolumeId, TileCoord } inside the registry transaction.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_StreamPartitionKey
    {
    public:
        FCk_GroundNav_VolumeId _VolumeId;
        int32 _PartitionId = INDEX_NONE;

    public:
        auto Get_IsValid() const -> bool
        {
            return _VolumeId.Get_IsStreamingValid() && _PartitionId > 0;
        }

        auto operator==(const FCk_GroundNav_StreamPartitionKey&) const -> bool = default;
    };

    CKGROUNDNAV_API auto GetTypeHash(const FCk_GroundNav_StreamPartitionKey& InKey) -> uint32;

    /**
     * A monotonically newer event for a partition. The manifest loader owns validation of the
     * all-profile payload; this service never reads assets or queries geometry.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_StreamPartitionLoad
    {
    public:
        FCk_GroundNav_StreamPartitionKey _Key;
        /** Opaque registration fence returned by Register_ManifestOwner for this owner lifetime. */
        uint64 _OwnerInstance = 0;
        uint64 _Generation = 0;
        TArray<FCk_GroundNav_StreamTileTransition> _Transitions;
    };

    enum class ECk_GroundNav_StreamPartitionLifecycleStatus : uint8
    {
        Published,
        NoChange,
        InvalidWorld,
        InvalidKey,
        InvalidOwner,
        InvalidOwnerInstance,
        InvalidGeneration,
        InvalidTransitions,
        DuplicateActivePartition,
        PartitionNotActive,
        StaleEvent,
        RegistryRefused
    };

    /** The registry result is preserved so callers can report its exact atomic refusal. */
    struct CKGROUNDNAV_API FCk_GroundNav_StreamPartitionLifecycleResult
    {
    public:
        ECk_GroundNav_StreamPartitionLifecycleStatus _Status =
            ECk_GroundNav_StreamPartitionLifecycleStatus::NoChange;
        world_fields::FCk_GroundNav_StreamRegistryResult _Registry;
        /** Valid only for a successful Register_ManifestOwner call. */
        uint64 _OwnerInstance = 0;

    public:
        auto Get_Succeeded() const -> bool
        {
            return _Status == ECk_GroundNav_StreamPartitionLifecycleStatus::Published ||
                   _Status == ECk_GroundNav_StreamPartitionLifecycleStatus::NoChange;
        }
    };

    /** The live owner fence a level-streaming consumer must carry into every partition event. */
    struct CKGROUNDNAV_API FCk_GroundNav_ManifestOwnerSnapshot
    {
    public:
        FCk_Handle _OwnerEntity;
        uint64 _OwnerInstance = 0;
        uint64 _DefaultFingerprint = 0;
        TMap<FGameplayTag, uint64> _VariantFingerprints;
        FCk_GroundNav_DataLayerSelector _DataLayerSelector;
        FName _SourceLevelPackage;
        FName _CookKey;

    public:
        auto Get_IsValid() const -> bool
        { return _OwnerInstance != 0 && _DefaultFingerprint != 0 && _DataLayerSelector.Get_IsCanonical(); }
    };

    /**
     * Game-thread lifecycle bridge for generated manifest sources. It owns only the world-local
     * partition-to-opaque-source association; publication, retained blobs, epochs, and all-profile
     * atomicity stay in world_fields. A rejected call changes neither sidecar.
     *
     * Engine-facing level and data-layer events are handled by the stream-manifest subsystem, which
     * calls Load/Deactivate/Reactivate/Unload only after it has decoded and validated an all-profile
     * generated manifest. Keeping those events out of this value service preserves deterministic
     * source identity and leaves lifecycle mutation behind the owner/generation fences below.
     */
    CKGROUNDNAV_API auto Register_ManifestOwner(
        UWorld*                                InWorld,
        const FCk_Handle&                      InOwnerEntity,
        FCk_GroundNav_VolumeId                 InVolumeId,
        const FCk_GroundNav_StreamFieldBundle& InInitialBundle,
        uint64                                 InDefaultFingerprint = 0,
        const TMap<FGameplayTag, uint64>&      InVariantFingerprints = {},
        const FCk_GroundNav_DataLayerSelector&  InDataLayerSelector = {},
        FName                                  InSourceLevelPackage = NAME_None,
        FName                                  InCookKey = NAME_None) -> FCk_GroundNav_StreamPartitionLifecycleResult;

    /**
     * Reads the currently registered manifest-owner lifetime. It is intentionally separate from
     * the field registry snapshot: a level can wait for both values before decoding its asset,
     * without exposing mutable lifecycle state to field readers.
     */
    CKGROUNDNAV_API auto TryGet_ManifestOwner(
        UWorld* InWorld,
        FCk_GroundNav_VolumeId InVolumeId) -> TOptional<FCk_GroundNav_ManifestOwnerSnapshot>;

    CKGROUNDNAV_API auto Load(
        UWorld*                              InWorld,
        const FCk_GroundNav_StreamPartitionLoad& InLoad) -> FCk_GroundNav_StreamPartitionLifecycleResult;

    /** Data-layer deactivation retains the registry's decoded blobs for zero-probe reactivation. */
    CKGROUNDNAV_API auto Deactivate(
        UWorld*                              InWorld,
        const FCk_GroundNav_StreamPartitionKey& InKey,
        uint64                               InOwnerInstance,
        uint64                               InGeneration) -> FCk_GroundNav_StreamPartitionLifecycleResult;

    CKGROUNDNAV_API auto Reactivate(
        UWorld*                              InWorld,
        const FCk_GroundNav_StreamPartitionKey& InKey,
        uint64                               InOwnerInstance,
        uint64                               InGeneration) -> FCk_GroundNav_StreamPartitionLifecycleResult;

    /** Runtime-cell unload drops the source, retained blobs, and opaque registry handle. */
    CKGROUNDNAV_API auto Unload(
        UWorld*                              InWorld,
        const FCk_GroundNav_StreamPartitionKey& InKey,
        uint64                               InOwnerInstance,
        uint64                               InGeneration) -> FCk_GroundNav_StreamPartitionLifecycleResult;

    /**
     * Owner teardown removes every active partition source in the same registry transaction that
     * unregisters the owner. It removes every partition state; the owner-instance fence remains
     * monotonic for the world lifetime, so a newly registered owner can restart at generation one
     * while late events from the prior owner lifetime still refuse.
     */
    CKGROUNDNAV_API auto Purge_OwnerPartitions(
        UWorld*                   InWorld,
        const FCk_Handle&         InOwnerEntity,
        FCk_GroundNav_VolumeId    InVolumeId) -> FCk_GroundNav_StreamPartitionLifecycleResult;
}

// --------------------------------------------------------------------------------------------------------------------
