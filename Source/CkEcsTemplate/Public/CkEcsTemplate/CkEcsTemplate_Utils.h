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
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_EcsTemplate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EcsTemplate",
              DisplayName="[Ck][EcsTemplate] Add Feature")
    static FCk_Handle_EcsTemplate
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_EcsTemplate_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EcsTemplate",
              DisplayName="[Ck][EcsTemplate] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EcsTemplate",
        DisplayName="[Ck][EcsTemplate] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_EcsTemplate
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|EcsTemplate",
        DisplayName="[Ck][EcsTemplate] Handle -> EcsTemplate Handle",
        meta = (CompactNodeTitle = "<AsEcsTemplate>", BlueprintAutocast))
    static FCk_Handle_EcsTemplate
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid EcsTemplate Handle",
        Category = "Ck|Utils|EcsTemplate",
        meta = (CompactNodeTitle = "INVALID_EcsTemplateHandle", Keywords = "make"))
    static FCk_Handle_EcsTemplate
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EcsTemplate",
        DisplayName="[Ck][EcsTemplate] Request ExampleRequest")
    static FCk_Handle_EcsTemplate
    Request_ExampleRequest(
        UPARAM(ref) FCk_Handle_EcsTemplate& InEcsTemplate,
        const FCk_Request_EcsTemplate_ExampleRequest& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------