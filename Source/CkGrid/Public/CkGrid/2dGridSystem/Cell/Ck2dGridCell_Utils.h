#pragma once

#include "Ck2dGridCell_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkCore/Enums/CkEnums.h"

#include "Ck2dGridCell_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Handle_2dGridSystem;

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKGRID_API UCk_Utils_2dGridCell_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_2dGridCell_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_2dGridCell);

private:
    static FCk_Handle_2dGridCell
    Create(
        FCk_Handle_2dGridSystem& InParentGrid,
        const FCk_Fragment_2dGridCell_ParamsData& InParams,
        ECk_EnableDisable InEnabledState = ECk_EnableDisable::Enable);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridCell",
              DisplayName="[Ck][2dGridCell] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|2dGridCell",
        DisplayName="[Ck][2dGridCell] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_2dGridCell
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|2dGridCell",
        DisplayName="[Ck][2dGridCell] Handle -> Cell Handle",
        meta = (CompactNodeTitle = "<As2dGridCell>", BlueprintAutocast))
    static FCk_Handle_2dGridCell
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Cell Handle",
        Category = "Ck|Utils|2dGridCell",
        meta = (CompactNodeTitle = "INVALID_2dGridCellHandle", Keywords = "make"))
    static FCk_Handle_2dGridCell
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridCell",
              DisplayName="[Ck][2dGridCell] Get Parent Grid")
    static FCk_Handle_2dGridSystem
    Get_ParentGrid(
        const FCk_Handle_2dGridCell& InCell);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridCell",
              DisplayName="[Ck][2dGridCell] Get Index")
    static int32
    Get_Index(
        const FCk_Handle_2dGridCell& InCell);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridCell",
              DisplayName="[Ck][2dGridCell] Get Coordinate")
    static FIntPoint
    Get_Coordinate(
        const FCk_Handle_2dGridCell& InCell);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridCell",
              DisplayName="[Ck][2dGridCell] Get Tags")
    static FGameplayTagContainer
    Get_Tags(
        const FCk_Handle_2dGridCell& InCell);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridCell",
              DisplayName="[Ck][2dGridCell] Get Is Disabled")
    static bool
    Get_IsDisabled(
        const FCk_Handle_2dGridCell& InCell);

private:
    friend class UCk_Utils_2dGridSystem_UE;
};

// --------------------------------------------------------------------------------------------------------------------