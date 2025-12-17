#pragma once

#include "CkStateTree/CkStateTree_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkStateTree_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_StateTree"))
class CKSTATETREE_API UCk_Utils_StateTree_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_StateTree_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_StateTree);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|StateTree",
              DisplayName="[Ck][StateTree] Add Feature")
    static FCk_Handle_StateTree
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_StateTree_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|StateTree",
              DisplayName="[Ck][StateTree] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|StateTree",
        DisplayName="[Ck][StateTree] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_StateTree
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|StateTree",
        DisplayName="[Ck][StateTree] Handle -> StateTree Handle",
        meta = (CompactNodeTitle = "<AsStateTree>", BlueprintAutocast))
    static FCk_Handle_StateTree
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid StateTree Handle",
        Category = "Ck|Utils|StateTree",
        meta = (CompactNodeTitle = "INVALID_StateTreeHandle", Keywords = "make"))
    static FCk_Handle_StateTree
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|StateTree",
        DisplayName="[Ck][StateTree] Request ExampleRequest")
    static FCk_Handle_StateTree
    Request_ExampleRequest(
        UPARAM(ref) FCk_Handle_StateTree& InStateTree,
        const FCk_Request_StateTree_ExampleRequest& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------