#pragma once

#include "Ck2dGridSystem_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkEcsExt/SceneNode/CkSceneNode_Fragment_Data.h"

#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Fragment_Data.h"

#include "Ck2dGridSystem_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_2dGridSystem"))
class CKGRID_API UCk_Utils_2dGridSystem_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_2dGridSystem_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_2dGridSystem);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Add Feature")
    static FCk_Handle_2dGridSystem
    Add(
        UPARAM(ref) FCk_Handle_Transform& InHandle,
        const FCk_Fragment_2dGridSystem_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|2dGridSystem",
        DisplayName="[Ck][2dGridSystem] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_2dGridSystem
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|2dGridSystem",
        DisplayName="[Ck][2dGridSystem] Handle -> Grid Handle",
        meta = (CompactNodeTitle = "<As2dGrid>", BlueprintAutocast))
    static FCk_Handle_2dGridSystem
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Grid Handle",
        Category = "Ck|Utils|2dGridSystem",
        meta = (CompactNodeTitle = "INVALID_2dGridHandle", Keywords = "make"))
    static FCk_Handle_2dGridSystem
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get Dimensions")
    static FIntPoint
    Get_Dimensions(
        const FCk_Handle_2dGridSystem& InGrid);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get Pivot")
    static FTransform
    Get_Pivot(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_LocalWorld InLocalWorld);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Request Update Pivot")
    static void
    Request_UpdatePivot(
        const FCk_Handle_2dGridSystem& InGrid,
        FVector InLocationOffset,
        FRotator InRotationOffset = FRotator::ZeroRotator);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get Pivot Transform For Anchor")
    static FTransform
    Get_PivotTransformForAnchor(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_PivotAnchor InAnchor);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Request Set Pivot To Anchor")
    static FTransform
    Request_SetPivotToAnchor(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_PivotAnchor InAnchor);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get Cell Size")
    static FVector2D
    Get_CellSize(
        const FCk_Handle_2dGridSystem& InGrid);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get Cell At Coordinate")
    static FCk_Handle_2dGridCell
    Get_CellAt(
        const FCk_Handle_2dGridSystem& InGrid,
        const FIntPoint& InCoordinate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get All Cells")
    static TArray<FCk_Handle_2dGridCell>
    Get_AllCells(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_CellFilter InCellFilter);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get All Cells (As Coordinate)")
    static TArray<FIntPoint>
    Get_AllCells_AsCoordinate(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_CellFilter InCellFilter);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get Footprint")
    static FIntPoint
    Get_Footprint(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_CellFilter InCellFilter);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get Intersects With")
    static bool
    Get_IntersectsWith(
        const FCk_Handle_2dGridSystem& InGridA,
        const FCk_Handle_2dGridSystem& InGridB,
        ECk_2dGridSystem_CellFilter InFilterA = ECk_2dGridSystem_CellFilter::OnlyActiveCells,
        ECk_2dGridSystem_CellFilter InFilterB = ECk_2dGridSystem_CellFilter::OnlyActiveCells);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get Is Aligned With")
    static bool
    Get_IsAlignedWith(
        const FCk_Handle_2dGridSystem& InGridA,
        const FCk_Handle_2dGridSystem& InGridB,
        float InAlignmentTolerance = 1.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get Grid Intersections")
    static FCk_GridIntersectionResult
    Get_Intersections(
        const FCk_Handle_2dGridSystem& InGridA,
        const FCk_Handle_2dGridSystem& InGridB,
        ECk_2dGridSystem_CellFilter InFilterA = ECk_2dGridSystem_CellFilter::OnlyActiveCells,
        ECk_2dGridSystem_CellFilter InFilterB = ECk_2dGridSystem_CellFilter::OnlyActiveCells,
        float InCellOverlapThreshold0to1 = 0.5f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get Grid Intersections (Simple)")
    static TArray<FCk_GridCellIntersection>
    Get_IntersectingCells(
        const FCk_Handle_2dGridSystem& InGridA,
        const FCk_Handle_2dGridSystem& InGridB,
        float InTolerancePercent = 0.5f);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|2dGridSystem",
        DisplayName="[Ck][2dGridSystem] Get Position For Anchor")
    static FVector
    Get_PositionForAnchor(
        const FCk_Handle_2dGridSystem& InGrid,
        const FIntPoint& InDesiredAnchorCoordinate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get Bounds")
    static FBox2D
    Get_Bounds(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_LocalWorld InLocalOrWorld,
        ECk_2dGridSystem_CellFilter InCellFilter = ECk_2dGridSystem_CellFilter::OnlyActiveCells);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridSystem|Debug",
              DisplayName="[Ck][2dGridSystem] Debug Draw Grid",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DebugDraw_Grid(
        const UObject* InWorldContextObject,
        const FCk_Handle_2dGridSystem& InGrid,
        const FCk_2dGridSystem_DebugDraw_Options& InOptions = FCk_2dGridSystem_DebugDraw_Options());

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridSystem|Debug",
              DisplayName="[Ck][2dGridSystem] Debug Draw Grid (Simple)",
              meta = (WorldContext = "InWorldContextObject", DevelopmentOnly))
    static void
    DebugDraw_Grid_Simple(
        const UObject* InWorldContextObject,
        const FCk_Handle_2dGridSystem& InGrid,
        float InDuration = 0.0f);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] ForEach Cell")
    static TArray<FCk_Handle_2dGridCell>
    ForEach_Cell(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_CellFilter InCellFilter);

public:
    static auto
    ForEach_Cell(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_CellFilter InCellFilter,
        const TFunction<void(FCk_Handle_2dGridCell)>& InFunc) -> void;

private:
    private:
    static FCk_GridIntersectionResult
    DoGet_Intersections_Aligned(
        const FCk_Handle_2dGridSystem& InGridA,
        const FCk_Handle_2dGridSystem& InGridB,
        ECk_2dGridSystem_CellFilter InFilterA,
        ECk_2dGridSystem_CellFilter InFilterB,
        float InCellOverlapThreshold0to1);

    static FCk_GridIntersectionResult
    DoGet_Intersections(
        const FCk_Handle_2dGridSystem& InGridA,
        const FCk_Handle_2dGridSystem& InGridB,
        ECk_2dGridSystem_CellFilter InFilterA,
        ECk_2dGridSystem_CellFilter InFilterB,
        float InCellOverlapThreshold0to1);
};

// --------------------------------------------------------------------------------------------------------------------