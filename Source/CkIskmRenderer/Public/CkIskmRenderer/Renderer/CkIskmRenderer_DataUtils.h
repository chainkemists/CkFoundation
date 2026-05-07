#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "CkIskmRenderer/Renderer/CkIskmRenderer_Fragment_Data.h"

#include "CkIskmRenderer_DataUtils.generated.h"

class UCk_IskmAnimCollection_Data;

UCLASS(NotBlueprintable)
class CKISKMRENDERER_API UCk_Utils_IskmRendererData_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IskmRendererData_UE);

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmRenderer",
        DisplayName="[Ck][IskmRendererData] Find Submesh Index By Name")
    static int32
    Find_SubmeshIndex_ByName(
        const UCk_IskmRenderer_Data* InAsset,
        FName InName);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmRenderer",
        DisplayName="[Ck][IskmRendererData] Get AnimCollection")
    static UCk_IskmAnimCollection_Data*
    Get_AnimCollection(const UCk_IskmRenderer_Data* InAsset);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmRenderer",
        DisplayName="[Ck][IskmRendererData] Get Max Submesh Per Instance")
    static int32
    Get_MaxSubmeshPerInstance(const UCk_IskmRenderer_Data* InAsset);
};
