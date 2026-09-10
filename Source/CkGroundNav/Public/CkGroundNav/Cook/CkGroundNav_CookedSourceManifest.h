#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGroundNav/Bake/CkGroundNav_DataLayerSelector.h"
#include "CkGroundNav/Cook/CkGroundNav_CookedTile.h"

#include <CoreMinimal.h>
#include <Engine/DataAsset.h>
#include <GameplayTagContainer.h>

#include "CkGroundNav_CookedSourceManifest.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/** The cooked tiles for one profile inside one durable World Partition source contribution. */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_GroundNav_CookedSourceProfile_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GroundNav_CookedSourceProfile_UE);

private:
    /** Empty is the default profile. A valid tag names one exact variant. */
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FGameplayTag _ProfileTag;

    /** The input fingerprint for this profile's field. */
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    uint64 _Fingerprint = 0;

    /** One soft tile reference for every manifest coordinate, in that exact coordinate order. */
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    TArray<TSoftObjectPtr<UCk_GroundNav_CookedTile_UE>> _Tiles;

public:
    CK_PROPERTY_GET(_ProfileTag);
    CK_PROPERTY_SET(_ProfileTag);

    CK_PROPERTY_GET(_Fingerprint);
    CK_PROPERTY_SET(_Fingerprint);

    CK_PROPERTY_GET(_Tiles);
    CK_PROPERTY_SET(_Tiles);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * A durable, data-only World Partition source contribution for one streamed GroundNav volume.
 *
 * _PartitionId identifies the source manifest, never a field or a tile. Tile identity remains
 * {positive volume id, lattice coordinate}; the runtime registry assigns the ephemeral source handle.
 */
UCLASS()
class CKGROUNDNAV_API UCk_GroundNav_CookedSourceManifest_UE : public UDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GroundNav_CookedSourceManifest_UE);

private:
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    int32 _FormatVersion = 0;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    int32 _StreamingVolumeId = INDEX_NONE;

    /** Positive durable source-partition identity, scoped by _StreamingVolumeId. */
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    int32 _PartitionId = INDEX_NONE;

    /** Canonical selector used to collect the geometry in every profile below. */
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    TArray<FName> _DataLayerNames;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FCk_GroundNav_CookedLatticeKey _LatticeKey;

    /** Strict row-major coordinates, shared by every profile. */
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    TArray<FIntPoint> _TileCoords;

    /** Exactly one default entry plus one entry for every authored profile variant. */
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    TArray<FCk_GroundNav_CookedSourceProfile_UE> _Profiles;

public:
    CK_PROPERTY_GET(_FormatVersion);
    CK_PROPERTY_SET(_FormatVersion);

    CK_PROPERTY_GET(_StreamingVolumeId);
    CK_PROPERTY_SET(_StreamingVolumeId);

    CK_PROPERTY_GET(_PartitionId);
    CK_PROPERTY_SET(_PartitionId);

    CK_PROPERTY_GET(_DataLayerNames);
    CK_PROPERTY_SET(_DataLayerNames);

    CK_PROPERTY_GET(_LatticeKey);
    CK_PROPERTY_SET(_LatticeKey);

    CK_PROPERTY_GET(_TileCoords);
    CK_PROPERTY_SET(_TileCoords);

    CK_PROPERTY_GET(_Profiles);
    CK_PROPERTY_SET(_Profiles);

public:
    auto Get_IsCompatibleWith(int32 InFormatVersion) const -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
