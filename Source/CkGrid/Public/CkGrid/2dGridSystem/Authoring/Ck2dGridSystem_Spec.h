#pragma once

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment_Data.h"

#include <Engine/DataAsset.h>
#include <GameplayTagContainer.h>

#include "Ck2dGridSystem_Spec.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// One blocker footprint authored on a grid Spec: the inclusive coordinate rectangle
// [RangeMin, RangeMax] is stamped as Disabled cells.
USTRUCT(BlueprintType)
struct CKGRID_API FCk_2dGridSystem_Spec_Blocker
{
    GENERATED_BODY()

public:
    // Optional label so gameplay can look this blocker up via Get_BlockerWithTag and toggle it
    // via Request_SetActive. Empty = anonymous.
    UPROPERTY(EditAnywhere, Category = "Grid", meta = (Categories = "Grid.Blocker"))
    FGameplayTag Name;

    UPROPERTY(EditAnywhere, Category = "Grid")
    FIntPoint RangeMin = FIntPoint::ZeroValue;

    UPROPERTY(EditAnywhere, Category = "Grid")
    FIntPoint RangeMax = FIntPoint::ZeroValue;
};

// --------------------------------------------------------------------------------------------------------------------

// Data-driven authoring layer for a single 2d grid, consumed at runtime by
// UCk_2dGridSystem_EntityScript.
UCLASS(BlueprintType, Category = "Ck|2dGridSystem")
class CKGRID_API UCk_2dGridSystem_AuthoringSpec : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Grid")
    FIntPoint Dimensions = FIntPoint(10, 10);

    UPROPERTY(EditAnywhere, Category = "Grid")
    FVector2D CellSize = FVector2D(100, 100);

    // Coordinates that start Disabled.
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
    // Grid-level fields only: PerCellTags and Blockers are NOT included — the EntityScript
    // applies those after the grid is added.
    auto
    Resolve_GridParams() const -> FCk_2dGridSystem_Spec;
};

// --------------------------------------------------------------------------------------------------------------------
