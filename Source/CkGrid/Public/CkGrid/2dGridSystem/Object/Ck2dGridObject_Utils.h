#pragma once

#include "Ck2dGridObject_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Enums/CkEnums.h"

#include <GameplayTagContainer.h>

#include "Ck2dGridObject_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_2dGridObject"))
class CKGRID_API UCk_Utils_2dGridObject_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_2dGridObject_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_2dGridObject);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridObject",
              DisplayName="[Ck][2dGridObject] Add Feature")
    static FCk_Handle_2dGridObject
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_2dGridObject_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridObject",
              DisplayName="[Ck][2dGridObject] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|2dGridObject",
        DisplayName="[Ck][2dGridObject] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_2dGridObject
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|2dGridObject",
        DisplayName="[Ck][2dGridObject] Handle -> Object Handle",
        meta = (CompactNodeTitle = "<As2dGridObject>", BlueprintAutocast))
    static FCk_Handle_2dGridObject
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Object Handle",
        Category = "Ck|Utils|2dGridObject",
        meta = (CompactNodeTitle = "INVALID_2dGridObjectHandle", Keywords = "make"))
    static FCk_Handle_2dGridObject
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridObject",
              DisplayName="[Ck][2dGridObject] Get Footprint Extent")
    static FIntPoint
    Get_FootprintExtent(
        const FCk_Handle_2dGridObject& InObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridObject",
              DisplayName="[Ck][2dGridObject] Get Required Cell Tags")
    static FGameplayTagContainer
    Get_RequiredCellTags(
        const FCk_Handle_2dGridObject& InObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridObject",
              DisplayName="[Ck][2dGridObject] Get Forbidden Cell Tags")
    static FGameplayTagContainer
    Get_ForbiddenCellTags(
        const FCk_Handle_2dGridObject& InObject);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridObject",
              DisplayName="[Ck][2dGridObject] Get Resolved Cells")
    static TArray<FIntPoint>
    Get_ResolvedCells(
        const FCk_Handle_2dGridObject& InObject,
        const FIntPoint& InAnchor,
        ECk_CardinalRotation InRotation);
};

// --------------------------------------------------------------------------------------------------------------------
