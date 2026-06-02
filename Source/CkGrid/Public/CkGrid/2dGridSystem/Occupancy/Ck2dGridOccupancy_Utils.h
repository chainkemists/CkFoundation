#pragma once

#include "Ck2dGridOccupancy_Fragment.h"

#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment_Data.h"
#include "CkGrid/2dGridSystem/Placement/Ck2dGridPlacement_Fragment_Data.h"

#include "Ck2dGridOccupancy_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Raw occupancy layer: NO validation (no bounds / disabled / overlap checks). Stamps the
// requested cells unconditionally and reconciles them via the StampCells processor. A higher
// "placement" layer is expected to validate before calling Request_AddPlacement.
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_2dGridSystem"))
class CKGRID_API UCk_Utils_2dGridOccupancy_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_2dGridOccupancy_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridOccupancy",
              DisplayName="[Ck][2dGridOccupancy] Request Add Placement")
    static FCk_Handle_2dGridPlacement
    Request_AddPlacement(
        UPARAM(ref) FCk_Handle_2dGridSystem& InGrid,
        const FCk_Handle& InOccupant,
        const FIntPoint& InAnchor,
        ECk_CardinalRotation InRotation,
        const TArray<FIntPoint>& InCells);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridOccupancy",
              DisplayName="[Ck][2dGridOccupancy] Request Remove Placement")
    static bool
    Request_RemovePlacement(
        UPARAM(ref) FCk_Handle_2dGridPlacement& InPlacement);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridOccupancy",
              DisplayName="[Ck][2dGridOccupancy] Get Placements")
    static TArray<FCk_Handle_2dGridPlacement>
    Get_Placements(
        const FCk_Handle_2dGridSystem& InGrid);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridOccupancy",
              DisplayName="[Ck][2dGridOccupancy] Get Is Occupied")
    static bool
    Get_IsOccupied(
        const FCk_Handle_2dGridSystem& InGrid,
        const FIntPoint& InCoordinate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridOccupancy",
              DisplayName="[Ck][2dGridOccupancy] Get Occupant At")
    static FCk_Handle
    Get_OccupantAt(
        const FCk_Handle_2dGridSystem& InGrid,
        const FIntPoint& InCoordinate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridOccupancy",
              DisplayName="[Ck][2dGridOccupancy] Get Placement At")
    static FCk_Handle_2dGridPlacement
    Get_PlacementAt(
        const FCk_Handle_2dGridSystem& InGrid,
        const FIntPoint& InCoordinate);

private:
    // Death-watch handler bound on the OCCUPANT entity. When the occupant is destroyed,
    // reads its back-ref fragment and destroys the placement it owns. Non-static: dynamic
    // delegates bind to a UObject instance (the CDO here).
    UFUNCTION()
    void
    OnOccupantBeginDestroy(
        FCk_Handle InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
