#pragma once

#include "CkShapes/Capsule/CkShapeCapsule_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkNet/CkNet_Utils.h"

#include "CkShapeCapsule_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKSHAPES_API UCk_Utils_ShapeCapsule_UE : public UCk_Utils_Ecs_Net_UE
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
        UPARAM(ref) FCk_Handle InHandle,
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
        DisplayName="[Ck][ShapeCapsule] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ShapeCapsule
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ShapeCapsule",
        DisplayName="[Ck][ShapeCapsule] Handle -> ShapeCapsule Handle",
        meta = (CompactNodeTitle = "<AsShapeCapsule>", BlueprintAutocast))
    static FCk_Handle_ShapeCapsule
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ShapeCapsule",
        DisplayName="[Ck][ShapeCapsule] Request ExampleRequest")
    static FCk_Handle_ShapeCapsule
    Request_UpdateShape(
        UPARAM(ref) FCk_Handle_ShapeCapsule& InShapeCapsule,
        const FCk_Request_ShapeCapsule_UpdateShape& InRequest);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ShapeCapsule",
        DisplayName="[Ck][ShapeCapsule] Request ExampleRequest")
    static FCk_Fragment_ShapeCapsule_ShapeData
    Get_ShapeData(
        const FCk_Handle_ShapeCapsule& InShapeCapsule);
};

// --------------------------------------------------------------------------------------------------------------------
