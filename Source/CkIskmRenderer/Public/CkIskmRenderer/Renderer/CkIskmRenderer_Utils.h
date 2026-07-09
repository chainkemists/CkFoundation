#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkIskmRenderer/Renderer/CkIskmRenderer_Fragment_Data.h"

#include "CkIskmRenderer_Utils.generated.h"

class UCk_IskmAnimCollection_Data;
class UCk_IskmRenderer_Data;

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_IskmRenderer"))
class CKISKMRENDERER_API UCk_Utils_IskmRenderer_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IskmRenderer_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_IskmRenderer);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmRenderer",
        DisplayName="[Ck][IskmRenderer] Add")
    static FCk_Handle_IskmRenderer
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        UCk_IskmRenderer_Data* InRendererData);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmRenderer",
        DisplayName="[Ck][IskmRenderer] Create")
    static FCk_Handle_IskmRenderer
    Create(
        UPARAM(ref) FCk_Handle& InOwner,
        UCk_IskmRenderer_Data* InRendererData);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmRenderer",
        DisplayName="[Ck][IskmRenderer] Has")
    static bool
    Has(const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmRenderer",
        DisplayName="[Ck][IskmRenderer] Get RendererData")
    static UCk_IskmRenderer_Data*
    Get_RendererData(const FCk_Handle_IskmRenderer& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmRenderer",
        DisplayName="[Ck][IskmRenderer] Get AnimCollection")
    static UCk_IskmAnimCollection_Data*
    Get_AnimCollection(const FCk_Handle_IskmRenderer& InHandle);
};
