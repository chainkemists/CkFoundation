#pragma once

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment_Data.h"

#include <Engine/DataAsset.h>
#include <GameplayTagContainer.h>

#include "Ck2dGridSystem_Spec.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// One blocker footprint authored on a grid Spec. Mirrors the runtime blocker params
// (FCk_Fragment_2dGridBlocker_ParamsData) which stamps the inclusive coordinate rectangle
// [_RangeMin, _RangeMax] as Disabled cells. The EntityScript applies these post-Add by
// spawning a blocker entity per entry.
USTRUCT(BlueprintType)
struct CKGRID_API FCk_2dGridSystem_Spec_Blocker
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Grid")
    FIntPoint RangeMin = FIntPoint::ZeroValue;

    UPROPERTY(EditAnywhere, Category = "Grid")
    FIntPoint RangeMax = FIntPoint::ZeroValue;
};

// --------------------------------------------------------------------------------------------------------------------

// Data-driven authoring layer for a single 2d grid. Edited by the editor paint tool (later tasks)
// and consumed at runtime by UCk_2dGridSystem_EntityScript. Resolve_GridParams() folds the
// grid-level fields into the runtime params fragment; PerCellTags and Blockers are applied by the
// EntityScript AFTER the grid is added (they are not part of the params fragment).
UCLASS(BlueprintType, Category = "Ck|2dGridSystem")
class CKGRID_API UCk_2dGridSystem_Spec : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Grid")
    FIntPoint Dimensions = FIntPoint(10, 10);

    UPROPERTY(EditAnywhere, Category = "Grid")
    FVector2D CellSize = FVector2D(100, 100);

    // Coordinates that start Disabled. Become the params fragment's _ExceptionCoordinates
    // (because DefaultCellState is Enable).
    UPROPERTY(EditAnywhere, Category = "Grid")
    TArray<FIntPoint> DisabledCells;

    // Tags applied to EVERY cell of this grid by default.
    UPROPERTY(EditAnywhere, Category = "Grid", meta = (Categories = "2dGridCell"))
    FGameplayTagContainer DefaultCellTags;

    // Per-coordinate tag overrides applied to individual cells after the grid is built.
    UPROPERTY(EditAnywhere, Category = "Grid", meta = (Categories = "2dGridCell"))
    TMap<FIntPoint, FGameplayTagContainer> PerCellTags;

    // Blocker footprints stamped onto the grid after it is built.
    UPROPERTY(EditAnywhere, Category = "Grid")
    TArray<FCk_2dGridSystem_Spec_Blocker> Blockers;

public:
    // Builds the runtime grid params from the grid-level Spec fields. DisabledCells map to
    // _ExceptionCoordinates against a DefaultCellState of Enable. PerCellTags/Blockers are NOT
    // included — the EntityScript applies those post-Add.
    auto
    Resolve_GridParams() const -> FCk_Fragment_2dGridSystem_ParamsData;
};

// --------------------------------------------------------------------------------------------------------------------
