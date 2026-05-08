#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment_Data.h"

#include "CkIskmProxy_Utils.generated.h"

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_IskmProxy"))
class CKISKMRENDERER_API UCk_Utils_IskmProxy_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IskmProxy_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_IskmProxy);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Add")
    static FCk_Handle_IskmProxy
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_IskmProxy_ParamsData& InParams);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmProxy",
        DisplayName="[Ck][IskmProxy] Has")
    static bool
    Has(const FCk_Handle& InHandle);
};
