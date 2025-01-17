#pragma once

#include "CkShapes/Cylinder/CkShapeCylinder_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkNet/CkNet_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkShapeCylinder_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKSHAPES_API UCk_Utils_ShapeCylinder_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ShapeCylinder_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ShapeCylinder);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ShapeCylinder",
              DisplayName="Add ShapeCylinder")
    static FCk_Handle_ShapeCylinder
    Add(
        UPARAM(ref) FCk_Handle InHandle,
        const FCk_Fragment_ShapeCylinder_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ShapeCylinder",
              DisplayName="Has ShapeCylinder")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ShapeCylinder",
        DisplayName="[Ck][ShapeCylinder] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ShapeCylinder
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ShapeCylinder",
        DisplayName="[Ck][ShapeCylinder] Handle -> ShapeCylinder Handle",
        meta = (CompactNodeTitle = "<AsShapeCylinder>", BlueprintAutocast))
    static FCk_Handle_ShapeCylinder
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ShapeCylinder",
        DisplayName="[Ck][ShapeCylinder] Request ExampleRequest")
    static FCk_Handle_ShapeCylinder
    Request_ExampleRequest(
        UPARAM(ref) FCk_Handle_ShapeCylinder& InShapeCylinder,
        const FCk_Request_ShapeCylinder_ExampleRequest& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------