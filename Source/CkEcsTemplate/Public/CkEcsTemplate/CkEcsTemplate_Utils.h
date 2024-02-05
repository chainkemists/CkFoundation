#pragma once

#include "CkEcsTemplate/CkEcsTemplate_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkNet/CkNet_Utils.h"
#include "CkSignal/CkSignal_Fragment_Data.h"

#include "CkEcsTemplate_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKECSTEMPLATE_API UCk_Utils_EcsTemplate_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EcsTemplate_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EcsTemplate",
              DisplayName="Add EcsTemplate")
    static void
    Add(
        UPARAM(ref) FCk_Handle InHandle,
        const FCk_Fragment_EcsTemplate_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EcsTemplate",
              DisplayName="Has EcsTemplate")
    static bool
    Has(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EcsTemplate",
              DisplayName="Ensure Has EcsTemplate")
    static bool
    Ensure(
        FCk_Handle InHandle);
};

// --------------------------------------------------------------------------------------------------------------------
