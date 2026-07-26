#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>

#include "CkPathNetwork_Types.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Where a ribbon came from. Generated ribbons are rebuilt when the detector re-runs over their
// region; Authored ribbons are only ever touched by hand (a Generated ribbon a designer edits is
// promoted to Authored by the editor tool so a re-detect can't clobber the edit).
UENUM(BlueprintType)
enum class ECk_PathNetwork_RibbonSource : uint8
{
    Authored,
    Generated
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PathNetwork_RibbonSource);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPATHNETWORK_API FCk_PathNetwork_RibbonPoint
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_PathNetwork_RibbonPoint);

private:
    // MakeEditWidget draws a draggable translate widget in the level viewport and interprets the
    // vector in ACTOR-LOCAL space — which is why ACk_PathNetwork_UE stores ribbons actor-relative
    // and converts them to world space at entity-construction time.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, MakeEditWidget=true))
    FVector _Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _HalfWidth = 100.0f;

public:
    CK_PROPERTY(_Location);
    CK_PROPERTY(_HalfWidth);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_PathNetwork_RibbonPoint, _Location, _HalfWidth);
};

// --------------------------------------------------------------------------------------------------------------------

// One sidewalk span: centerline polyline + per-point half-widths. Authored and detector-generated
// ribbons are the SAME type, which is what keeps auto-built results hand-editable. Junctions are
// never authored — the builder snaps endpoints (_NodeSnapRadius) and splits T-junctions.
USTRUCT(BlueprintType)
struct CKPATHNETWORK_API FCk_PathNetwork_Ribbon
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_PathNetwork_Ribbon);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    TArray<FCk_PathNetwork_RibbonPoint> _Points;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    ECk_PathNetwork_RibbonSource _Source = ECk_PathNetwork_RibbonSource::Authored;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FGuid _RibbonId;

public:
    CK_PROPERTY(_Points);
    CK_PROPERTY(_Source);
    CK_PROPERTY(_RibbonId);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_PathNetwork_Ribbon, _Points);
};

// --------------------------------------------------------------------------------------------------------------------

// Raster occupancy from a per-game UCk_PathNetwork_Detector_UE. Row-major _SizeX * _SizeY cells;
// cell (X,Y) covers the square at _Origin + (X,Y)*_CellSize; _Occupancy 0 = off-path. _Heights is
// optional (empty = flat at _Origin.Z) and, when present, must match _Occupancy in length.
USTRUCT(BlueprintType)
struct CKPATHNETWORK_API FCk_PathNetwork_DetectionMask
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_PathNetwork_DetectionMask);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    FVector _Origin = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _CellSize = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0"))
    int32 _SizeX = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0"))
    int32 _SizeY = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    TArray<uint8> _Occupancy;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    TArray<float> _Heights;

public:
    CK_PROPERTY(_Origin);
    CK_PROPERTY(_CellSize);
    CK_PROPERTY(_SizeX);
    CK_PROPERTY(_SizeY);
    CK_PROPERTY(_Occupancy);
    CK_PROPERTY(_Heights);

public:
    auto Get_IsValidMask() const -> bool;
    auto Get_IsOccupied(int32 InX, int32 InY) const -> bool;
    auto Get_CellWorldLocation(int32 InX, int32 InY) const -> FVector;

public:
    CK_DEFINE_CONSTRUCTORS(FCk_PathNetwork_DetectionMask, _Origin, _CellSize, _SizeX, _SizeY);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPATHNETWORK_API FCk_PathNetwork_VectorizeParams
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_PathNetwork_VectorizeParams);

private:
    // Floor for the half-width estimated from the distance transform. Cells at the mask border of
    // a painted region legitimately measure near-zero; this keeps ribbons usable there.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _MinHalfWidth = 50.0f;

    // Douglas-Peucker tolerance in cm. Higher = fewer centerline points, straighter ribbons.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0.0"))
    float _SimplifyTolerance = 25.0f;

    // Dead-end skeleton chains shorter than this are dropped as noise (paint specks, thinning
    // whiskers). Chains connecting two junctions are structural and always kept.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="0.0"))
    float _MinRibbonLength = 200.0f;

public:
    CK_PROPERTY(_MinHalfWidth);
    CK_PROPERTY(_SimplifyTolerance);
    CK_PROPERTY(_MinRibbonLength);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPATHNETWORK_API FCk_PathNetwork_BuildParams
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_PathNetwork_BuildParams);

private:
    // Ribbon endpoints within this radius of each other (or of another ribbon's interior — a
    // T-junction) fuse into one network node.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="1.0"))
    float _NodeSnapRadius = 150.0f;

    // Spatial-index cell size in cm. Also the granularity of rebuild versioning: a corridor is
    // invalidated when any chunk it crosses is rebuilt.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true, ClampMin="100.0"))
    float _ChunkSize = 3200.0f;

public:
    CK_PROPERTY(_NodeSnapRadius);
    CK_PROPERTY(_ChunkSize);
};

// --------------------------------------------------------------------------------------------------------------------
