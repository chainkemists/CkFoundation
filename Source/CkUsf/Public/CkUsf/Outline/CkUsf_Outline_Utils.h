#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkUsf/Outline/CkUsf_Outline_Types.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkUsf_Outline_Utils.generated.h"

class UCkUsf_OutlinePreset;

UCLASS(NotBlueprintable)
class CKUSF_API UCk_Utils_Usf_Outline_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Usf_Outline_UE);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Usf|Outline",
              DisplayName = "[Ck][Usf] Set Outline Claim (Entity)",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle Set_OutlineClaim(
        UPARAM(ref) FCk_Handle& InTarget,
        const FCk_Handle& InSource,
        UPARAM(meta = (Categories = "Outline")) FGameplayTag InOutlineTag,
        ECk_Usf_OutlineScope InScope,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Usf|Outline",
              DisplayName = "[Ck][Usf] Clear Outline Claim (Entity)",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle Clear_OutlineClaim(
        UPARAM(ref) FCk_Handle& InTarget,
        const FCk_Handle& InSource,
        UPARAM(meta = (Categories = "Outline")) FGameplayTag InOutlineTag,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Usf|Outline")
    static bool Has_OutlineClaim(
        const FCk_Handle& InTarget,
        const FCk_Handle& InSource,
        UPARAM(meta = (Categories = "Outline")) FGameplayTag InOutlineTag);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Usf|Outline")
    static bool Has_Outline(const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Usf|Outline")
    static UCkUsf_OutlinePreset* TryGet_OutlinePreset(const FCk_Handle& InHandle);
};
