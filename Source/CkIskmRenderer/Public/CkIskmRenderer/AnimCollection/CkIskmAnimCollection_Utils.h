#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"

#include "CkIskmAnimCollection_Utils.generated.h"

UCLASS(NotBlueprintable)
class CKISKMRENDERER_API UCk_Utils_IskmAnimCollection_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IskmAnimCollection_UE);

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmAnimCollection",
        DisplayName="[Ck][IskmAnimCollection] Find Sequence Index By Asset")
    static int32
    Find_SequenceIndex_ByAsset(
        const UCk_IskmAnimCollection_Data* InAsset,
        const UAnimSequenceBase* InSequence);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmAnimCollection",
        DisplayName="[Ck][IskmAnimCollection] Get Skeleton")
    static USkeleton*
    Get_Skeleton(const UCk_IskmAnimCollection_Data* InAsset);
};
