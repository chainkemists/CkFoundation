#pragma once

#include "CkSpatialQuery/Probe/CkProbe_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkNet/CkNet_Utils.h"

#include "CkProbe_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKSPATIALQUERY_API UCk_Utils_Probe_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Probe_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Probe);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Probe",
              DisplayName="Add Probe")
    static FCk_Handle_Probe
    Add(
        UPARAM(ref) FCk_Handle InHandle,
        const FCk_Fragment_Probe_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Probe",
              DisplayName="Has Probe")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Probe
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Handle -> Probe Handle",
        meta = (CompactNodeTitle = "<AsProbe>", BlueprintAutocast))
    static FCk_Handle_Probe
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Request ExampleRequest")
    static FCk_Handle_Probe
    Request_ExampleRequest(
        UPARAM(ref) FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_ExampleRequest& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------