#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <Engine/DataAsset.h>

#include "CkGroundNav_CookedTile.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * The lattice a cooked tile was produced against, as VALUES.
 *
 * Carried on every tile rather than only on the collection it belongs to, because a tile asset is
 * loadable on its own: a tile read into a field divided differently would lay its cells over ground
 * it was never baked from, and every index it carries would name something else. Comparing the key
 * is what turns that into a refusal instead of a corrupt field.
 */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_GroundNav_CookedLatticeKey
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GroundNav_CookedLatticeKey);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FVector2D _OriginXY = FVector2D::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FIntPoint _Divisions = FIntPoint{1, 1};

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _MinZUu = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _MaxZUu = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _TileSizeUu = 1600.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _CellSizeUu = 25.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _CellHeightUu = 10.0f;

public:
    CK_PROPERTY_GET(_OriginXY);
    CK_PROPERTY_SET(_OriginXY);

    CK_PROPERTY_GET(_Divisions);
    CK_PROPERTY_SET(_Divisions);

    CK_PROPERTY_GET(_MinZUu);
    CK_PROPERTY_SET(_MinZUu);

    CK_PROPERTY_GET(_MaxZUu);
    CK_PROPERTY_SET(_MaxZUu);

    CK_PROPERTY_GET(_TileSizeUu);
    CK_PROPERTY_SET(_TileSizeUu);

    CK_PROPERTY_GET(_CellSizeUu);
    CK_PROPERTY_SET(_CellSizeUu);

    CK_PROPERTY_GET(_CellHeightUu);
    CK_PROPERTY_SET(_CellHeightUu);

public:
    auto operator==(const FCk_GroundNav_CookedLatticeKey&) const -> bool = default;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * One cooked tile of one ground-nav field: the tile blob, and enough identity to refuse it.
 *
 * READ-ONLY IN THE DETAILS PANEL, not read-only on disk: the cook writes every one of these through
 * the setters below, the same way CkJolt's cooked cell is written. What VisibleAnywhere buys is that
 * a HUMAN cannot edit one - the blob is the answer and the fields beside it only say what the blob
 * is, so an edited coord, lattice or fingerprint would disagree with the bytes it sits next to and
 * nothing downstream could tell.
 */
UCLASS()
class CKGROUNDNAV_API UCk_GroundNav_CookedTile_UE : public UDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GroundNav_CookedTile_UE);

private:
    /** The blob format the bytes below were written under. A reader speaking another one refuses the
     *  asset rather than reinterpreting it. */
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    int32 _FormatVersion = 0;

    /** Where this tile sits in the lattice. An FIntPoint rather than the field's own coord type,
     *  which is a plain value struct outside reflection: X is FCk_GroundNav_TileCoord::_X and Y is
     *  its _Y, and nothing else about the two differs. */
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FIntPoint _TileCoord = FIntPoint::ZeroValue;

    /** The content fingerprint of the bake this tile came out of, compared against the fingerprint of
     *  the CURRENT inputs: a tile cooked under inputs that have since moved describes other ground. */
    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    uint64 _Fingerprint = 0;

    UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = true))
    FCk_GroundNav_CookedLatticeKey _LatticeKey;

    UPROPERTY()
    TArray<uint8> _Blob;

public:
    CK_PROPERTY_GET(_FormatVersion);
    CK_PROPERTY_SET(_FormatVersion);

    CK_PROPERTY_GET(_TileCoord);
    CK_PROPERTY_SET(_TileCoord);

    CK_PROPERTY_GET(_Fingerprint);
    CK_PROPERTY_SET(_Fingerprint);

    CK_PROPERTY_GET(_LatticeKey);
    CK_PROPERTY_SET(_LatticeKey);

    CK_PROPERTY_GET(_Blob);
    CK_PROPERTY_SET(_Blob);

public:
    /** Whether a reader speaking InFormatVersion may read this asset's blob. EXACT equality, never a
     *  range: a format the reader does not speak is refused, and a refusal is the only safe answer
     *  when the alternative is reading a differently-shaped record as if it were this one. */
    auto Get_IsCompatibleWith(int32 InFormatVersion) const -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
