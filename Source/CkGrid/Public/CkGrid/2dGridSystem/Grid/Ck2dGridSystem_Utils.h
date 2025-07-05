#pragma once

#include "Ck2dGridSystem_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkEcsExt/SceneNode/CkSceneNode_Fragment_Data.h"

#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Fragment_Data.h"

#include "Ck2dGridSystem_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_2dGridSystem_DebugDraw_CellVisualization : uint8
{
    None,
    Outline,
    Filled
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_2dGridSystem_DebugDraw_CellVisualization);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_2dGridSystem_PivotAnchor : uint8
{
    Center,
    BottomLeft,
    BottomCenter,
    BottomRight,
    MiddleLeft,
    MiddleRight,
    TopLeft,
    TopCenter,
    TopRight
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_2dGridSystem_PivotAnchor);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGRID_API FCk_2dGridSystem_DebugDraw_Options
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_2dGridSystem_DebugDraw_Options);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_2dGridSystem_DebugDraw_CellVisualization _CellVisualization = ECk_2dGridSystem_DebugDraw_CellVisualization::Outline;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _ShowCoordinates = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _ShowPivot = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    bool _ShowCellSizeInfo = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FLinearColor _EnabledCellColor = FLinearColor::Green;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FLinearColor _DisabledCellColor = FLinearColor::Red;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FLinearColor _PivotColor = FLinearColor::Yellow;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FLinearColor _TextColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _CellThickness = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _PivotSize = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _Duration = 0.0f;

public:
    CK_PROPERTY(_CellVisualization);
    CK_PROPERTY(_ShowCoordinates);
    CK_PROPERTY(_ShowPivot);
    CK_PROPERTY(_ShowCellSizeInfo);
    CK_PROPERTY(_EnabledCellColor);
    CK_PROPERTY(_DisabledCellColor);
    CK_PROPERTY(_PivotColor);
    CK_PROPERTY(_TextColor);
    CK_PROPERTY(_CellThickness);
    CK_PROPERTY(_PivotSize);
    CK_PROPERTY(_Duration);
};

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
    static FCk_Handle_SceneNode
    Get_Pivot(
        const FCk_Handle_2dGridSystem& InGrid);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Request Update Pivot")
    static void
    Request_UpdatePivot(
        const FCk_Handle_2dGridSystem& InGrid,
        const FTransform& InNewPivotTransform);

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
    static void
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
              DisplayName="[Ck][2dGridSystem] Get Cell Bounds Dimensions")
    static FIntPoint
    Get_CellBoundsDimensions(
        const FCk_Handle_2dGridSystem& InGrid,
        ECk_2dGridSystem_CellFilter InCellFilter);

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

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|2dGridSystem",
        DisplayName="[Ck][2dGridSystem] Get Transform Rotation Degrees")
    static int32
    Get_TransformRotationDegrees(
        const FCk_Handle_2dGridSystem& InGrid);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|2dGridSystem",
        DisplayName="[Ck][2dGridSystem] Get Coordinate Remapped For Transform")
    static FIntPoint
    Get_CoordinateRemappedForTransform(
        const FCk_Handle_2dGridSystem& InGrid,
        const FIntPoint& InLocalCoordinate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridSystem",
              DisplayName="[Ck][2dGridSystem] Get Entity Rotation Degrees")
    static float
    Get_EntityRotationDegrees(
        const FCk_Handle_2dGridSystem& InGrid);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|2dGridSystem", DisplayName="[Ck][2dGridSystem] Get Position For Anchor")
    static FVector
    Get_PositionForAnchor(
        const FCk_Handle_2dGridSystem& InGrid,
        const FIntPoint& InDesiredAnchorCoordinate);

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
};

// --------------------------------------------------------------------------------------------------------------------