#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include <CoreMinimal.h>

#include "CkGroundNav_BakeTypes.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Value types shared by every bake stage. Deliberately free of world, registry and entity concepts:
// the whole bake is math over these, and the ECS shell above it only owns the result.
// --------------------------------------------------------------------------------------------------------------------

/**
 * How a bake stage ended. Failure is ALWAYS one of these — never an empty field silently published
 * as if it were built, which is the one outcome that would make a hole indistinguishable from a floor.
 */
UENUM(BlueprintType)
enum class ECk_GroundNav_BakeStatus : uint8
{
    // The stage produced its whole output.
    Completed,

    // The stage ran out of its probe budget mid-way and recorded a resume point. Nothing is published.
    BudgetExhausted,

    // The geometry backend could not answer. The tile stays Unbuilt; it is NOT an empty built tile.
    BackendUnavailable,

    // The inputs were rejected at admission (an invalid agent profile, a degenerate config).
    InvalidInput,

    // The stage exceeded a hard structural limit (column count over the tile budget).
    LimitExceeded,

    // The geometry backend's world changed while a sliced build was in flight. Tiles baked on
    // different frames against different worlds would disagree about their shared seam columns and
    // the seam portal between them would silently vanish - the field would then read BLOCKED where
    // the truth is "not built against this world". The build fails closed instead; nothing is
    // published, and the caller rebuilds. Appended last so the earlier values keep their indices.
    StaleGeometry
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GroundNav_BakeStatus);

// --------------------------------------------------------------------------------------------------------------------

/**
 * The outcome of one bake stage: a status plus the counters the tests assert on. Probe count is the
 * PRIMARY budget and the primary assertion — it is deterministic for a given fixture and config,
 * where wall time is not.
 *
 * ONE PROBE IS ONE INNERMOST CELL OR SPAN READ. Every stage bills its dominant loop in that unit and
 * nothing else: a triangle-to-cell clip, a neighbour-span visit, a chamfer relax read, a plate
 * surface scan, a portal candidate read. The unit is the same in every stage so the per-tile sum is
 * a cost a budget can be denominated in, and every loop that is billed iterates in index order over
 * value arrays, so the count is exactly reproducible. A stage that bills something cheaper than
 * its dominant loop (a triangle count, a plate count) under-reports by a factor that grows with
 * column depth or layer count, which is precisely the property a budget cannot tolerate.
 */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_GroundNav_BakeStageResult
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_GroundNav_BakeStageResult);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_GroundNav_BakeStatus _Status = ECk_GroundNav_BakeStatus::Completed;

    // Probes actually spent, in the unit defined above. The budgeting contract is asserted against
    // this, never against time.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _ProbesSpent = 0;

    // Inputs the stage refused and counted rather than processing: degenerate or non-finite geometry.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _DroppedInputCount = 0;

public:
    CK_PROPERTY(_Status);
    CK_PROPERTY(_ProbesSpent);
    CK_PROPERTY(_DroppedInputCount);

public:
    auto Get_IsCompleted() const -> bool { return _Status == ECk_GroundNav_BakeStatus::Completed; }

public:
    CK_DEFINE_CONSTRUCTORS(FCk_GroundNav_BakeStageResult, _Status);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * The resolution and extent knobs one bake runs at.
 *
 * These are plain floats in unreal units by deliberate choice, matching the navigation project
 * settings precedent: the never-bare-floats doctrine governs AUTHORED GAMEPLAY SHAPES, and a bake
 * resolution is neither. The authored shape on the agent profile beside this IS a shape type.
 */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_GroundNav_BakeConfig
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_GroundNav_BakeConfig);

private:
    // Horizontal size of one finest-resolution cell.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _CellSizeUu = 25.0f;

    // Vertical quantum. Two surfaces closer than this in one column are one span.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _CellHeightUu = 10.0f;

    // Edge length of one square tile. Tiles are the unit of identity, epoch, repair and streaming.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _TileSizeUu = 1600.0f;

    // Hard ceiling on columns per tile. Exceeding it fails the tile with LimitExceeded rather than
    // letting a mis-configured cell size allocate without bound.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _MaxColumnsPerTile = 262144;

public:
    CK_PROPERTY_GET(_CellSizeUu);
    CK_PROPERTY_GET(_CellHeightUu);
    CK_PROPERTY(_TileSizeUu);
    CK_PROPERTY_GET(_MaxColumnsPerTile);

public:
    auto Get_IsValid() const -> bool;

public:
    CK_DEFINE_CONSTRUCTORS(FCk_GroundNav_BakeConfig, _CellSizeUu, _CellHeightUu);
};

// --------------------------------------------------------------------------------------------------------------------
