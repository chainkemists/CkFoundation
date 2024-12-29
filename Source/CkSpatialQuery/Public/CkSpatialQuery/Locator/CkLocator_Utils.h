#pragma once

#include "CkLocator_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkNet/CkNet_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkLocator_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKSPATIALQUERY_API UCk_Utils_Locator_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Locator_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Locator);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Locator",
              DisplayName="Add Locator")
    static FCk_Handle_Locator
    Add(
        UPARAM(ref) FCk_Handle InHandle,
        const FCk_Fragment_Locator_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Locator",
              DisplayName="Has Locator")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Locator",
        DisplayName="[Ck][Locator] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Locator
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Locator",
        DisplayName="[Ck][Locator] Handle -> Locator Handle",
        meta = (CompactNodeTitle = "<AsLocator>", BlueprintAutocast))
    static FCk_Handle_Locator
    DoCastChecked(
        FCk_Handle InHandle);

public:
	UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Locator",
        DisplayName="[Ck][Locator] Request ExampleRequest")
    static FCk_Handle_Locator
    Request_ExampleRequest(
        UPARAM(ref) FCk_Handle_Locator& InLocator,
        const FCk_Request_Locator_ExampleRequest& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------