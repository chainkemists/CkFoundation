#pragma once

#include "CkShapes/Box/CkShapeBox_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkNet/CkNet_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkShapeBox_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKSHAPES_API UCk_Utils_ShapeBox_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ShapeBox_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ShapeBox);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ShapeBox",
              DisplayName="Add ShapeBox")
    static FCk_Handle_ShapeBox
    Add(
        UPARAM(ref) FCk_Handle InHandle,
        const FCk_Fragment_ShapeBox_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ShapeBox",
              DisplayName="Has ShapeBox")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ShapeBox",
        DisplayName="[Ck][ShapeBox] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ShapeBox
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ShapeBox",
        DisplayName="[Ck][ShapeBox] Handle -> ShapeBox Handle",
        meta = (CompactNodeTitle = "<AsShapeBox>", BlueprintAutocast))
    static FCk_Handle_ShapeBox
    DoCastChecked(
        FCk_Handle InHandle);

public:
	UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ShapeBox",
        DisplayName="[Ck][ShapeBox] Request ExampleRequest")
    static FCk_Handle_ShapeBox
    Request_ExampleRequest(
        UPARAM(ref) FCk_Handle_ShapeBox& InShapeBox,
        const FCk_Request_ShapeBox_ExampleRequest& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------