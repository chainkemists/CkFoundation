#pragma once

#include "CkGroundNav/Bake/CkGroundNav_DataLayerSelector.h"
#include "CkGroundNav/Cook/CkGroundNav_CookedSourceManifest.h"

#include <GameFramework/Info.h>

#include "CkGroundNav_StreamManifestBinding_UE.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * The explicit runtime-cell-to-manifest association. World Partition supplies only the ULevel
 * lifetime; authored IDs, selector, and tile ownership keep that engine event from guessing either
 * GroundNav identity or bake inputs. The active volume owns the authoritative fingerprints.
 */
UCLASS(BlueprintType)
class CKGROUNDNAV_API ACk_GroundNav_StreamManifestBinding_UE : public AInfo
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "GroundNav", meta = (ClampMin = "1"))
    int32 _StreamingVolumeId = INDEX_NONE;

    UPROPERTY(EditAnywhere, Category = "GroundNav", meta = (ClampMin = "1"))
    int32 _PartitionId = INDEX_NONE;

    /** Must already be canonical: lexical order, no duplicates, and no None values. */
    UPROPERTY(EditAnywhere, Category = "GroundNav")
    TArray<FName> _DataLayerNames;

    /** Authoring ownership, strictly row-major and nonempty. The cooker turns these into one manifest. */
    UPROPERTY(EditAnywhere, Category = "GroundNav")
    TArray<FIntPoint> _TileCoords;

    /** Generated cooked asset for this exact {VolumeId, PartitionId, selector} source. */
    UPROPERTY(EditAnywhere, Category = "GroundNav")
    TSoftObjectPtr<UCk_GroundNav_CookedSourceManifest_UE> _Manifest;

public:
    auto TryGet_DataLayerSelector(ck::groundnav::FCk_GroundNav_DataLayerSelector& OutSelector) const -> bool;
    auto Get_HasCanonicalTileCoords() const -> bool;
    auto Get_HasValidIdentity() const -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
