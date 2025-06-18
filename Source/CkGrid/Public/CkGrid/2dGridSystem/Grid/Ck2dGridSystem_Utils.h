#pragma once

#include "Ck2dGridSystem_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Fragment_Data.h"

#include "Ck2dGridSystem_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
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
        UPARAM(ref) FCk_Handle& InHandle,
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
              DisplayName="[Ck][2dGridSystem] Get Cell Size")
    static FVector2D
    Get_CellSize(
        const FCk_Handle_2dGridSystem& InGrid);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get Transform")
    static FTransform
    Get_Transform(
        const FCk_Handle_2dGridSystem& InGrid);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get Active Coordinates")
    static TSet<FIntPoint>
    Get_ActiveCoordinates(
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

public:
    static auto
    ForEach_Cell(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_CellFilter InCellFilter,
        const TFunction<void(FCk_Handle_2dGridCell)>& InFunc) -> void;
};

// --------------------------------------------------------------------------------------------------------------------