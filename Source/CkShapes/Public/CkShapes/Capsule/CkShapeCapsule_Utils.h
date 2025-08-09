#pragma once

#include "CkShapes/Capsule/CkShapeCapsule_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkShapeCapsule_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_ShapeCapsule"))
class CKSHAPES_API UCk_Utils_ShapeCapsule_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ShapeCapsule_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ShapeCapsule);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ShapeCapsule",
              DisplayName="[Ck][Shapes][Capsule] Add Feature")
    static FCk_Handle_ShapeCapsule
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_ShapeCapsule_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ShapeCapsule",
              DisplayName="[Ck][Shapes][Capsule] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ShapeCapsule",
        DisplayName="[Ck][Shapes][Capsule] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ShapeCapsule
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ShapeCapsule",
        DisplayName="[Ck][Shapes][Capsule] Handle -> ShapeCapsule Handle",
        meta = (CompactNodeTitle = "<AsShapeCapsule>", BlueprintAutocast))
    static FCk_Handle_ShapeCapsule
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid ShapeCapsule Handle",
        Category = "Ck|Utils|ShapeCapsule",
        meta = (CompactNodeTitle = "INVALID_ShapeCapsuleHandle", Keywords = "make"))
    static FCk_Handle_ShapeCapsule
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ShapeCapsule",
        DisplayName="[Ck][Shapes][Capsule] Request Update Dimensions")
    static FCk_Handle_ShapeCapsule
    Request_UpdateDimensions(
        UPARAM(ref) FCk_Handle_ShapeCapsule& InShapeCapsule,
        const FCk_Request_ShapeCapsule_UpdateDimensions& InRequest);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ShapeCapsule",
        DisplayName="[Ck][Shapes][Capsule] Get Dimensions")
    static FCk_ShapeCapsule_Dimensions
    Get_Dimensions(
        const FCk_Handle_ShapeCapsule& InShapeCapsule);
};

// --------------------------------------------------------------------------------------------------------------------