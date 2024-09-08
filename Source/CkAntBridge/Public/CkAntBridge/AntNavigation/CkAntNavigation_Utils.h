#pragma once

#include "CkECS/Handle/CkHandle.h"
#include "CkNet/CkNet_Utils.h"

#include "CkAntNavigation_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKANTBRIDGE_API UCk_Utils_AntNavigation_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AntNavigation_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AntNavigation",
              DisplayName="Add AntNavigation")
    static void
    Add(
        UPARAM(ref) FCk_Handle InHandle,
        const FCk_Fragment_AntNavigation_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AntNavigation",
              DisplayName="Has AntNavigation")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|AntNavigation",
              DisplayName="Ensure Has AntNavigation")
    static bool
    Ensure(
        FCk_Handle InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
