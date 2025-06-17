#pragma once

#include "Ck2dGridSystem_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Fragment_Data.h"

#include <Math/TransformCalculus2D.h>

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
              DisplayName="[Ck][2dGridSystem] Create New Grid System")
    static FCk_Handle_2dGridSystem
    Create(
        const FCk_Handle& InOwner,
        const FCk_Fragment_2dGridSystem_ParamsData& InParams);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Create New Grid System (Transient)",
              meta = (WorldContext="InWorldContextObject", DefaultToSelf="InWorldContextObject"))
    static FCk_Handle_2dGridSystem
    Create_Transient(
        const FCk_Fragment_2dGridSystem_ParamsData& InParams,
        const UObject* InWorldContextObject);

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
        meta = (CompactNodeTitle = "<AsGrid>", BlueprintAutocast))
    static FCk_Handle_2dGridSystem
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Grid Handle",
        Category = "Ck|Utils|2dGridSystem",
        meta = (CompactNodeTitle = "INVALID_GridHandle", Keywords = "make"))
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

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Set Transform")
    static FCk_Handle_2dGridSystem
    Set_Transform(
        UPARAM(ref) FCk_Handle_2dGridSystem& InGrid,
        const FTransform& InTransform);

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
        const FCk_Handle_2dGridSystem& InGrid);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get Active Cells")
    static TArray<FCk_Handle_2dGridCell>
    Get_ActiveCells(
        const FCk_Handle_2dGridSystem& InGrid);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get Inactive Cells")
    static TArray<FCk_Handle_2dGridCell>
    Get_InactiveCells(
        const FCk_Handle_2dGridSystem& InGrid);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] For Each Cell",
              meta=(AutoCreateRefTerm="InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_2dGridCell>
    ForEach_Cell(
        const FCk_Handle_2dGridSystem& InGrid,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] For Each Active Cell",
              meta=(AutoCreateRefTerm="InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_2dGridCell>
    ForEach_ActiveCell(
        const FCk_Handle_2dGridSystem& InGrid,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);

public:
    static auto
    ForEach_Cell(
        const FCk_Handle_2dGridSystem& InGrid,
        const TFunction<void(FCk_Handle_2dGridCell)>& InFunc) -> void;

    static auto
    ForEach_ActiveCell(
        const FCk_Handle_2dGridSystem& InGrid,
        const TFunction<void(FCk_Handle_2dGridCell)>& InFunc) -> void;
};

// --------------------------------------------------------------------------------------------------------------------