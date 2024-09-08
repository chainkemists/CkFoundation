#pragma once

#include "CkAntBridge/AntSquad/CkAntSquad_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkNet/CkNet_Utils.h"

#include "CkAntSquad_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKANTBRIDGE_API UCk_Utils_AntSquad_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AntSquad_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_AntSquad);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AntSquad",
              DisplayName="[Ck][AntSquad] Add Feature")
    static FCk_Handle_AntSquad
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_AntSquad_ParamsData& InParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AntSquad",
              DisplayName="[Ck][AntSquad] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AntSquad",
              DisplayName="[Ck][AntSquad] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_AntSquad
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AntSquad",
              DisplayName="[Ck][AntSquad] Handle -> AntSquad Handle",
              meta = (CompactNodeTitle = "<AsAntSquad>", BlueprintAutocast))
    static FCk_Handle_AntSquad
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AntSquad",
              DisplayName="[Ck][AntSquad] Request Add Agent",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_AntSquad
    Request_AddAgent(
        UPARAM(ref) FCk_Handle_AntSquad& InAntSquadHandle,
        const FCk_Request_AntSquad_AddAgent& InRequest);

public:
    // TODO: Add Bind/Unbind
};

// --------------------------------------------------------------------------------------------------------------------
