#pragma once

#include "CkBallistics/CkBallistics_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkBallistics_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKBALLISTICS_API UCk_Utils_Ballistics_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Ballistics_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Ballistics);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Ballistics",
              DisplayName="Add Ballistics")
    static FCk_Handle_Ballistics
    Add(
        UPARAM(ref) FCk_Handle InHandle,
        const FCk_Fragment_Ballistics_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Ballistics",
              DisplayName="Has Ballistics")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Ballistics",
        DisplayName="[Ck][Ballistics] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Ballistics
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Ballistics",
        DisplayName="[Ck][Ballistics] Handle -> Ballistics Handle",
        meta = (CompactNodeTitle = "<AsBallistics>", BlueprintAutocast))
    static FCk_Handle_Ballistics
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Ballistics Handle",
        Category = "Ck|Utils|Ballistics",
        meta = (CompactNodeTitle = "INVALID_BallisticsHandle", Keywords = "make"))
    static FCk_Handle_Ballistics
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Ballistics",
        DisplayName="[Ck][Ballistics] Request ExampleRequest")
    static FCk_Handle_Ballistics
    Request_ExampleRequest(
        UPARAM(ref) FCk_Handle_Ballistics& InBallistics,
        const FCk_Request_Ballistics_ExampleRequest& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------